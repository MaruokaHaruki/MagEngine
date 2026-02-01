#define _USE_MATH_DEFINES
#include "EnemyBullet.h"
#include "CloudImpactHelper.h"
#include "Object3dSetup.h"
#include "Particle.h"
#include "Player.h"
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void EnemyBullet::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position, const Vector3 &direction) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	transform_.translate = position;
	transform_.scale = {0.5f, 0.5f, 0.5f};

	// 進行方向に向けて回転を設定
	// Y軸周りの回転（ヨー）
	float yaw = std::atan2(direction.x, direction.z);

	// X軸周りの回転（ピッチ）
	float horizontalDistance = std::sqrt(direction.x * direction.x + direction.z * direction.z);
	float pitch = std::atan2(-direction.y, horizontalDistance);

	transform_.rotate = {pitch, yaw, 0.0f};

	velocity_ = {
		direction.x * EnemyBulletConstants::kSpeed,
		direction.y * EnemyBulletConstants::kSpeed,
		direction.z * EnemyBulletConstants::kSpeed};

	radius_ = EnemyBulletConstants::kRadius;
	lifeTimer_ = 0.0f;
	isAlive_ = true;
	particle_ = nullptr;
	particleSetup_ = nullptr;

	BaseObject::Initialize(transform_.translate, radius_);
}

///=============================================================================
///                        パーティクルシステムの設定
void EnemyBullet::SetParticleSystem(MagEngine::Particle *particle, MagEngine::ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;
}

///=============================================================================
///                        更新
void EnemyBullet::Update() {
	if (!isAlive_)
		return;

	const float deltaTime = 1.0f / 60.0f;
	lifeTimer_ += deltaTime;

	transform_.translate.x += velocity_.x * deltaTime;
	transform_.translate.y += velocity_.y * deltaTime;
	transform_.translate.z += velocity_.z * deltaTime;

	if (lifeTimer_ >= EnemyBulletConstants::kLifeTime) {
		isAlive_ = false;
	}

	if (obj_) {
		if (Transform *objTransform = obj_->GetTransform()) {
			*objTransform = transform_;
		}
		obj_->Update();
	}

	BaseObject::Update(transform_.translate);
}

///=============================================================================
///                        描画
void EnemyBullet::Draw() {
	if (isAlive_ && obj_) {
		obj_->Draw();
	}
}

///=============================================================================
///                        衝突処理
void EnemyBullet::OnCollisionEnter(BaseObject *other) {
	if (!isAlive_)
		return;
	// 敵の弾がプレイヤーまたはプレイヤーの弾に衝突した場合に消滅
	if (dynamic_cast<Player *>(other) != nullptr) {
		// パーティクル生成
		if (particle_) {
			particle_->SetBillboard(true);
			particle_->SetVelocityRange({-3.0f, -2.0f, -3.0f}, {3.0f, 3.0f, 3.0f});
			particle_->SetColorRange({1.0f, 0.3f, 0.0f, 1.0f}, {1.0f, 0.6f, 0.2f, 1.0f});
			particle_->SetLifetimeRange(0.2f, 0.4f);
			particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.6f, 0.6f, 0.6f});
			particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
			particle_->SetGravity({0.0f, -3.0f, 0.0f});
			particle_->SetFadeInOut(0.0f, 0.8f);
			particle_->Emit("ExplosionSparks", transform_.translate, 10);
		}

		// 雲に影響を追加（敵弾）
		CloudImpactHelper::ApplyBulletImpact(GetPosition(), false); // false = 敵弾

		// 弾を消滅させる
		isAlive_ = false;
	}
}

void EnemyBullet::OnCollisionStay(BaseObject *other) {
	other; // 警告回避
}

void EnemyBullet::OnCollisionExit(BaseObject *other) {
	other; // 警告回避
}

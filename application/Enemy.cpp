#include "Enemy.h"
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include <cmath>

void Enemy::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	// 初期設定
	Transform *transform = obj_->GetTransform();
	if (transform) {
		transform->translate = position;
		transform->scale = {1.0f, 1.0f, 1.0f};
		transform->rotate = {0.0f, 3.14159f, 0.0f}; // プレイヤーの方向を向く（180度回転）
	}

	// 移動設定
	speed_ = 0.0f;					   // 敵の移動速度
	velocity_ = {0.0f, 0.0f, -speed_}; // プレイヤーに向かって移動（Z軸負方向）

	// その他の設定
	isAlive_ = true;
	radius_ = 1.0f;		   // 当たり判定の半径
	rotationSpeed_ = 1.0f; // 回転速度（ラジアン/秒）

	// パーティクル関連の初期化
	particle_ = nullptr;
	particleSetup_ = nullptr;
	particleCreated_ = false;

	// 破壊演出関連の初期化
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = 2.0f; // 2秒間破壊演出を表示

	// BaseObjectの初期化（当たり判定）
	Vector3 pos = position;
	BaseObject::Initialize(pos, radius_);
}

void Enemy::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;
}

void Enemy::Update() {
	if (destroyState_ == DestroyState::Dead || !obj_) {
		return;
	}

	Transform *transform = obj_->GetTransform();
	if (!transform) {
		return;
	}

	// 破壊演出中の処理
	if (destroyState_ == DestroyState::Destroying) {
		const float frameTime = 1.0f / 60.0f;
		destroyTimer_ += frameTime;
		
		// 破壊演出時間が経過したら完全に消滅
		if (destroyTimer_ >= destroyDuration_) {
			destroyState_ = DestroyState::Dead;
			isAlive_ = false;
		}
		return; // 破壊中は移動などの処理をスキップ
	}

	// 通常の更新処理（生存中のみ）
	if (destroyState_ == DestroyState::Alive) {
		// 移動処理
		const float frameTime = 1.0f / 60.0f;
		transform->translate.x += velocity_.x * frameTime;
		transform->translate.y += velocity_.y * frameTime;
		transform->translate.z += velocity_.z * frameTime;

		// 回転処理（Y軸周りに回転）
		transform->rotate.y += rotationSpeed_ * frameTime;

		// 画面外に出たら削除
		if (transform->translate.z < -20.0f) {
			destroyState_ = DestroyState::Dead;
			isAlive_ = false;
		}

		// BaseObjectのコライダー位置を更新
		BaseObject::Update(transform->translate);
	}

	// Object3dの更新
	obj_->Update();
}

void Enemy::Draw() {
	// 生存中のみ描画
	if (destroyState_ == DestroyState::Alive && obj_) {
		obj_->Draw();
	}
}

void Enemy::DrawImGui() {
	if (!obj_) {
		return;
	}

	Transform *transform = obj_->GetTransform();
	if (transform) {
		ImGui::Begin("Enemy Debug");
		ImGui::Text("Position: (%.2f, %.2f, %.2f)",
					transform->translate.x, transform->translate.y, transform->translate.z);
		ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
		ImGui::Text("Is Alive: %s", isAlive_ ? "Yes" : "No");
		ImGui::SliderFloat("Speed", &speed_, 0.5f, 10.0f);
		ImGui::SliderFloat("Rotation Speed", &rotationSpeed_, 0.1f, 5.0f);
		ImGui::SliderFloat("Radius", &radius_, 0.5f, 3.0f);
		ImGui::End();
	}
}

Vector3 Enemy::GetPosition() const {
	if (obj_) {
		return obj_->GetPosition();
	}
	return {0.0f, 0.0f, 0.0f};
}

void Enemy::OnCollisionEnter(BaseObject *other) {
	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	// プレイヤーの弾との衝突時の処理
	if (particle_ && !particleCreated_) {
		// 敵の位置でパーティクルを発生
		Vector3 enemyPos = GetPosition();
		particle_->Emit("DestroyEffect", enemyPos, 20); // 20個のパーティクルを生成
		particleCreated_ = true;
	}

	// 破壊状態に移行（すぐには消さない）
	destroyState_ = DestroyState::Destroying;
	destroyTimer_ = 0.0f;
}

void Enemy::OnCollisionStay(BaseObject *other) {
	// 継続中の衝突処理（必要に応じて実装）
}

void Enemy::OnCollisionExit(BaseObject *other) {
	// 衝突終了時の処理（必要に応じて実装）
}

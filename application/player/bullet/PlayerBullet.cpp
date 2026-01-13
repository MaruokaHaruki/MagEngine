#include "PlayerBullet.h"
#include "Object3d.h"
using namespace MagEngine;

void PlayerBullet::Initialize(MagEngine::Object3dSetup *object3dSetup,
	const std::string &modelPath,const Vector3 &position,const Vector3 &direction) {

	// Object3dの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	// 初期設定
	Transform *transform = obj_->GetTransform();
	if(transform) {
		transform->translate = position;
		transform->scale = { 0.1f, 0.1f, 0.1f }; // 弾は小さく
	}

	// 移動設定
	speed_ = 128.0f; // 弾の速度
	velocity_ = { direction.x * speed_, direction.y * speed_, direction.z * speed_ };

	// 生存時間設定
	lifeTime_ = 0.0f;
	maxLifeTime_ = 3.0f; // 3秒で消える
	isAlive_ = true;

	// 当たり判定設定
	radius_ = 0.5f;

	// BaseObjectの初期化（当たり判定）
	Vector3 pos = position;
	BaseObject::Initialize(pos, radius_);
}

void PlayerBullet::Update() {
	if(!isAlive_ || !obj_) {
		return;
	}

	Transform *transform = obj_->GetTransform();
	if(!transform) {
		return;
	}

	// 移動処理
	const float frameTime = 1.0f / 60.0f;
	transform->translate.x += velocity_.x * frameTime;
	transform->translate.y += velocity_.y * frameTime;
	transform->translate.z += velocity_.z * frameTime;

	// 生存時間の更新
	lifeTime_ += frameTime;
	if(lifeTime_ >= maxLifeTime_) {
		isAlive_ = false;
	}

	// BaseObjectのコライダー位置を更新
	BaseObject::Update(transform->translate);

	// Object3dの更新
	obj_->Update();
}

void PlayerBullet::Draw() {
	if(isAlive_ && obj_) {
		obj_->Draw();
	}
}

Vector3 PlayerBullet::GetPosition() const {
	if(obj_) {
		return obj_->GetPosition();
	}
	return { 0.0f, 0.0f, 0.0f };
}

void PlayerBullet::OnCollisionEnter(BaseObject *other) {
	// 敵との衝突時の処理
	// 弾を削除
	SetDead();
}

void PlayerBullet::OnCollisionStay(BaseObject *other) {
	// 継続中の衝突処理（必要に応じて実装）
}

void PlayerBullet::OnCollisionExit(BaseObject *other) {
	// 衝突終了時の処理（必要に応じて実装）
}

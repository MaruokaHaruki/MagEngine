#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include <algorithm>
#include <cmath>
///=============================================================================
///                        初期化
void Enemy::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	//========================================
	// 3Dオブジェクトの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	//========================================
	// メイントランスフォームの初期設定
	transform_.translate = position;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f}; // プレイヤーの方向を向く（180度回転）

	// Object3dのトランスフォームに反映
	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}

	//========================================
	// 飛行制御の初期化
	currentDirection_ = {0.0f, 0.0f, 1.0f};			// 初期方向（前方）
	targetDirection_ = {0.0f, 0.0f, 1.0f};			// 初期目標方向
	speed_ = 0.0f;									// 敵の移動速度
	currentSpeed_ = 0.0f;							// 初期速度
	maxTurnRate_ = 2.5f;							// 最大旋回速度を上げる（ラジアン/秒）
	hasTarget_ = false;								// 目標位置なし

	//========================================
	// 状態の初期化
	isAlive_ = true; // 生存状態
	radius_ = 1.0f;	 // 当たり判定の半径
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = 2.0f; // 2秒間破壊演出を表示

	//========================================
	// パーティクル関連の初期化
	particle_ = nullptr;
	particleCreated_ = false;

	//========================================
	// BaseObjectの初期化（当たり判定）
	BaseObject::Initialize(transform_.translate, radius_);
}
///=============================================================================
///                        パーティクルシステムの設定
void Enemy::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;
}
///=============================================================================
///                        移動パラメータの設定
void Enemy::SetMovementParams(float speed, const Vector3 &targetPosition) {
	speed_ = speed;
	hasTarget_ = true;

	// 目標方向を計算
	Vector3 direction = {
		targetPosition.x - transform_.translate.x,
		targetPosition.y - transform_.translate.y,
		targetPosition.z - transform_.translate.z};

	float length = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (length > 0.0f) {
		targetDirection_ = {direction.x / length, direction.y / length, direction.z / length};
	}

	currentSpeed_ = speed_ * 0.5f;
}
///=============================================================================
///                        更新
void Enemy::Update() {
	// 死亡状態またはオブジェクトが無効な場合は処理しない
	if (destroyState_ == DestroyState::Dead || !obj_) {
		return;
	}

	// 破壊演出の更新
	if (destroyState_ == DestroyState::Destroying) {
		destroyTimer_ += 1.0f / 60.0f;
		if (destroyTimer_ >= destroyDuration_) {
			destroyState_ = DestroyState::Dead;
			isAlive_ = false;
		}
		return;
	}

	UpdateMovement();
	CheckOutOfBounds();
	BaseObject::Update(transform_.translate);
	obj_->Update();
}
///=============================================================================
///                        通常状態の更新（移動・回転）
void Enemy::UpdateMovement() {
	const float frameTime = 1.0f / 60.0f;

	if (hasTarget_) {
		UpdateFlightDynamics(frameTime);
		currentSpeed_ = std::min(currentSpeed_ + speed_ * frameTime, speed_ * 1.2f);
	} else {
		// 直進
		currentSpeed_ = speed_;
	}

	// 位置更新
	transform_.translate.x += currentDirection_.x * currentSpeed_ * frameTime;
	transform_.translate.y += currentDirection_.y * currentSpeed_ * frameTime;
	transform_.translate.z += currentDirection_.z * currentSpeed_ * frameTime;

	// 機体の向き更新
	if (currentDirection_.x != 0.0f || currentDirection_.z != 0.0f) {
		transform_.rotate.y = atan2f(currentDirection_.x, currentDirection_.z);
	}
	transform_.rotate.x = -asinf(std::max(-1.0f, std::min(1.0f, currentDirection_.y)));

	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}
}
///=============================================================================
///                        飛行力学の更新
void Enemy::UpdateFlightDynamics(float frameTime) {
	// 現在方向から目標方向への補間
	float dotProduct = currentDirection_.x * targetDirection_.x +
					   currentDirection_.y * targetDirection_.y +
					   currentDirection_.z * targetDirection_.z;

	// 角度差が小さい場合は直接設定
	if (dotProduct > 0.98f) {
		currentDirection_ = targetDirection_;
		hasTarget_ = false; // 目標に向いたら直進モードに
		return;
	}

	// 外積で回転軸を計算
	Vector3 crossProduct = {
		currentDirection_.y * targetDirection_.z - currentDirection_.z * targetDirection_.y,
		currentDirection_.z * targetDirection_.x - currentDirection_.x * targetDirection_.z,
		currentDirection_.x * targetDirection_.y - currentDirection_.y * targetDirection_.x};

	float crossLength = sqrtf(crossProduct.x * crossProduct.x +
							  crossProduct.y * crossProduct.y +
							  crossProduct.z * crossProduct.z);

	if (crossLength > 0.001f) {
		float maxRotation = maxTurnRate_ * frameTime;
		float actualRotation = std::min(acosf(std::max(-1.0f, std::min(1.0f, dotProduct))), maxRotation);

		Vector3 axis = {crossProduct.x / crossLength, crossProduct.y / crossLength, crossProduct.z / crossLength};

		// ロドリゲスの回転公式
		float cosAngle = cosf(actualRotation);
		float sinAngle = sinf(actualRotation);
		float dot = axis.x * currentDirection_.x + axis.y * currentDirection_.y + axis.z * currentDirection_.z;

		currentDirection_ = {
			currentDirection_.x * cosAngle + (axis.y * currentDirection_.z - axis.z * currentDirection_.y) * sinAngle + axis.x * dot * (1 - cosAngle),
			currentDirection_.y * cosAngle + (axis.z * currentDirection_.x - axis.x * currentDirection_.z) * sinAngle + axis.y * dot * (1 - cosAngle),
			currentDirection_.z * cosAngle + (axis.x * currentDirection_.y - axis.y * currentDirection_.x) * sinAngle + axis.z * dot * (1 - cosAngle)};
	}
}
///=============================================================================
///                        画面外判定
void Enemy::CheckOutOfBounds() {
	// 画面外に出たら削除（前方に向かった場合も含む）
	if (transform_.translate.z < -20.0f || transform_.translate.z > 30.0f) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
	}
}
///=============================================================================
///                        描画
void Enemy::Draw() {
	// 生存中のみ描画
	if (destroyState_ == DestroyState::Alive && obj_) {
		obj_->Draw();
	}
}
///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	if (!obj_)
		return;

	ImGui::Begin("Enemy Debug");
	ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform_.translate.x, transform_.translate.y, transform_.translate.z);
	ImGui::Text("Is Alive: %s", isAlive_ ? "Yes" : "No");
	ImGui::SliderFloat("Speed", &speed_, 0.5f, 20.0f);
	ImGui::SliderFloat("Turn Rate", &maxTurnRate_, 0.5f, 5.0f);
	ImGui::End();
}
///=============================================================================
///                        位置取得
Vector3 Enemy::GetPosition() const {
	return transform_.translate;
}
///=============================================================================
///                        衝突処理関数
void Enemy::OnCollisionEnter(BaseObject *other) {
	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	//========================================
	// パーティクルエフェクトの生成
	if (particle_ && !particleCreated_) {
		Vector3 enemyPos = transform_.translate; // メイントランスフォームから位置取得

		//========================================
		// 1. 火花エフェクト（Board形状）- メインの爆発
		particle_->SetVelocityRange({-10.0f, -5.0f, -10.0f}, {10.0f, 10.0f, 10.0f});
		particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f}); // オレンジ～黄色
		particle_->SetLifetimeRange(0.5f, 1.5f);
		particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.8f, 0.8f, 0.8f});
		particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f}); // 最小値を0.0fから変更
		particle_->SetGravity({0.0f, -8.0f, 0.0f});
		particle_->SetFadeInOut(0.02f, 0.8f);
		particle_->Emit("ExplosionSparks", enemyPos, 30); // 30個の火花

		particleCreated_ = true;
	}

	//========================================
	// 破壊状態に移行（すぐには消さない）
	destroyState_ = DestroyState::Destroying;
	destroyTimer_ = 0.0f;
}
///=============================================================================
///                        衝突継続処理
void Enemy::OnCollisionStay(BaseObject *other) {
	// 継続中の衝突処理（必要に応じて実装）
}
///=============================================================================
///                        衝突終了処理
void Enemy::OnCollisionExit(BaseObject *other) {
	// 衝突終了時の処理（必要に応じて実装）
}
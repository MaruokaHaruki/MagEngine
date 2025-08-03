#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include <algorithm> // std::min, std::maxのために追加
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
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}

	//========================================
	// 移動パラメータの設定
	speed_ = 0.0f;					   // 敵の移動速度
	velocity_ = {0.0f, 0.0f, -speed_}; // プレイヤーに向かって移動（Z軸負方向）
	rotationSpeed_ = 1.0f;			   // 回転速度（ラジアン/秒）
	hasTarget_ = false;				   // 目標位置なし

	//========================================
	// 戦闘機らしい飛行制御の初期化
	currentDirection_ = {0.0f, 0.0f, 1.0f};			// 初期方向（前方）
	targetDirection_ = {0.0f, 0.0f, 1.0f};			// 初期目標方向
	currentSpeed_ = 0.0f;							// 初期速度
	maxTurnRate_ = 1.5f;							// 最大旋回速度（ラジアン/秒）
	acceleration_ = 8.0f;							// 加速度
	bankingAngle_ = 0.0f;							// バンク角
	maxBankingAngle_ = 45.0f * (3.14159f / 180.0f); // 最大45度のバンク角

	//========================================
	// 行動状態の初期化
	behaviorState_ = BehaviorState::Approaching;
	hoverTime_ = 0.0f;
	maxHoverTime_ = 3.0f + static_cast<float>(rand() % 3); // 3-5秒ランダム
	hoverOffset_ = {0.0f, 0.0f, 0.0f};

	//========================================
	// 状態パラメータの設定
	isAlive_ = true; // 生存状態
	radius_ = 1.0f;	 // 当たり判定の半径

	//========================================
	// パーティクル関連の初期化
	particle_ = nullptr;
	particleSetup_ = nullptr;
	particleCreated_ = false;

	//========================================
	// 破壊演出関連の初期化
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = 2.0f; // 2秒間破壊演出を表示

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
	targetPosition_ = targetPosition;
	hasTarget_ = true;
	behaviorState_ = BehaviorState::Approaching;

	// 目標位置への方向ベクトルを計算
	Vector3 direction = {
		targetPosition_.x - transform_.translate.x,
		targetPosition_.y - transform_.translate.y,
		targetPosition_.z - transform_.translate.z};

	// 正規化
	float length = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (length > 0.0f) {
		direction.x /= length;
		direction.y /= length;
		direction.z /= length;
	}

	// 目標方向を設定（急激な方向転換を避ける）
	targetDirection_ = direction;

	// 初期速度は現在の速度から徐々に変化
	if (currentSpeed_ < 1.0f) {
		currentSpeed_ = speed_ * 0.3f; // 初期は30%の速度から開始
	}
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
		UpdateDestruction();
		return;
	}

	// 通常状態の更新
	if (destroyState_ == DestroyState::Alive) {
		if (hasTarget_) {
			UpdateAIMovement();
		} else {
			UpdateMovement();
		}
		CheckOutOfBounds();

		// BaseObjectのコライダー位置を更新
		BaseObject::Update(transform_.translate);
	}

	// オブジェクトの更新
	UpdateObject();
}
///=============================================================================
///                        破壊演出の更新
void Enemy::UpdateDestruction() {
	const float frameTime = 1.0f / 60.0f;
	destroyTimer_ += frameTime;

	// 破壊演出時間が経過したら完全に消滅
	if (destroyTimer_ >= destroyDuration_) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
	}
}
///=============================================================================
///                        通常状態の更新（移動・回転）
void Enemy::UpdateMovement() {
	const float frameTime = 1.0f / 60.0f;

	// 位置の更新
	transform_.translate.x += velocity_.x * frameTime;
	transform_.translate.y += velocity_.y * frameTime;
	transform_.translate.z += velocity_.z * frameTime;

	// 回転の更新
	transform_.rotate.z += rotationSpeed_ * frameTime;

	// Object3dのトランスフォームに反映
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}
}
///=============================================================================
///                        AI移動の更新
void Enemy::UpdateAIMovement() {
	const float frameTime = 1.0f / 60.0f;

	switch (behaviorState_) {
	case BehaviorState::Approaching:
		UpdateApproaching(frameTime);
		break;
	case BehaviorState::Hovering:
		UpdateHovering(frameTime);
		break;
	case BehaviorState::Attacking:
		// 将来の拡張用
		break;
	}

	// Object3dのトランスフォームに反映
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}
}

void Enemy::UpdateApproaching(float frameTime) {
	// 目標位置への方向を再計算
	Vector3 toTarget = {
		targetPosition_.x - transform_.translate.x,
		targetPosition_.y - transform_.translate.y,
		targetPosition_.z - transform_.translate.z};

	float distanceToTarget = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

	// 目標位置に近づいたらホバリング状態に移行
	if (distanceToTarget < 4.0f) {
		behaviorState_ = BehaviorState::Hovering;
		hoverTime_ = 0.0f;
		hoverOffset_.x = static_cast<float>((rand() % 21) - 10) * 0.15f; // -1.5 ～ 1.5
		hoverOffset_.y = static_cast<float>((rand() % 11) - 5) * 0.1f;	 // -0.5 ～ 0.5
		hoverOffset_.z = static_cast<float>((rand() % 11) - 5) * 0.15f;	 // -0.75 ～ 0.75
		return;
	}

	// 目標方向を更新
	if (distanceToTarget > 0.1f) {
		targetDirection_.x = toTarget.x / distanceToTarget;
		targetDirection_.y = toTarget.y / distanceToTarget;
		targetDirection_.z = toTarget.z / distanceToTarget;
	}

	// 戦闘機らしい飛行制御を更新
	UpdateFlightDynamics(frameTime);

	// 減速処理（目標に近づいたら）
	float targetSpeed = speed_;
	if (distanceToTarget < 8.0f) {
		targetSpeed = speed_ * (distanceToTarget / 8.0f) * 0.5f + speed_ * 0.3f; // 最低30%の速度は維持
	}

	// 速度を滑らかに変更
	currentSpeed_ += (targetSpeed - currentSpeed_) * acceleration_ * frameTime * 0.1f;
	if (currentSpeed_ < 0.1f)
		currentSpeed_ = 0.1f;

	// 位置を更新
	transform_.translate.x += currentDirection_.x * currentSpeed_ * frameTime;
	transform_.translate.y += currentDirection_.y * currentSpeed_ * frameTime;
	transform_.translate.z += currentDirection_.z * currentSpeed_ * frameTime;

	// 機体の向きと傾きを更新
	UpdateAircraftOrientation(frameTime);
}

void Enemy::UpdateHovering(float frameTime) {
	hoverTime_ += frameTime;

	// ホバリング時間が経過したら画面外に向かって移動
	if (hoverTime_ >= maxHoverTime_) {
		behaviorState_ = BehaviorState::Approaching;
		hasTarget_ = false;

		// 前方に向かう方向を設定
		targetDirection_ = {0.0f, 0.0f, 1.0f};
		currentSpeed_ = speed_ * 0.8f; // 離脱時は80%の速度
		return;
	}

	// ホバリング中の目標位置を計算
	float hoverFactor = sinf(hoverTime_ * 1.5f) * 0.7f; // ゆっくりとした周期
	Vector3 hoverTarget;
	hoverTarget.x = targetPosition_.x + hoverOffset_.x * hoverFactor;
	hoverTarget.y = targetPosition_.y + hoverOffset_.y * hoverFactor;
	hoverTarget.z = targetPosition_.z + hoverOffset_.z * hoverFactor;

	// ホバリング位置への方向を計算
	Vector3 toHoverTarget = {
		hoverTarget.x - transform_.translate.x,
		hoverTarget.y - transform_.translate.y,
		hoverTarget.z - transform_.translate.z};

	float distance = sqrtf(toHoverTarget.x * toHoverTarget.x + toHoverTarget.y * toHoverTarget.y + toHoverTarget.z * toHoverTarget.z);

	if (distance > 0.1f) {
		targetDirection_.x = toHoverTarget.x / distance;
		targetDirection_.y = toHoverTarget.y / distance;
		targetDirection_.z = toHoverTarget.z / distance;
	}

	// ホバリング中は低速で移動
	float hoverSpeed = speed_ * 0.3f;
	currentSpeed_ += (hoverSpeed - currentSpeed_) * frameTime * 2.0f;

	// 戦闘機らしい飛行制御を更新
	UpdateFlightDynamics(frameTime);

	// 位置を更新
	transform_.translate.x += currentDirection_.x * currentSpeed_ * frameTime;
	transform_.translate.y += currentDirection_.y * currentSpeed_ * frameTime;
	transform_.translate.z += currentDirection_.z * currentSpeed_ * frameTime;

	// 機体の向きと傾きを更新
	UpdateAircraftOrientation(frameTime);
}

void Enemy::UpdateFlightDynamics(float frameTime) {
	// 現在の方向から目標方向への角度差を計算
	float dotProduct = currentDirection_.x * targetDirection_.x +
					   currentDirection_.y * targetDirection_.y +
					   currentDirection_.z * targetDirection_.z;

	// 角度差が小さい場合は直接設定
	if (dotProduct > 0.99f) {
		currentDirection_ = targetDirection_;
		return;
	}

	// 外積で回転軸を計算
	Vector3 crossProduct = {
		currentDirection_.y * targetDirection_.z - currentDirection_.z * targetDirection_.y,
		currentDirection_.z * targetDirection_.x - currentDirection_.x * targetDirection_.z,
		currentDirection_.x * targetDirection_.y - currentDirection_.y * targetDirection_.x};

	float crossLength = sqrtf(crossProduct.x * crossProduct.x + crossProduct.y * crossProduct.y + crossProduct.z * crossProduct.z);

	if (crossLength > 0.001f) {
		// 最大旋回速度に基づいて回転量を制限
		float maxRotation = maxTurnRate_ * frameTime;
		float actualRotation = std::min(acosf(std::max(-1.0f, std::min(1.0f, dotProduct))), maxRotation);

		// 単位外積ベクトルを計算
		Vector3 axis = {crossProduct.x / crossLength, crossProduct.y / crossLength, crossProduct.z / crossLength};

		// ロドリゲスの回転公式で方向を更新
		float cosAngle = cosf(actualRotation);
		float sinAngle = sinf(actualRotation);

		Vector3 newDirection;
		newDirection.x = currentDirection_.x * cosAngle +
						 (axis.y * currentDirection_.z - axis.z * currentDirection_.y) * sinAngle +
						 axis.x * (axis.x * currentDirection_.x + axis.y * currentDirection_.y + axis.z * currentDirection_.z) * (1 - cosAngle);
		newDirection.y = currentDirection_.y * cosAngle +
						 (axis.z * currentDirection_.x - axis.x * currentDirection_.z) * sinAngle +
						 axis.y * (axis.x * currentDirection_.x + axis.y * currentDirection_.y + axis.z * currentDirection_.z) * (1 - cosAngle);
		newDirection.z = currentDirection_.z * cosAngle +
						 (axis.x * currentDirection_.y - axis.y * currentDirection_.x) * sinAngle +
						 axis.z * (axis.x * currentDirection_.x + axis.y * currentDirection_.y + axis.z * currentDirection_.z) * (1 - cosAngle);

		currentDirection_ = newDirection;

		// バンク角を計算（旋回時の傾き）
		float turnIntensity = actualRotation / maxRotation;
		float targetBanking = 0.0f;

		// Y軸周りの回転成分でバンク方向を決定
		if (crossProduct.y > 0.001f) {
			targetBanking = -maxBankingAngle_ * turnIntensity; // 左旋回時は右バンク
		} else if (crossProduct.y < -0.001f) {
			targetBanking = maxBankingAngle_ * turnIntensity; // 右旋回時は左バンク
		}

		// バンク角を滑らかに変更
		bankingAngle_ += (targetBanking - bankingAngle_) * frameTime * 3.0f;
	} else {
		// バンク角を0に戻す
		bankingAngle_ += (0.0f - bankingAngle_) * frameTime * 2.0f;
	}
}

void Enemy::UpdateAircraftOrientation(float frameTime) {
	// 機体の向きを飛行方向に設定
	if (currentDirection_.x != 0.0f || currentDirection_.z != 0.0f) {
		transform_.rotate.y = atan2f(currentDirection_.x, currentDirection_.z);
	}

	// ピッチ角を計算（上昇・下降）
	float pitch = -asinf(std::max(-1.0f, std::min(1.0f, currentDirection_.y)));
	transform_.rotate.x = pitch;

	// バンク角を適用
	transform_.rotate.z = bankingAngle_;
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
///                        オブジェクトの更新
void Enemy::UpdateObject() {
	obj_->Update();
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
	if (!obj_) {
		return;
	}

	ImGui::Begin("Enemy Debug");
	ImGui::Text("Position: (%.2f, %.2f, %.2f)",
				transform_.translate.x, transform_.translate.y, transform_.translate.z);
	ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
	ImGui::Text("Is Alive: %s", isAlive_ ? "Yes" : "No");
	ImGui::SliderFloat("Speed", &speed_, 0.5f, 10.0f);
	ImGui::SliderFloat("Rotation Speed", &rotationSpeed_, 0.1f, 5.0f);
	ImGui::SliderFloat("Radius", &radius_, 0.5f, 3.0f);
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
		particle_->SetTranslateRange({-0.2f, -0.2f, -0.2f}, {0.2f, 0.2f, 0.2f});
		particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f}); // オレンジ～黄色
		particle_->SetLifetimeRange(0.5f, 1.5f);
		particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.8f, 0.8f, 0.8f});
		particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f}); // 最小値を0.0fから変更
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {3.14f, 3.14f, 3.14f});
		particle_->SetEndRotationRange({3.14f, 3.14f, 3.14f}, {6.28f, 6.28f, 6.28f}); // min < max確保
		particle_->SetGravity({0.0f, -8.0f, 0.0f});
		particle_->SetFadeInOut(0.02f, 0.8f);
		particle_->Emit("ExplosionSparks", enemyPos, 30); // 30個の火花

		//========================================
		// 2. 衝撃波エフェクト（Ring形状）
		particle_->SetVelocityRange({-2.0f, -1.0f, -2.0f}, {2.0f, 1.0f, 2.0f});
		particle_->SetTranslateRange({-0.1f, -0.1f, -0.1f}, {0.1f, 0.1f, 0.1f});
		particle_->SetColorRange({1.0f, 0.8f, 0.4f, 0.6f}, {1.0f, 1.0f, 0.8f, 0.8f}); // alpha値調整
		particle_->SetLifetimeRange(0.8f, 1.2f);
		particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
		particle_->SetEndScaleRange({3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f}); // 大きく広がる
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {3.14f, 3.14f, 3.14f});
		particle_->SetEndRotationRange({3.14f, 3.14f, 3.14f}, {6.28f, 6.28f, 6.28f}); // min < max確保
		particle_->SetGravity({0.0f, 0.0f, 0.0f});									  // 重力なし
		particle_->SetFadeInOut(0.1f, 0.6f);
		particle_->Emit("ExplosionRing", enemyPos, 3); // 3つの衝撃波リング

		//========================================
		// 3. 煙柱エフェクト（Cylinder形状）
		particle_->SetVelocityRange({-3.0f, 2.0f, -3.0f}, {3.0f, 8.0f, 3.0f}); // 上向きに強く
		particle_->SetTranslateRange({-0.3f, 0.0f, -0.3f}, {0.3f, 0.5f, 0.3f});
		particle_->SetColorRange({0.4f, 0.4f, 0.4f, 0.3f}, {0.8f, 0.8f, 0.8f, 0.6f}); // alpha値調整
		particle_->SetLifetimeRange(1.5f, 3.0f);									  // 長く残る
		particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
		particle_->SetEndScaleRange({1.5f, 2.0f, 1.5f}, {2.5f, 3.0f, 2.5f}); // 徐々に大きく
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {1.57f, 1.57f, 1.57f});
		particle_->SetEndRotationRange({1.57f, 1.57f, 1.57f}, {4.71f, 4.71f, 4.71f}); // min < max確保
		particle_->SetGravity({0.0f, -1.0f, 0.0f});									  // 軽い重力
		particle_->SetFadeInOut(0.2f, 0.7f);
		particle_->Emit("ExplosionSmoke", enemyPos, 8); // 8個の煙柱

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

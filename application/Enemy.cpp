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
	currentVelocity_ = {0.0f, 0.0f, 0.0f};
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	targetRotationEuler_ = {0.0f, 0.0f, 0.0f};
	acceleration_ = 8.0f;					  // 加速度
	rotationSmoothing_ = 8.0f;				  // 回転の滑らかさ
	maxRollAngle_ = 45.0f * (static_cast<float>(M_PI) / 180.0f);  // 最大ロール角（45度）
	maxPitchAngle_ = 30.0f * (static_cast<float>(M_PI) / 180.0f); // 最大ピッチ角（30度）
	bankingFactor_ = 1.5f;					  // バンキング係数

	//========================================
	// ステートマシンの初期化
	currentState_ = FlightState::Spawn;
	spawnPosition_ = position;
	stateTimer_ = 0.0f;

	// 初期方向を前方（+Z）に設定
	currentDirection_ = {0.0f, 0.0f, 1.0f};
	targetDirection_ = {0.0f, 0.0f, 1.0f};

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
	targetPosition_ = targetPosition;

	// MoveToTargetステートに移行
	if (currentState_ == FlightState::Spawn) {
		currentState_ = FlightState::MoveToTarget;
		stateTimer_ = 0.0f;
	}
}
///=============================================================================
///                        AI行動の開始
void Enemy::StartAIBehavior() {
	currentState_ = FlightState::AIBehavior;
	stateTimer_ = 0.0f;
}
///=============================================================================
///                        離脱の開始
void Enemy::StartDisengagement() {
	currentState_ = FlightState::Disengagement;
	stateTimer_ = 0.0f;

	// 離脱目標位置を設定（+Z方向の画面奥に向かう）
	disengageTarget_ = transform_.translate;
	disengageTarget_.z += 50.0f; // +Z方向に50ユニット先
}
///=============================================================================
///                        移動方向の設定（新規追加）
void Enemy::SetMovementDirection(float speed, const Vector3 &direction) {
	speed_ = speed;

	// 方向を正規化
	float length = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (length > 0.001f) {
		float invLength = 1.0f / length;
		currentDirection_ = {
			direction.x * invLength,
			direction.y * invLength,
			direction.z * invLength};
		targetDirection_ = currentDirection_;
	}

	// 直進飛行ステートに設定
	currentState_ = FlightState::StraightFlight;
	stateTimer_ = 0.0f;
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

	const float frameTime = 1.0f / 60.0f;
	stateTimer_ += frameTime;

	// ステート別の更新
	switch (currentState_) {
	case FlightState::Spawn:
		UpdateSpawnState(frameTime);
		break;
	case FlightState::StraightFlight:
		UpdateStraightFlightState(frameTime);
		break;
	case FlightState::MoveToTarget:
		UpdateMoveToTargetState(frameTime);
		break;
	case FlightState::AIBehavior:
		UpdateAIBehaviorState(frameTime);
		break;
	case FlightState::Disengagement:
		UpdateDisengagementState(frameTime);
		break;
	}

	UpdateFlightControl(frameTime);
	CalculatePitchAndRoll();
	CheckOutOfBounds();
	BaseObject::Update(transform_.translate);
	obj_->Update();
}
///=============================================================================
///                        ステート別更新：スポーン
void Enemy::UpdateSpawnState(float frameTime) {
	// FIXME: frameTimeが使用されていないので回避すること
	frameTime;
	// スポーン直後は+Z方向に直進
	targetVelocity_ = {0.0f, 0.0f, speed_};

	// 一定時間後に自動的にMoveToTargetに移行（設定されていない場合）
	if (stateTimer_ > 1.0f && currentState_ == FlightState::Spawn) {
		StartDisengagement(); // 目標が設定されていない場合は離脱
	}
}
///=============================================================================
///                        ステート別更新：直進飛行（新規追加）
void Enemy::UpdateStraightFlightState(float frameTime) {
	// FIXME: frameTimeが使用されていないので回避すること
	frameTime;
	// 常に設定された方向に直進
	targetVelocity_ = {
		currentDirection_.x * speed_,
		currentDirection_.y * speed_,
		currentDirection_.z * speed_};

	// 機体の向きを飛行方向に合わせる
	targetDirection_ = currentDirection_;
}
///=============================================================================
///                        ステート別更新：目標位置への移動
void Enemy::UpdateMoveToTargetState(float frameTime) {
	// FIXME: frameTimeが使用されていないので回避すること
	frameTime;
	Vector3 direction = {
		targetPosition_.x - transform_.translate.x,
		targetPosition_.y - transform_.translate.y,
		targetPosition_.z - transform_.translate.z};

	float distance = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	if (distance > 1.0f) {
		// 正規化した方向に速度を設定
		float invLength = 1.0f / distance;
		targetVelocity_ = {
			direction.x * invLength * speed_,
			direction.y * invLength * speed_,
			direction.z * invLength * speed_};
	} else {
		// 目標に到達したらAI行動に移行
		StartAIBehavior();
	}
}
///=============================================================================
///                        ステート別更新：AI挙動
void Enemy::UpdateAIBehaviorState(float frameTime) {
	// FIXME: frameTimeが使用されていないので回避すること
	frameTime;
	// 現在は簡単な円運動を実装（将来的にはより複雑なAIに置き換え）
	// float circleRadius = 5.0f;
	// float angularSpeed = 1.0f;
	// float angle = stateTimer_ * angularSpeed;

	// Vector3 circleCenter = targetPosition_;
	// targetVelocity_ = {
	//	cosf(angle + M_PI * 0.5f) * angularSpeed * circleRadius,
	//	0.0f,
	//	sinf(angle + M_PI * 0.5f) * angularSpeed * circleRadius};

	// 一定時間後に離脱
	if (stateTimer_ > 5.0f) {
		StartDisengagement();
	}
}
///=============================================================================
///                        ステート別更新：離脱
void Enemy::UpdateDisengagementState(float frameTime) {
	// FIXME: frameTimeが使用されていないので回避すること
	frameTime;
	Vector3 direction = {
		disengageTarget_.x - transform_.translate.x,
		disengageTarget_.y - transform_.translate.y,
		disengageTarget_.z - transform_.translate.z};

	float distance = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	if (distance > 1.0f) {
		float invLength = 1.0f / distance;
		targetVelocity_ = {
			direction.x * invLength * speed_ * 1.5f, // 離脱時は高速
			direction.y * invLength * speed_ * 1.5f,
			direction.z * invLength * speed_ * 1.5f};
	}
}
///=============================================================================
///                        飛行制御の更新
void Enemy::UpdateFlightControl(float frameTime) {
	// 速度の補間（Playerと同様）
	currentVelocity_.x += (targetVelocity_.x - currentVelocity_.x) * acceleration_ * frameTime;
	currentVelocity_.y += (targetVelocity_.y - currentVelocity_.y) * acceleration_ * frameTime;
	currentVelocity_.z += (targetVelocity_.z - currentVelocity_.z) * acceleration_ * frameTime;

	// 位置更新
	transform_.translate.x += currentVelocity_.x * frameTime;
	transform_.translate.y += currentVelocity_.y * frameTime;
	transform_.translate.z += currentVelocity_.z * frameTime;
}
///=============================================================================
///                        ピッチとロールの計算
void Enemy::CalculatePitchAndRoll() {
	// 飛行方向から機体の向きを計算（+Z方向を基本とする）
	Vector3 forwardDir = targetDirection_;

	// ヨー角（Y軸回転）を計算 - +Z方向を0度とする
	targetRotationEuler_.y = atan2f(forwardDir.x, forwardDir.z);

	// ピッチ角（X軸回転）を計算
	float horizontalDistance = sqrtf(forwardDir.x * forwardDir.x + forwardDir.z * forwardDir.z);
	if (horizontalDistance > 0.001f) {
		float pitchAngle = atan2f(forwardDir.y, horizontalDistance);
		targetRotationEuler_.x = std::max(-maxPitchAngle_, std::min(maxPitchAngle_, pitchAngle));
	}

	// ロール角（Z軸回転）- 旋回時のバンキング
	Vector3 velocityChange = {
		targetVelocity_.x - currentVelocity_.x,
		targetVelocity_.y - currentVelocity_.y,
		targetVelocity_.z - currentVelocity_.z};

	float lateralAccel = velocityChange.x;
	float rollAngle = lateralAccel * bankingFactor_;
	targetRotationEuler_.z = std::max(-maxRollAngle_, std::min(maxRollAngle_, rollAngle));

	// 回転の補間
	const float frameTime = 1.0f / 60.0f;
	transform_.rotate.x += (targetRotationEuler_.x - transform_.rotate.x) * rotationSmoothing_ * frameTime;
	transform_.rotate.y += (targetRotationEuler_.y - transform_.rotate.y) * rotationSmoothing_ * frameTime;
	transform_.rotate.z += (targetRotationEuler_.z - transform_.rotate.z) * rotationSmoothing_ * frameTime;

	// Object3dのトランスフォームに反映
	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}
}
///=============================================================================
///                        画面外判定
void Enemy::CheckOutOfBounds() {
	// 画面外に出たら削除（前方に向かった場合も含む）
	if (transform_.translate.z < -100.0f || transform_.translate.z > 100.0f) {
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
	// FIXME: otherが使用されていないので修正すること
	other;

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
	// FIXME: otherが使用されていないので修正すること
	other;
	// 継続中の衝突処理（必要に応じて実装）
}
///=============================================================================
///                        衝突終了処理
void Enemy::OnCollisionExit(BaseObject *other) {
	// FIXME: otherが使用されていないので修正すること
	other;
	// 衝突終了時の処理（必要に応じて実装）
}
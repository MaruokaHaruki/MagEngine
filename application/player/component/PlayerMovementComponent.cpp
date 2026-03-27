#define _USE_MATH_DEFINES
#include "PlayerMovementComponent.h"
#include "PlayerConstants.h"
#include "Transform.h"
#include <cmath>

//=============================================================================
// 初期化
void PlayerMovementComponent::Initialize() {
	const Vector3 zero{};
	currentVelocity_ = zero;
	targetVelocity_ = zero;
	targetRotationEuler_ = zero;

	//========================================
	//          移動パラメータ
	//========================================
	moveSpeed_ = PlayerConstants::Movement::DEFAULT_MOVE_SPEED;
	acceleration_ = PlayerConstants::Movement::DEFAULT_ACCELERATION;
	rotationSmoothing_ = PlayerConstants::Movement::DEFAULT_ROTATION_SMOOTHING;
	maxRollAngle_ = PlayerConstants::Movement::MAX_ROLL_ANGLE;
	maxPitchAngle_ = PlayerConstants::Movement::MAX_PITCH_ANGLE;

	//========================================
	//          ブースト関連の初期化
	//========================================
	maxBoostGauge_ = PlayerConstants::Boost::MAX_GAUGE;
	boostGauge_ = maxBoostGauge_;
	boostSpeed_ = PlayerConstants::Boost::SPEED_MULTIPLIER;
	boostConsumption_ = PlayerConstants::Boost::CONSUMPTION_RATE;
	boostRecovery_ = PlayerConstants::Boost::RECOVERY_RATE;
	isBoosting_ = false;

	//========================================
	//          バレルロール関連の初期化
	//========================================
	isBarrelRolling_ = false;
	barrelRollTime_ = 0.0f;
	barrelRollDuration_ = PlayerConstants::BarrelRoll::DURATION;
	barrelRollCooldown_ = PlayerConstants::BarrelRoll::COOLDOWN;
	barrelRollCoolTimer_ = 0.0f;
	barrelRollCost_ = PlayerConstants::BarrelRoll::COST;
	barrelRollDirection_ = true;
	barrelRollStartRotation_ = zero;
	barrelRollStartVelocity_ = zero;
	barrelRollAcceleration_ = PlayerConstants::BarrelRoll::ACCELERATION_MULTIPLIER;
}

//=============================================================================
// 更新
void PlayerMovementComponent::Update(Transform *transform, float deltaTime) {
	if (!transform) {
		return;
	}

	// バレルロール中の処理
	if (isBarrelRolling_) {
		UpdateBarrelRoll(transform, deltaTime);
		ApplyMovement(transform, deltaTime);
		return;
	}

	// クールダウンタイマーの更新
	if (barrelRollCoolTimer_ > 0.0f) {
		barrelRollCoolTimer_ -= deltaTime;
	}

	UpdateVelocity();
	ApplyMovement(transform, deltaTime);
	ApplyRotation(transform);
}

//=============================================================================
// 入力処理
void PlayerMovementComponent::ProcessInput(float inputX, float inputY) {
	// バレルロール中は入力を受け付けない
	if (isBarrelRolling_) {
		return;
	}

	// デッドゾーン処理
	if (std::fabs(inputX) < PlayerConstants::STICK_DEADZONE) {
		inputX = 0.0f;
	}
	if (std::fabs(inputY) < PlayerConstants::STICK_DEADZONE) {
		inputY = 0.0f;
	}

	// ブースト中は速度倍率を適用
	float speedMultiplier = isBoosting_ ? boostSpeed_ : 1.0f;

	// 目標速度の設定
	targetVelocity_.x = inputX * moveSpeed_ * speedMultiplier;
	targetVelocity_.y = inputY * moveSpeed_ * speedMultiplier;
	targetVelocity_.z = 0.0f;

	// 目標回転の更新
	UpdateTargetRotation(inputX, inputY);
}

//=============================================================================
// バレルロール開始
void PlayerMovementComponent::StartBarrelRoll(bool isRight) {
	// ゲージ不足、クールダウン中、すでにバレルロール中は開始できない
	if (!CanBarrelRoll() || isBarrelRolling_) {
		return;
	}

	// ゲージを消費
	boostGauge_ -= barrelRollCost_;
	if (boostGauge_ < 0.0f) {
		boostGauge_ = 0.0f;
	}

	isBarrelRolling_ = true;
	barrelRollTime_ = 0.0f;
	barrelRollDirection_ = isRight;
	barrelRollStartRotation_ = targetRotationEuler_;
	barrelRollStartVelocity_ = currentVelocity_; // 進行方向を記録
	barrelRollCoolTimer_ = barrelRollCooldown_;
}

//=============================================================================
// 移動状況に応じた適応的バレルロール開始
void PlayerMovementComponent::StartAdaptiveBarrelRoll() {
	// ゲージ不足、クールダウン中、すでにバレルロール中は開始できない
	if (!CanBarrelRoll() || isBarrelRolling_) {
		return;
	}

	// ゲージを消費
	boostGauge_ -= barrelRollCost_;
	if (boostGauge_ < 0.0f) {
		boostGauge_ = 0.0f;
	}

	isBarrelRolling_ = true;
	barrelRollTime_ = 0.0f;
	
	// 現在の速度から移動状況を判定
	float speedMagnitude = std::sqrt(
		currentVelocity_.x * currentVelocity_.x +
		currentVelocity_.y * currentVelocity_.y +
		currentVelocity_.z * currentVelocity_.z);
	
	// 移動速度がほぼ無い場合はその場回避（右回転）
	if (speedMagnitude < 0.5f) {
		barrelRollDirection_ = true; // その場回避は右回転
		barrelRollStartVelocity_ = {}; // 速度なし
	} else {
		// 移動方向を判定して回転方向を決定
		// X方向の移動が大きい場合、その方向に回転
		if (std::abs(currentVelocity_.x) > std::abs(currentVelocity_.y)) {
			barrelRollDirection_ = currentVelocity_.x > 0.0f; // 右移動なら右回転
		} else {
			// Y方向の移動が大きい場合、または前方移動の場合は右回転
			barrelRollDirection_ = true;
		}
		barrelRollStartVelocity_ = currentVelocity_; // 現在の移動方向を記録
	}
	
	barrelRollStartRotation_ = targetRotationEuler_;
	barrelRollCoolTimer_ = barrelRollCooldown_;
}

//=============================================================================
// バレルロール更新
void PlayerMovementComponent::UpdateBarrelRoll(MagMath::Transform *transform, float deltaTime) {
	if (!transform) {
		return;
	}

	barrelRollTime_ += deltaTime;
	float progress = barrelRollTime_ / barrelRollDuration_;

	if (progress >= 1.0f) {
		// バレルロール終了
		isBarrelRolling_ = false;
		barrelRollTime_ = 0.0f;
		targetRotationEuler_ = barrelRollStartRotation_;
		// 速度は保持させる（慣性のためそのままにする）
		return;
	}

	// イージング（EaseOutQuad + オーバーシュート気味）- より滑らかでリズミカルな加速
	float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress); // EaseOutQuad
	// さらにオーバーシュート要素を追加（最初は控えめ、中盤で加速）
	if (progress < 0.7f) {
		easedProgress = progress < 0.5f
							? 2.0f * progress * progress
							: 1.0f - (-2.0f * progress + 2.0f) * (-2.0f * progress + 2.0f) / 2.0f;
	}

	// ロール回転（360度 = 1回転）- 元の姿勢に戻る
	float rollDirection = barrelRollDirection_ ? -1.0f : 1.0f;	 // 右=負の回転、左=正の回転
	float rollAngle = easedProgress * 2.0f * PI * rollDirection; // 2π（360度）
	transform->rotate.z = barrelRollStartRotation_.z + rollAngle;

	// 進行方向への加速処理 - より指数的でダイナミック
	// バレルロール開始時の速度がある場合、その方向へ加速
	float speedMagnitude = std::sqrt(
		barrelRollStartVelocity_.x * barrelRollStartVelocity_.x +
		barrelRollStartVelocity_.z * barrelRollStartVelocity_.z);

	if (speedMagnitude > 0.001f) {
		// 進行方向の正規化
		Vector3 direction = {
			barrelRollStartVelocity_.x / speedMagnitude,
			0.0f,
			barrelRollStartVelocity_.z / speedMagnitude};

		// より強い加速（最大で3倍速）- ダイナミックな加速感
		float speedBoost = 1.0f + (barrelRollAcceleration_ - 1.0f) * easedProgress * easedProgress * 1.5f;
		float acceleratedSpeed = speedMagnitude * speedBoost;
		currentVelocity_ = {
			direction.x * acceleratedSpeed,
			0.0f,
			direction.z * acceleratedSpeed};
	} else {
		// 速度がない場合は前方へ加速
		float speedBoost = 1.0f + (barrelRollAcceleration_ - 1.0f) * easedProgress * easedProgress * 1.5f;
		float acceleratedSpeed = moveSpeed_ * speedBoost;
		currentVelocity_ = {0.0f, 0.0f, acceleratedSpeed};
	}
}

//=============================================================================
// ブースト処理
void PlayerMovementComponent::ProcessBoost(bool boostInput, float deltaTime) {
	// バレルロール中はブースト不可
	if (isBarrelRolling_) {
		isBoosting_ = false;
		return;
	}

	if (boostInput && boostGauge_ > 0.0f) {
		// ブースト中
		isBoosting_ = true;
		boostGauge_ -= boostConsumption_ * deltaTime;
		if (boostGauge_ < 0.0f) {
			boostGauge_ = 0.0f;
		}
	} else {
		// ブースト終了・回復
		isBoosting_ = false;
		// バレルロール中でなければゲージ回復
		if (!isBarrelRolling_ && boostGauge_ < maxBoostGauge_) {
			boostGauge_ += boostRecovery_ * deltaTime;
			if (boostGauge_ > maxBoostGauge_) {
				boostGauge_ = maxBoostGauge_;
			}
		}
	}
}

//=============================================================================
// 速度の更新
void PlayerMovementComponent::UpdateVelocity() {
	// 現在の速度を目標速度に滑らかに近づける
	currentVelocity_.x = Lerp(currentVelocity_.x, targetVelocity_.x, acceleration_);
	currentVelocity_.y = Lerp(currentVelocity_.y, targetVelocity_.y, acceleration_);
	currentVelocity_.z = Lerp(currentVelocity_.z, targetVelocity_.z, acceleration_);
}

//=============================================================================
// 目標回転の更新
void PlayerMovementComponent::UpdateTargetRotation(float inputX, float inputY) {
	Vector3 desiredRotationEuler = {
		MagMath::DegreesToRadians(-maxPitchAngle_ * inputY),
		0.0f,
		MagMath::DegreesToRadians(-maxRollAngle_ * inputX)};

	targetRotationEuler_.x = Lerp(targetRotationEuler_.x, desiredRotationEuler.x, rotationSmoothing_);
	targetRotationEuler_.y = Lerp(targetRotationEuler_.y, desiredRotationEuler.y, rotationSmoothing_);
	targetRotationEuler_.z = Lerp(targetRotationEuler_.z, desiredRotationEuler.z, rotationSmoothing_);
}

//=============================================================================
// 移動の適用
void PlayerMovementComponent::ApplyMovement(Transform *transform, float deltaTime) {
	if (!transform) {
		return;
	}

	transform->translate.x += currentVelocity_.x * deltaTime;
	transform->translate.y += currentVelocity_.y * deltaTime;
	transform->translate.z += currentVelocity_.z * deltaTime;
}

//=============================================================================
// 回転の適用
void PlayerMovementComponent::ApplyRotation(Transform *transform) {
	if (!transform) {
		return;
	}

	transform->rotate.x = targetRotationEuler_.x;
	transform->rotate.y = targetRotationEuler_.y;
	transform->rotate.z = targetRotationEuler_.z;
}

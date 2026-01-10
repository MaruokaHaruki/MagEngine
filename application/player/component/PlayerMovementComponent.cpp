#define _USE_MATH_DEFINES
#include "PlayerMovementComponent.h"
#include "Transform.h"
#include <cmath>

//=============================================================================
// 初期化
void PlayerMovementComponent::Initialize() {
	const Vector3 zero{};
	currentVelocity_ = zero;
	targetVelocity_ = zero;
	targetRotationEuler_ = zero;

	moveSpeed_ = 5.0f;
	acceleration_ = 0.1f;
	rotationSmoothing_ = 0.1f;
	maxRollAngle_ = 30.0f;
	maxPitchAngle_ = 15.0f;

	// ブースト関連の初期化
	maxBoostGauge_ = 100.0f;
	boostGauge_ = maxBoostGauge_;
	boostSpeed_ = 2.0f;
	boostConsumption_ = 30.0f;
	boostRecovery_ = 15.0f;
	isBoosting_ = false;

	// バレルロール関連の初期化
	isBarrelRolling_ = false;
	barrelRollTime_ = 0.0f;
	barrelRollDuration_ = 0.8f;
	barrelRollCooldown_ = 1.5f;
	barrelRollCoolTimer_ = 0.0f;
	barrelRollCost_ = 30.0f; // バレルロール1回で30ゲージ消費
	barrelRollDirection_ = true;
	barrelRollStartRotation_ = zero;
	barrelRollMovementOffset_ = zero;
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

	const float deadZone = 0.1f;

	// デッドゾーン処理
	if (std::fabs(inputX) < deadZone) {
		inputX = 0.0f;
	}
	if (std::fabs(inputY) < deadZone) {
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
	barrelRollCoolTimer_ = barrelRollCooldown_;

	// 横移動オフセットを設定（回避効果）- 元の方向に戻す
	float offsetDirection = isRight ? 1.0f : -1.0f; // 右バレルロール=右に移動
	barrelRollMovementOffset_ = {offsetDirection * moveSpeed_ * 2.0f, 0.0f, 0.0f};
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
		barrelRollMovementOffset_ = {0.0f, 0.0f, 0.0f};
		return;
	}

	// イージング（EaseInOutCubic）
	float easedProgress = progress < 0.5f
							  ? 4.0f * progress * progress * progress
							  : 1.0f - std::pow(-2.0f * progress + 2.0f, 3.0f) / 2.0f;

	// ロール回転（360度）- 回転方向のみ逆に変更
	float rollDirection = barrelRollDirection_ ? -1.0f : 1.0f; // 右=負の回転、左=正の回転
	float rollAngle = easedProgress * 2.0f * PI * rollDirection;
	transform->rotate.z = barrelRollStartRotation_.z + rollAngle;

	// 横移動の適用（方向は変更なし）
	currentVelocity_ = barrelRollMovementOffset_;
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

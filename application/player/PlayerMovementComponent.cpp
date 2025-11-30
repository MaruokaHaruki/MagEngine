#define _USE_MATH_DEFINES
#include "PlayerMovementComponent.h"
#include "Transform.h"
#include <cmath>

namespace {
	const float PI = 3.1415926535f;

	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}

	inline float DegreesToRadians(float degrees) {
		return degrees * (PI / 180.0f);
	}
} // namespace

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
}

//=============================================================================
// 更新
void PlayerMovementComponent::Update(Transform *transform, float deltaTime) {
	if (!transform) {
		return;
	}

	UpdateVelocity();
	ApplyMovement(transform, deltaTime);
	ApplyRotation(transform);
}

//=============================================================================
// 入力処理
void PlayerMovementComponent::ProcessInput(float inputX, float inputY) {
	const float deadZone = 0.1f;

	// デッドゾーン処理
	if (std::fabs(inputX) < deadZone) {
		inputX = 0.0f;
	}
	if (std::fabs(inputY) < deadZone) {
		inputY = 0.0f;
	}

	// 目標速度の設定
	targetVelocity_.x = inputX * moveSpeed_;
	targetVelocity_.y = inputY * moveSpeed_;
	targetVelocity_.z = 0.0f;

	// 目標回転の更新
	UpdateTargetRotation(inputX, inputY);
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
		DegreesToRadians(-maxPitchAngle_ * inputY),
		0.0f,
		DegreesToRadians(-maxRollAngle_ * inputX)};

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

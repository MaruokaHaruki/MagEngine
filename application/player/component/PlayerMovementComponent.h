#pragma once
#include "Vector3.h"

// 前方宣言
struct Transform;

///=============================================================================
///						移動管理コンポーネント
class PlayerMovementComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize();
	void Update(Transform *transform, float deltaTime);

	///--------------------------------------------------------------
	///                        移動処理
	void ProcessInput(float inputX, float inputY);
	void ApplyMovement(Transform *transform, float deltaTime);
	void ApplyRotation(Transform *transform);

	///--------------------------------------------------------------
	///                        ゲッター
	const Vector3 &GetCurrentVelocity() const {
		return currentVelocity_;
	}
	const Vector3 &GetTargetVelocity() const {
		return targetVelocity_;
	}
	const Vector3 &GetTargetRotation() const {
		return targetRotationEuler_;
	}
	float GetMoveSpeed() const {
		return moveSpeed_;
	}
	float GetAcceleration() const {
		return acceleration_;
	}

	///--------------------------------------------------------------
	///                        セッター
	void SetMoveSpeed(float speed) {
		moveSpeed_ = speed;
	}
	void SetAcceleration(float accel) {
		acceleration_ = accel;
	}
	void SetRotationSmoothing(float smoothing) {
		rotationSmoothing_ = smoothing;
	}
	void SetMaxRollAngle(float angle) {
		maxRollAngle_ = angle;
	}
	void SetMaxPitchAngle(float angle) {
		maxPitchAngle_ = angle;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	void UpdateVelocity();
	void UpdateTargetRotation(float inputX, float inputY);

	///--------------------------------------------------------------
	///                        メンバ変数
	Vector3 currentVelocity_;	  // 現在の移動速度
	Vector3 targetVelocity_;	  // 目標移動速度
	Vector3 targetRotationEuler_; // 目標回転角度（オイラー角）

	float moveSpeed_;		  // 基本移動速度
	float acceleration_;	  // 加速度（速度変化の滑らかさ）
	float rotationSmoothing_; // 回転の滑らかさ
	float maxRollAngle_;	  // 最大ロール角（度）
	float maxPitchAngle_;	  // 最大ピッチ角（度）
};

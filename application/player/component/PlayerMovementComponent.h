#pragma once
#include "MagMath.h"
using namespace MagMath;

///=============================================================================
///						移動管理コンポーネント
class PlayerMovementComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize();
	void Update(MagMath::Transform *transform, float deltaTime);

	///--------------------------------------------------------------
	///                        移動処理
	void ProcessInput(float inputX, float inputY);
	void ApplyMovement(MagMath::Transform *transform, float deltaTime);
	void ApplyRotation(MagMath::Transform *transform);

	///--------------------------------------------------------------
	///                        バレルロール処理
	void StartBarrelRoll(bool isRight);
	void UpdateBarrelRoll(MagMath::Transform *transform, float deltaTime);
	bool IsBarrelRolling() const {
		return isBarrelRolling_;
	}
	float GetBarrelRollProgress() const {
		return barrelRollTime_ / barrelRollDuration_;
	}
	bool CanBarrelRoll() const {
		return boostGauge_ >= barrelRollCost_ && barrelRollCoolTimer_ <= 0.0f;
	}

	///--------------------------------------------------------------
	///                        ブースト処理
	void ProcessBoost(bool boostInput, float deltaTime);
	bool CanBoost() const {
		return boostGauge_ > 0.0f && !isBarrelRolling_;
	}
	bool IsBoosting() const {
		return isBoosting_;
	}
	float GetBoostGauge() const {
		return boostGauge_;
	}
	float GetMaxBoostGauge() const {
		return maxBoostGauge_;
	}
	float GetBoostGaugeRatio() const {
		return boostGauge_ / maxBoostGauge_;
	}

	///--------------------------------------------------------------
	///                        ゲッター
	const MagMath::Vector3 &GetCurrentVelocity() const {
		return currentVelocity_;
	}
	const MagMath::Vector3 &GetTargetVelocity() const {
		return targetVelocity_;
	}
	const MagMath::Vector3 &GetTargetRotation() const {
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
	void SetBoostSpeed(float speed) {
		boostSpeed_ = speed;
	}
	void SetBoostConsumption(float consumption) {
		boostConsumption_ = consumption;
	}
	void SetBoostRecovery(float recovery) {
		boostRecovery_ = recovery;
	}
	void SetBarrelRollDuration(float duration) {
		barrelRollDuration_ = duration;
	}
	void SetBarrelRollCooldown(float cooldown) {
		barrelRollCooldown_ = cooldown;
	}
	void SetBarrelRollCost(float cost) {
		barrelRollCost_ = cost;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	void UpdateVelocity();
	void UpdateTargetRotation(float inputX, float inputY);

	///--------------------------------------------------------------
	///                        メンバ変数
	MagMath::Vector3 currentVelocity_;	  // 現在の移動速度
	MagMath::Vector3 targetVelocity_;	  // 目標移動速度
	MagMath::Vector3 targetRotationEuler_; // 目標回転角度（オイラー角）

	float moveSpeed_;		  // 基本移動速度
	float acceleration_;	  // 加速度（速度変化の滑らかさ）
	float rotationSmoothing_; // 回転の滑らかさ
	float maxRollAngle_;	  // 最大ロール角（度）
	float maxPitchAngle_;	  // 最大ピッチ角（度）

	///--------------------------------------------------------------
	///                        ブースト関連
	float boostGauge_;		 // 現在のブーストゲージ
	float maxBoostGauge_;	 // 最大ブーストゲージ
	float boostSpeed_;		 // ブースト時の移動速度倍率
	float boostConsumption_; // ブーストゲージ消費速度（per second）
	float boostRecovery_;	 // ブーストゲージ回復速度（per second）
	bool isBoosting_;		 // ブースト中フラグ

	///--------------------------------------------------------------
	///                        バレルロール関連
	bool isBarrelRolling_;			   // バレルロール実行中フラグ
	float barrelRollTime_;			   // バレルロール経過時間
	float barrelRollDuration_;		   // バレルロール全体時間
	float barrelRollCooldown_;		   // バレルロールクールダウン時間
	float barrelRollCoolTimer_;		   // 現在のクールダウンタイマー
	float barrelRollCost_;			   // バレルロール消費ゲージ量
	bool barrelRollDirection_;		   // true=右回転, false=左回転
	MagMath::Vector3 barrelRollStartRotation_;  // バレルロール開始時の回転
	MagMath::Vector3 barrelRollMovementOffset_; // バレルロール中の横移動オフセット
};

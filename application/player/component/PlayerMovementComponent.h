#pragma once
#include "MagMath.h"
#include <algorithm>

///=============================================================================
///						移動管理コンポーネント
/// @brief 通常移動、ブースト、バレルロールの速度・姿勢を管理する。
/// @details 入力収集は担当しない。入力値をProcessInput()へ渡した後、同一フレームにUpdate()を呼び出してTransformへ反映する。
class PlayerMovementComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 移動パラメータと内部状態を初期化する
	void Initialize();
	/// @brief 現在の速度と姿勢をTransformへ反映する
	/// @note transformがnullptrの場合は何もしない。バレルロール中は通常入力より回避演出を優先する。
	void Update(MagMath::Transform *transform, float deltaTime);

	///--------------------------------------------------------------
	///                        移動処理
	/// @brief 入力から目標速度と目標姿勢を更新する
	/// @note Update()の前に呼び出す。バレルロール中は回避姿勢を維持するため入力を受け付けない。
	void ProcessInput(float inputX, float inputY);
	/// @brief 現在速度をTransformの位置へ積分する
	/// @param transform 移動を反映するTransform。nullptrの場合は何もしない。
	/// @param deltaTime 前フレームからの経過時間（秒）
	void ApplyMovement(MagMath::Transform *transform, float deltaTime);
	/// @brief 目標回転をTransformの姿勢へ反映する
	/// @param transform 回転を反映するTransform。nullptrの場合は何もしない。
	void ApplyRotation(MagMath::Transform *transform);

	///--------------------------------------------------------------
	///                        バレルロール処理
	/// @brief 指定方向へバレルロールを開始する
	/// @param isRight trueの場合は右回転、falseの場合は左回転
	/// @note ゲージ不足・クールダウン中・実行中の場合は開始しない。
	void StartBarrelRoll(bool isRight);
	/// @brief 現在速度から回避方向を選んでバレルロールを開始する
	/// @note 停止中は右回転として扱うため、左右を明示したい入力経路ではStartBarrelRoll()を使用する。
	void StartAdaptiveBarrelRoll();
	/// @brief バレルロール中の回転と速度を更新する
	/// @param transform 演出を反映するTransform。nullptrの場合は何もしない。
	/// @param deltaTime 前フレームからの経過時間（秒）
	void UpdateBarrelRoll(MagMath::Transform *transform, float deltaTime);
	/// @brief バレルロールを実行中かを取得する
	/// @return バレルロール中の場合はtrue、それ以外はfalse
	bool IsBarrelRolling() const {
		return isBarrelRolling_;
	}
	/// @brief バレルロールの時間進行度を取得する
	/// @return 開始から終了までの進行度
	float GetBarrelRollProgress() const {
		// NOTE: durationは設定側で正値を維持する。0以下を指定すると進行度を計算できない。
		return barrelRollTime_ / barrelRollDuration_;
	}
	/// @brief バレルロールを開始できるかを判定する
	/// @return ゲージが十分でクールダウン終了済みの場合はtrue、それ以外はfalse
	bool CanBarrelRoll() const {
		return boostGauge_ >= barrelRollCost_ && barrelRollCoolTimer_ <= 0.0f;
	}

	///--------------------------------------------------------------
	///                        ブースト処理
	/// @brief ブースト状態とゲージ残量を更新する
	/// @note バレルロール中は両方の速度補正を重ねず、回避の速度制御を優先する。
	void ProcessBoost(bool boostInput, float deltaTime);
	/// @brief ブーストを使用可能かを判定する
	/// @return ゲージが残っておりバレルロール中でない場合はtrue、それ以外はfalse
	bool CanBoost() const {
		return boostGauge_ > 0.0f && !isBarrelRolling_;
	}
	/// @brief ブーストを実行中かを取得する
	/// @return ブースト中の場合はtrue、それ以外はfalse
	bool IsBoosting() const {
		return isBoosting_;
	}
	/// @brief 現在のブーストゲージを取得する
	/// @return 現在のゲージ量
	float GetBoostGauge() const {
		return boostGauge_;
	}
	/// @brief ブーストゲージ上限を取得する
	/// @return 最大ゲージ量
	float GetMaxBoostGauge() const {
		return maxBoostGauge_;
	}
	float GetBoostGaugeRatio() const {
		// NOTE: Initialize()でmaxBoostGauge_を正値に設定済みであることを前提とする。
		return boostGauge_ / maxBoostGauge_;
	}
	
	/// @brief ブーストゲージを報酬分だけ回復する
	/// @param reward 加算するゲージ量。上限を超える分は加算しない。
	void AddBoostGaugeReward(float reward) {
		boostGauge_ = std::min(boostGauge_ + reward, maxBoostGauge_);
	}

	///--------------------------------------------------------------
	///                        ゲッター
	/// @brief 現在Transformへ反映する移動速度を取得する
	/// @return 現在速度への参照
	const MagMath::Vector3 &GetCurrentVelocity() const {
		return currentVelocity_;
	}
	/// @brief 入力から決定した目標移動速度を取得する
	/// @return 目標速度への参照
	const MagMath::Vector3 &GetTargetVelocity() const {
		return targetVelocity_;
	}
	/// @brief 入力から決定した目標姿勢を取得する
	/// @return 目標回転（ラジアン）への参照
	const MagMath::Vector3 &GetTargetRotation() const {
		return targetRotationEuler_;
	}
	/// @brief 通常移動速度の設定値を取得する
	/// @return 基本移動速度
	float GetMoveSpeed() const {
		return moveSpeed_;
	}
	/// @brief 目標速度への追従係数を取得する
	/// @return 加速度設定値
	float GetAcceleration() const {
		return acceleration_;
	}

	///--------------------------------------------------------------
	///                        セッター
	/// @brief 通常移動速度を設定する
	/// @param speed 基本移動速度
	void SetMoveSpeed(float speed) {
		moveSpeed_ = speed;
	}
	/// @brief 目標速度への追従係数を設定する
	/// @param accel 補間に使用する加速度設定値
	void SetAcceleration(float accel) {
		acceleration_ = accel;
	}
	/// @brief 入力姿勢への追従係数を設定する
	/// @param smoothing 回転補間に使用する係数
	void SetRotationSmoothing(float smoothing) {
		rotationSmoothing_ = smoothing;
	}
	/// @brief 入力時の最大ロール角を設定する
	/// @param angle 最大ロール角（度）
	void SetMaxRollAngle(float angle) {
		maxRollAngle_ = angle;
	}
	/// @brief 入力時の最大ピッチ角を設定する
	/// @param angle 最大ピッチ角（度）
	void SetMaxPitchAngle(float angle) {
		maxPitchAngle_ = angle;
	}
	/// @brief ブースト中の速度倍率を設定する
	/// @param speed ブースト速度倍率
	void SetBoostSpeed(float speed) {
		boostSpeed_ = speed;
	}
	/// @brief ブースト中のゲージ消費速度を設定する
	/// @param consumption 1秒あたりの消費量
	void SetBoostConsumption(float consumption) {
		boostConsumption_ = consumption;
	}
	/// @brief 非ブースト時のゲージ回復速度を設定する
	/// @param recovery 1秒あたりの回復量
	void SetBoostRecovery(float recovery) {
		boostRecovery_ = recovery;
	}
	/// @brief バレルロールの継続時間を設定する
	/// @param duration 継続時間（秒）
	void SetBarrelRollDuration(float duration) {
		barrelRollDuration_ = duration;
	}
	/// @brief バレルロール後の再使用待ち時間を設定する
	/// @param cooldown クールダウン時間（秒）
	void SetBarrelRollCooldown(float cooldown) {
		barrelRollCooldown_ = cooldown;
	}
	/// @brief バレルロール開始時のゲージ消費量を設定する
	/// @param cost 消費するゲージ量
	void SetBarrelRollCost(float cost) {
		barrelRollCost_ = cost;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	/// @brief 現在速度を目標速度へ補間する
	void UpdateVelocity();
	/// @brief 入力値から目標ピッチ・ロール角を更新する
	/// @param inputX 横方向入力
	/// @param inputY 縦方向入力
	void UpdateTargetRotation(float inputX, float inputY);

	///--------------------------------------------------------------
	///                        メンバ変数
	MagMath::Vector3 currentVelocity_;     // Transformへ積分する現在速度（units/秒）
	MagMath::Vector3 targetVelocity_;      // 入力とブーストから決める追従目標速度
	MagMath::Vector3 targetRotationEuler_; // 入力を視覚表現へ変換した目標回転（ラジアン）

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
	bool isBarrelRolling_;					   // バレルロール実行中フラグ
	float barrelRollTime_;					   // バレルロール経過時間
	float barrelRollDuration_;				   // バレルロール全体時間
	float barrelRollCooldown_;				   // バレルロールクールダウン時間
	float barrelRollCoolTimer_;				   // 現在のクールダウンタイマー
	float barrelRollCost_;					   // バレルロール消費ゲージ量
	bool barrelRollDirection_;				   // true=右回転, false=左回転
	MagMath::Vector3 barrelRollStartRotation_; // バレルロール開始時の回転
	MagMath::Vector3 barrelRollStartVelocity_; // バレルロール開始時の进行速度
	float barrelRollAcceleration_;			   // バレルロール時の加速度（倍率）
};

#pragma once

#include "MagMath.h"
using namespace MagMath;

#include <algorithm>
#include <cmath>

///=============================================================================
///					Just Avoidance（ジャスト回避）コンポーネント
/// @brief プレイヤーの精密な回避機能を管理するコンポーネント
/// @details 敵の攻撃が迫ってくる直前の一瞬のタイミングで
///          バレルロールを成功させると、スロー演出 + 機体強化される
///
/// ===== ジャスト回避の流れ =====
/// 1. 敵弾がプレイヤーに接近中 → RegisterIncomingDamage()
/// 2. バレルロール開始 → OnBarrelRollStarted()
/// 3. ジャスト判定窓内でロール完了 → CheckJustAvoidanceSuccess()成功
/// 4. スロー演出 + 機体強化バフ発動
class PlayerJustAvoidanceComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize();
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        Just Avoidance判定
	/// @brief 敵弾との距離・接近状況を基に、ジャスト判定対象として登録
	/// @param bulletPosition 敵弾の現在位置
	/// @param bulletPreviousPosition 敵弾の前フレーム位置
	/// @param bulletRadius 敵弾の半径
	/// @param playerPosition プレイヤーの現在位置
	void RegisterIncomingBullet(const Vector3 &bulletPosition,
								const Vector3 &bulletPreviousPosition,
								float bulletRadius,
								const Vector3 &playerPosition);

	/// @brief バレルロール実行を通知
	void OnBarrelRollStarted();

	/// @brief ジャスト回避の判定と報酬付与
	/// @param outWasJustAvoidance ジャスト回避が成功したか
	/// @param outSuccessRate 成功度（0.0～1.0、1.0が完璧）
	/// @return スロー強度（0.0～1.0、1.0で最強スロー）
	float CheckJustAvoidanceSuccess(bool &outWasJustAvoidance, float &outSuccessRate);

	///--------------------------------------------------------------
	///                        ゲッター
	/// @brief ジャスト判定ウィンドウ内かどうか（UI表示用）
	bool IsInJustAvoidanceWindow() const;

	/// @brief ウィンドウ内の残り時間を取得（UI表示用）
	float GetJustAvoidanceWindowTimeRemaining() const;

	/// @brief 最後の成功率を取得
	float GetLastSuccessRate() const {
		return lastSuccessRate_;
	}

	/// @brief スロー時間が残っているか
	bool IsSlowActive() const {
		return slowTimer_ > 0.0f;
	}

	/// @brief 現在のスロー強度を取得（0.0～1.0）
	float GetSlowStrength() const {
		float strength = slowTimer_ / slowDuration_;
		return strength > 0.0f ? strength : 0.0f;
	}

	/// @brief 機体強化バフが有効か
	bool IsPowerUpActive() const {
		return powerUpTimer_ > 0.0f;
	}

	/// @brief 現在の攻撃力倍率を取得
	float GetAttackMultiplier() const {
		return IsPowerUpActive() ? attackMultiplier_ : 1.0f;
	}

	/// @brief 現在の移動速度倍率を取得
	float GetSpeedMultiplier() const {
		return IsPowerUpActive() ? speedMultiplier_ : 1.0f;
	}

	/// @brief ゲーム全体に適用するタイム スケール倍率を取得（スロー用）
	/// @details スロー中は0.5～1.0の値を返す（1.0 - slowStrength）
	float GetGameTimeScale() const {
		return IsSlowActive() ? (1.0f - GetSlowStrength()) : 1.0f;
	}

	///--------------------------------------------------------------
	///                        セッター
	/// @brief Just Avoidanceウィンドウのサイズを設定（秒）
	void SetJustAvoidanceWindowSize(float windowSize) {
		justAvoidanceWindowSize_ = windowSize;
	}

	/// @brief 敵弾検出範囲を設定（units）
	void SetDetectionRadius(float radius) {
		detectionRadius_ = radius;
	}

	/// @brief スロー演出の持続時間を設定（秒）
	void SetSlowDuration(float duration) {
		slowDuration_ = duration;
	}

	/// @brief スロー強度を設定（0.0～1.0）
	void SetSlowStrength(float strength) {
		if (strength < 0.0f)
			slowStrength_ = 0.0f;
		else if (strength > 1.0f)
			slowStrength_ = 1.0f;
		else
			slowStrength_ = strength;
	}

	/// @brief 機体強化バフの持続時間を設定（秒）
	void SetPowerUpDuration(float duration) {
		powerUpDuration_ = duration;
	}

	/// @brief 攻撃力倍率を設定
	void SetAttackMultiplier(float multiplier) {
		attackMultiplier_ = multiplier;
	}

	/// @brief 移動速度倍率を設定
	void SetSpeedMultiplier(float multiplier) {
		speedMultiplier_ = multiplier;
	}

private:
	///--------------------------------------------------------------
	///                        内部メンバ関数

	/// @brief 敵弾がプレイヤーに接近中かどうかを判定
	bool IsBulletApproaching(const Vector3 &bulletPos, const Vector3 &bulletPrevPos,
							 const Vector3 &playerPos) const;

	/// @brief 敵弾との距離を計算
	float CalculateBulletDistance(const Vector3 &bulletPos, const Vector3 &playerPos) const;

	///--------------------------------------------------------------
	///                        メンバ変数

	// Just Avoidance実行関連
	float lastIncomingBulletTime_;	//! 最後に敵弾検出した時刻
	bool hasIncomingBullet_;		//! 検出中の敵弾があるフラグ
	float lastBarrelRollStartTime_; //! 最後にバレルロールを開始した時刻
	Vector3 lastBulletPosition_;	//! 最後に検出した敵弾の位置
	Vector3 lastBulletPreviousPos_; //! 最後に検出した敵弾の前フレーム位置

	// Just Avoidanceウィンドウ（秒）
	float justAvoidanceWindowSize_; //! デフォルト: 0.25秒
	float detectionRadius_;			//! 敵弾検出範囲（デフォルト: 5.0units）
	float detectionTimeout_;		//! 検出状態の有効期限（デフォルト: 0.5秒）

	// スロー演出関連
	float slowTimer_;	 //! スロー時間カウンタ
	float slowDuration_; //! スロー持続時間（デフォルト: 0.5秒）
	float slowStrength_; //! スロー強度 0.0～1.0（デフォルト: 0.5）
						 //! 0.5 = 時間が半速になる

	// 機体強化バフ関連
	float powerUpTimer_;	 //! 強化バフのカウンタ
	float powerUpDuration_;	 //! 強化バフ持続時間（デフォルト: 2.0秒）
	float attackMultiplier_; //! 攻撃力倍率（デフォルト: 1.5f）
	float speedMultiplier_;	 //! 移動速度倍率（デフォルト: 1.3f）

	// パフォーマンス追跡用
	float lastSuccessRate_; //! 直近のジャスト回避の精度（0-1、1.0が完璧）
};

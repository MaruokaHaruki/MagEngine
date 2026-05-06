/*********************************************************************
 * \file   PlayerConfigLoader.h
 * \brief  プレイヤー設定JSONファイルの読み込みと解析
 *
 * \author Harukichimaru
 * \date   May 2025
 *********************************************************************/
#pragma once
#include <string>

/**
 * @brief プレイヤー設定をJSONから読み込む静的ユーティリティ
 *
 * resources/config/player/player_config.json から全プレイヤー設定値を
 * 読み込み、各コンポーネントが必要な値を取得できるようにする責務を持つ
 * 
 * 設計のポイント：
 * - 各値へのアクセスは静的メソッドを通じて行う
 * - JSONパース処理はすべてこのクラスに集約
 * - PlayerConstants.hの値はデフォルト値として機能（互換性維持）
 * - 将来的に複数のプレイヤータイプ対応に拡張可能
 */
class PlayerConfigLoader {
public:
	/**
	 * @brief プレイヤー設定ファイルを読み込む
	 * 
	 * @param configPath JSONファイルへのパス
	 *        （デフォルト："resources/config/player/player_config.json"）
	 * @return 読み込み成功時true、失敗時false
	 */
	static bool LoadConfig(const std::string &configPath = "resources/config/player/player_config.json");

	/**
	 * @brief 初期化済みかどうかを確認
	 * 
	 * @return プレイヤー設定が読み込まれている場合true
	 */
	static bool IsInitialized();

	//========================================
	//          アクセッサメソッド
	//========================================

	// Timing
	static float GetFrameTime();
	static float GetFrameRate();

	// Input
	static float GetStickDeadzone();
	static float GetTriggerThreshold();

	// Weapon - Bullet
	static float GetBulletSpeed();
	static float GetBulletLifetime();
	static float GetBulletRadius();
	static float GetShootCooldown();

	// Weapon - Missile
	static float GetMissileSpeed();
	static float GetMissileTurnRate();
	static float GetMissileLifetime();
	static int GetMissileMaxAmmo();
	static float GetMissileRecoveryTime();

	// Movement
	static float GetDefaultMoveSpeed();
	static float GetDefaultAcceleration();
	static float GetDefaultRotationSmoothing();
	static float GetMaxRollAngle();
	static float GetMaxPitchAngle();

	// Boost
	static float GetBoostMaxGauge();
	static float GetBoostSpeedMultiplier();
	static float GetBoostConsumptionRate();
	static float GetBoostRecoveryRate();

	// BarrelRoll
	static float GetBarrelRollDuration();
	static float GetBarrelRollCooldown();
	static float GetBarrelRollCost();
	static float GetBarrelRollAccelerationMultiplier();
	static float GetBarrelRollRotationAngleRadians();

	// LockOn
	static float GetLockOnRange();
	static float GetLockOnFOVDegrees();
	static float GetLockOnAcquisitionInterval();
	static int GetLockOnMaxTargets();
	static float GetLockOnRetentionTime();

	// JustAvoidance
	static float GetJustAvoidanceWindowSize();
	static float GetJustAvoidanceBoostReward();
	static float GetJustAvoidanceDamageTimeout();
	static float GetJustAvoidancePerfectTimingThreshold();

	// Health
	static int GetDefaultMaxHP();
	static float GetInvincibilityDuration();
	static int GetEnemyBulletDamage();
	static int GetCollisionDamage();

	// Defeat
	static float GetDefeatAnimationDuration();
	static float GetDefeatPhase1Ratio();
	static float GetDefeatGravityAcceleration();
	static float GetDefeatGroundYThreshold();
	static float GetDefeatNoseDiveAngle();

private:
	// 内部フラグ
	static bool initialized_;
};

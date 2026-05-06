/*********************************************************************
 * \file   PlayerConfigLoader.cpp
 * \brief  プレイヤー設定JSONファイルの読み込みと解析の実装
 *
 * \author Harukichimaru
 * \date   May 2025
 *********************************************************************/
#include "PlayerConfigLoader.h"
#include "../../../externals/json.hpp"
#include "../PlayerConstants.h"
#include <fstream>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

//========================================
// 静的メンバ変数の初期化
//========================================
bool PlayerConfigLoader::initialized_ = false;

// キャッシュ用の内部変数（読み込みが成功した場合のみ使用）
namespace {
	// Timing
	static float g_frameTime = PlayerConstants::FRAME_TIME;
	static float g_frameRate = PlayerConstants::FRAME_RATE;

	// Input
	static float g_stickDeadzone = PlayerConstants::STICK_DEADZONE;
	static float g_triggerThreshold = PlayerConstants::TRIGGER_THRESHOLD;

	// Weapon - Bullet
	static float g_bulletSpeed = PlayerConstants::Weapon::BULLET_SPEED;
	static float g_bulletLifetime = PlayerConstants::Weapon::BULLET_LIFETIME;
	static float g_bulletRadius = PlayerConstants::Weapon::BULLET_RADIUS;
	static float g_shootCooldown = PlayerConstants::Weapon::SHOOT_COOLDOWN;

	// Weapon - Missile
	static float g_missileSpeed = PlayerConstants::Weapon::MISSILE_SPEED;
	static float g_missileTurnRate = PlayerConstants::Weapon::MISSILE_TURN_RATE;
	static float g_missileLifetime = PlayerConstants::Weapon::MISSILE_LIFETIME;
	static int g_missileMaxAmmo = PlayerConstants::Weapon::MISSILE_MAX_AMMO;
	static float g_missileRecoveryTime = PlayerConstants::Weapon::MISSILE_RECOVERY_TIME;

	// Movement
	static float g_defaultMoveSpeed = PlayerConstants::Movement::DEFAULT_MOVE_SPEED;
	static float g_defaultAcceleration = PlayerConstants::Movement::DEFAULT_ACCELERATION;
	static float g_defaultRotationSmoothing = PlayerConstants::Movement::DEFAULT_ROTATION_SMOOTHING;
	static float g_maxRollAngle = PlayerConstants::Movement::MAX_ROLL_ANGLE;
	static float g_maxPitchAngle = PlayerConstants::Movement::MAX_PITCH_ANGLE;

	// Boost
	static float g_boostMaxGauge = PlayerConstants::Boost::MAX_GAUGE;
	static float g_boostSpeedMultiplier = PlayerConstants::Boost::SPEED_MULTIPLIER;
	static float g_boostConsumptionRate = PlayerConstants::Boost::CONSUMPTION_RATE;
	static float g_boostRecoveryRate = PlayerConstants::Boost::RECOVERY_RATE;

	// BarrelRoll
	static float g_barrelRollDuration = PlayerConstants::BarrelRoll::DURATION;
	static float g_barrelRollCooldown = PlayerConstants::BarrelRoll::COOLDOWN;
	static float g_barrelRollCost = PlayerConstants::BarrelRoll::COST;
	static float g_barrelRollAccelerationMultiplier = PlayerConstants::BarrelRoll::ACCELERATION_MULTIPLIER;
	static float g_barrelRollRotationAngleRadians = PlayerConstants::BarrelRoll::ROTATION_ANGLE_RADIANS;

	// LockOn
	static float g_lockOnRange = PlayerConstants::LockOn::RANGE;
	static float g_lockOnFOVDegrees = PlayerConstants::LockOn::FOV_DEGREES;
	static float g_lockOnAcquisitionInterval = PlayerConstants::LockOn::ACQUISITION_INTERVAL;
	static int g_lockOnMaxTargets = PlayerConstants::LockOn::MAX_TARGETS;
	static float g_lockOnRetentionTime = PlayerConstants::LockOn::RETENTION_TIME;

	// JustAvoidance
	static float g_justAvoidanceWindowSize = PlayerConstants::JustAvoidance::WINDOW_SIZE;
	static float g_justAvoidanceBoostReward = PlayerConstants::JustAvoidance::BOOST_REWARD;
	static float g_justAvoidanceDamageTimeout = PlayerConstants::JustAvoidance::DAMAGE_TIMEOUT;
	static float g_justAvoidancePerfectTimingThreshold = PlayerConstants::JustAvoidance::PERFECT_TIMING_THRESHOLD;

	// Health
	static int g_defaultMaxHP = PlayerConstants::Health::DEFAULT_MAX_HP;
	static float g_invincibilityDuration = PlayerConstants::Health::INVINCIBILITY_DURATION;
	static int g_enemyBulletDamage = PlayerConstants::Health::ENEMY_BULLET_DAMAGE;
	static int g_collisionDamage = PlayerConstants::Health::COLLISION_DAMAGE;

	// Defeat
	static float g_defeatAnimationDuration = PlayerConstants::Defeat::ANIMATION_DURATION;
	static float g_defeatPhase1Ratio = PlayerConstants::Defeat::PHASE1_RATIO;
	static float g_defeatGravityAcceleration = PlayerConstants::Defeat::GRAVITY_ACCELERATION;
	static float g_defeatGroundYThreshold = PlayerConstants::Defeat::GROUND_Y_THRESHOLD;
	static float g_defeatNoseDiveAngle = PlayerConstants::Defeat::NOSE_DIVE_ANGLE;
}

bool PlayerConfigLoader::LoadConfig(const std::string &configPath) {
	try {
		std::ifstream file(configPath);
		if (!file.is_open()) {
			std::cerr << "Failed to open player config: " << configPath << std::endl;
			return false;
		}

		json j;
		file >> j;
		file.close();

		// Timing
		g_frameTime = j.value("timing", json::object())
			.value("frameTime", PlayerConstants::FRAME_TIME);
		g_frameRate = j.value("timing", json::object())
			.value("frameRate", PlayerConstants::FRAME_RATE);

		// Input
		g_stickDeadzone = j.value("input", json::object())
			.value("stickDeadzone", PlayerConstants::STICK_DEADZONE);
		g_triggerThreshold = j.value("input", json::object())
			.value("triggerThreshold", PlayerConstants::TRIGGER_THRESHOLD);

		// Weapon - Bullet
		auto weaponJson = j.value("weapon", json::object());
		auto bulletJson = weaponJson.value("bullet", json::object());
		g_bulletSpeed = bulletJson.value("speed", PlayerConstants::Weapon::BULLET_SPEED);
		g_bulletLifetime = bulletJson.value("lifetime", PlayerConstants::Weapon::BULLET_LIFETIME);
		g_bulletRadius = bulletJson.value("radius", PlayerConstants::Weapon::BULLET_RADIUS);
		g_shootCooldown = bulletJson.value("shootCooldown", PlayerConstants::Weapon::SHOOT_COOLDOWN);

		// Weapon - Missile
		auto missileJson = weaponJson.value("missile", json::object());
		g_missileSpeed = missileJson.value("speed", PlayerConstants::Weapon::MISSILE_SPEED);
		g_missileTurnRate = missileJson.value("turnRate", PlayerConstants::Weapon::MISSILE_TURN_RATE);
		g_missileLifetime = missileJson.value("lifetime", PlayerConstants::Weapon::MISSILE_LIFETIME);
		g_missileMaxAmmo = missileJson.value("maxAmmo", PlayerConstants::Weapon::MISSILE_MAX_AMMO);
		g_missileRecoveryTime = missileJson.value("recoveryTime", PlayerConstants::Weapon::MISSILE_RECOVERY_TIME);

		// Movement
		auto movementJson = j.value("movement", json::object());
		g_defaultMoveSpeed = movementJson.value("defaultSpeed", PlayerConstants::Movement::DEFAULT_MOVE_SPEED);
		g_defaultAcceleration = movementJson.value("defaultAcceleration", PlayerConstants::Movement::DEFAULT_ACCELERATION);
		g_defaultRotationSmoothing = movementJson.value("defaultRotationSmoothing", PlayerConstants::Movement::DEFAULT_ROTATION_SMOOTHING);
		g_maxRollAngle = movementJson.value("maxRollAngle", PlayerConstants::Movement::MAX_ROLL_ANGLE);
		g_maxPitchAngle = movementJson.value("maxPitchAngle", PlayerConstants::Movement::MAX_PITCH_ANGLE);

		// Boost
		auto boostJson = j.value("boost", json::object());
		g_boostMaxGauge = boostJson.value("maxGauge", PlayerConstants::Boost::MAX_GAUGE);
		g_boostSpeedMultiplier = boostJson.value("speedMultiplier", PlayerConstants::Boost::SPEED_MULTIPLIER);
		g_boostConsumptionRate = boostJson.value("consumptionRate", PlayerConstants::Boost::CONSUMPTION_RATE);
		g_boostRecoveryRate = boostJson.value("recoveryRate", PlayerConstants::Boost::RECOVERY_RATE);

		// BarrelRoll
		auto barrelRollJson = j.value("barrelRoll", json::object());
		g_barrelRollDuration = barrelRollJson.value("duration", PlayerConstants::BarrelRoll::DURATION);
		g_barrelRollCooldown = barrelRollJson.value("cooldown", PlayerConstants::BarrelRoll::COOLDOWN);
		g_barrelRollCost = barrelRollJson.value("cost", PlayerConstants::BarrelRoll::COST);
		g_barrelRollAccelerationMultiplier = barrelRollJson.value("accelerationMultiplier", PlayerConstants::BarrelRoll::ACCELERATION_MULTIPLIER);
		g_barrelRollRotationAngleRadians = barrelRollJson.value("rotationAngleRadians", PlayerConstants::BarrelRoll::ROTATION_ANGLE_RADIANS);

		// LockOn
		auto lockOnJson = j.value("lockOn", json::object());
		g_lockOnRange = lockOnJson.value("range", PlayerConstants::LockOn::RANGE);
		g_lockOnFOVDegrees = lockOnJson.value("fovDegrees", PlayerConstants::LockOn::FOV_DEGREES);
		g_lockOnAcquisitionInterval = lockOnJson.value("acquisitionInterval", PlayerConstants::LockOn::ACQUISITION_INTERVAL);
		g_lockOnMaxTargets = lockOnJson.value("maxTargets", PlayerConstants::LockOn::MAX_TARGETS);
		g_lockOnRetentionTime = lockOnJson.value("retentionTime", PlayerConstants::LockOn::RETENTION_TIME);

		// JustAvoidance
		auto justAvoidanceJson = j.value("justAvoidance", json::object());
		g_justAvoidanceWindowSize = justAvoidanceJson.value("windowSize", PlayerConstants::JustAvoidance::WINDOW_SIZE);
		g_justAvoidanceBoostReward = justAvoidanceJson.value("boostReward", PlayerConstants::JustAvoidance::BOOST_REWARD);
		g_justAvoidanceDamageTimeout = justAvoidanceJson.value("damageTimeout", PlayerConstants::JustAvoidance::DAMAGE_TIMEOUT);
		g_justAvoidancePerfectTimingThreshold = justAvoidanceJson.value("perfectTimingThreshold", PlayerConstants::JustAvoidance::PERFECT_TIMING_THRESHOLD);

		// Health
		auto healthJson = j.value("health", json::object());
		g_defaultMaxHP = healthJson.value("defaultMaxHP", PlayerConstants::Health::DEFAULT_MAX_HP);
		g_invincibilityDuration = healthJson.value("invincibilityDuration", PlayerConstants::Health::INVINCIBILITY_DURATION);
		g_enemyBulletDamage = healthJson.value("enemyBulletDamage", PlayerConstants::Health::ENEMY_BULLET_DAMAGE);
		g_collisionDamage = healthJson.value("collisionDamage", PlayerConstants::Health::COLLISION_DAMAGE);

		// Defeat
		auto defeatJson = j.value("defeat", json::object());
		g_defeatAnimationDuration = defeatJson.value("animationDuration", PlayerConstants::Defeat::ANIMATION_DURATION);
		g_defeatPhase1Ratio = defeatJson.value("phase1Ratio", PlayerConstants::Defeat::PHASE1_RATIO);
		g_defeatGravityAcceleration = defeatJson.value("gravityAcceleration", PlayerConstants::Defeat::GRAVITY_ACCELERATION);
		g_defeatGroundYThreshold = defeatJson.value("groundYThreshold", PlayerConstants::Defeat::GROUND_Y_THRESHOLD);
		g_defeatNoseDiveAngle = defeatJson.value("noseDiveAngle", PlayerConstants::Defeat::NOSE_DIVE_ANGLE);

		initialized_ = true;
		std::cout << "Successfully loaded player config from: " << configPath << std::endl;
		return true;

	} catch (const std::exception &e) {
		std::cerr << "Error loading player config: " << e.what() << std::endl;
		return false;
	}
}

bool PlayerConfigLoader::IsInitialized() {
	return initialized_;
}

//========================================
// Timing
//========================================
float PlayerConfigLoader::GetFrameTime() {
	return g_frameTime;
}

float PlayerConfigLoader::GetFrameRate() {
	return g_frameRate;
}

//========================================
// Input
//========================================
float PlayerConfigLoader::GetStickDeadzone() {
	return g_stickDeadzone;
}

float PlayerConfigLoader::GetTriggerThreshold() {
	return g_triggerThreshold;
}

//========================================
// Weapon - Bullet
//========================================
float PlayerConfigLoader::GetBulletSpeed() {
	return g_bulletSpeed;
}

float PlayerConfigLoader::GetBulletLifetime() {
	return g_bulletLifetime;
}

float PlayerConfigLoader::GetBulletRadius() {
	return g_bulletRadius;
}

float PlayerConfigLoader::GetShootCooldown() {
	return g_shootCooldown;
}

//========================================
// Weapon - Missile
//========================================
float PlayerConfigLoader::GetMissileSpeed() {
	return g_missileSpeed;
}

float PlayerConfigLoader::GetMissileTurnRate() {
	return g_missileTurnRate;
}

float PlayerConfigLoader::GetMissileLifetime() {
	return g_missileLifetime;
}

int PlayerConfigLoader::GetMissileMaxAmmo() {
	return g_missileMaxAmmo;
}

float PlayerConfigLoader::GetMissileRecoveryTime() {
	return g_missileRecoveryTime;
}

//========================================
// Movement
//========================================
float PlayerConfigLoader::GetDefaultMoveSpeed() {
	return g_defaultMoveSpeed;
}

float PlayerConfigLoader::GetDefaultAcceleration() {
	return g_defaultAcceleration;
}

float PlayerConfigLoader::GetDefaultRotationSmoothing() {
	return g_defaultRotationSmoothing;
}

float PlayerConfigLoader::GetMaxRollAngle() {
	return g_maxRollAngle;
}

float PlayerConfigLoader::GetMaxPitchAngle() {
	return g_maxPitchAngle;
}

//========================================
// Boost
//========================================
float PlayerConfigLoader::GetBoostMaxGauge() {
	return g_boostMaxGauge;
}

float PlayerConfigLoader::GetBoostSpeedMultiplier() {
	return g_boostSpeedMultiplier;
}

float PlayerConfigLoader::GetBoostConsumptionRate() {
	return g_boostConsumptionRate;
}

float PlayerConfigLoader::GetBoostRecoveryRate() {
	return g_boostRecoveryRate;
}

//========================================
// BarrelRoll
//========================================
float PlayerConfigLoader::GetBarrelRollDuration() {
	return g_barrelRollDuration;
}

float PlayerConfigLoader::GetBarrelRollCooldown() {
	return g_barrelRollCooldown;
}

float PlayerConfigLoader::GetBarrelRollCost() {
	return g_barrelRollCost;
}

float PlayerConfigLoader::GetBarrelRollAccelerationMultiplier() {
	return g_barrelRollAccelerationMultiplier;
}

float PlayerConfigLoader::GetBarrelRollRotationAngleRadians() {
	return g_barrelRollRotationAngleRadians;
}

//========================================
// LockOn
//========================================
float PlayerConfigLoader::GetLockOnRange() {
	return g_lockOnRange;
}

float PlayerConfigLoader::GetLockOnFOVDegrees() {
	return g_lockOnFOVDegrees;
}

float PlayerConfigLoader::GetLockOnAcquisitionInterval() {
	return g_lockOnAcquisitionInterval;
}

int PlayerConfigLoader::GetLockOnMaxTargets() {
	return g_lockOnMaxTargets;
}

float PlayerConfigLoader::GetLockOnRetentionTime() {
	return g_lockOnRetentionTime;
}

//========================================
// JustAvoidance
//========================================
float PlayerConfigLoader::GetJustAvoidanceWindowSize() {
	return g_justAvoidanceWindowSize;
}

float PlayerConfigLoader::GetJustAvoidanceBoostReward() {
	return g_justAvoidanceBoostReward;
}

float PlayerConfigLoader::GetJustAvoidanceDamageTimeout() {
	return g_justAvoidanceDamageTimeout;
}

float PlayerConfigLoader::GetJustAvoidancePerfectTimingThreshold() {
	return g_justAvoidancePerfectTimingThreshold;
}

//========================================
// Health
//========================================
int PlayerConfigLoader::GetDefaultMaxHP() {
	return g_defaultMaxHP;
}

float PlayerConfigLoader::GetInvincibilityDuration() {
	return g_invincibilityDuration;
}

int PlayerConfigLoader::GetEnemyBulletDamage() {
	return g_enemyBulletDamage;
}

int PlayerConfigLoader::GetCollisionDamage() {
	return g_collisionDamage;
}

//========================================
// Defeat
//========================================
float PlayerConfigLoader::GetDefeatAnimationDuration() {
	return g_defeatAnimationDuration;
}

float PlayerConfigLoader::GetDefeatPhase1Ratio() {
	return g_defeatPhase1Ratio;
}

float PlayerConfigLoader::GetDefeatGravityAcceleration() {
	return g_defeatGravityAcceleration;
}

float PlayerConfigLoader::GetDefeatGroundYThreshold() {
	return g_defeatGroundYThreshold;
}

float PlayerConfigLoader::GetDefeatNoseDiveAngle() {
	return g_defeatNoseDiveAngle;
}

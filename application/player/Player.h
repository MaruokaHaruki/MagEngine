/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \brief  プレイヤーキャラクターの総合管理クラス
 *
 * 責務：
 * - 各コンポーネント（移動・HP・射撃・ロックオン・敗北演出）の統合管理
 * - 入力処理の委譲
 * - オブジェクト管理
 *
 * 各責務は以下のコンポーネントに分離：
 * - PlayerMovementComponent: 移動・バレルロール・ブースト
 * - PlayerCombatComponent: 弾・ミサイル発射
 * - PlayerHealthComponent: HP・ダメージ
 * - PlayerLockedOnComponent: ロックオン機能
 * - PlayerDefeatComponent: 敗北演出
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Input.h"
#include "MagMath.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include "PlayerConstants.h"
#include "ResourcePaths.h"
#include "component/PlayerCombatComponent.h"
#include "component/PlayerDefeatComponent.h"
#include "component/PlayerHealthComponent.h"
#include "component/PlayerJustAvoidanceComponent.h"
#include "component/PlayerLockedOnComponent.h"
#include "component/PlayerMovementComponent.h"
#include "GameOverAnimation.h"
#include <memory>
#include <string>
#include <vector>

//========================================
// 前方宣言
class Object3dSetup;
class EnemyManager;
class EnemyBase; // Enemy から EnemyBase に変更
class EnemyBullet;

///=============================================================================
///						武装設定構造体
struct WeaponConfig {
	//========================================
	//          弾（銃）の設定
	//========================================
	/// @brief 弾のモデルパス
	std::string bulletModelPath = ResourcePath::Model::BULLET;
	
	/// @brief 弾のテクスチャパス（未使用時は空文字列）
	std::string bulletTexturePath = ResourcePath::Texture::BULLET_DEFAULT;
	
	/// @brief 弾の速度（units/秒）
	float bulletSpeed = PlayerConstants::Weapon::BULLET_SPEED;
	
	/// @brief 弾の生存時間（秒）
	float bulletMaxLifeTime = PlayerConstants::Weapon::BULLET_LIFETIME;
	
	/// @brief 弾の当たり判定半径（units）
	float bulletRadius = PlayerConstants::Weapon::BULLET_RADIUS;
	
	/// @brief 連射クールタイム（秒）
	float shootCoolTime = PlayerConstants::Weapon::SHOOT_COOLDOWN;

	//========================================
	//          ミサイルの設定
	//========================================
	/// @brief ミサイルのモデルパス
	std::string missileModelPath = ResourcePath::Model::MISSILE;
	
	/// @brief ミサイルのテクスチャパス（未使用時は空文字列）
	std::string missileTexturePath = ResourcePath::Texture::MISSILE_DEFAULT;
	
	/// @brief ミサイルの速度（units/秒）
	float missileSpeed = PlayerConstants::Weapon::MISSILE_SPEED;
	
	/// @brief ミサイルの最大旋回速度（度/秒）
	float missileMaxTurnRate = PlayerConstants::Weapon::MISSILE_TURN_RATE;
	
	/// @brief ミサイルの生存時間（秒）
	float missileMaxLifeTime = PlayerConstants::Weapon::MISSILE_LIFETIME;
	
	/// @brief ミサイル最大残弾数
	int missileMaxAmmo = PlayerConstants::Weapon::MISSILE_MAX_AMMO;
	
	/// @brief ミサイル1発の回復時間（秒）
	float missileRecoveryTime = PlayerConstants::Weapon::MISSILE_RECOVERY_TIME;

	//========================================
	//          拡張用
	//========================================
	// マシンガンなど他の武装タイプはここに追加
};

///=============================================================================
///						クラス定義
class Player : public BaseObject {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 初期化
	/// @param object3dSetup object3dのセットアップ情報
	/// @param modelPath モデルパス
	void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath);
	/// @brief 更新
	void Update();
	/// @brief 描画
	void Draw();
	/// @brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        弾描画
	/// @brief 弾描画
	void DrawBullets();
	/// @brief ミサイル描画
	void DrawMissiles();
	/// @brief 弾のトレイル描画
	void DrawBulletsTrails();
	/// @brief ミサイルのトレイル描画
	void DrawMissilesTrails();

	//========================================
	// EnemyManager設定（ミサイル・ロックオン用）
	/// @brief 敵マネージャーの設定
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
		combatComponent_.SetEnemyManager(enemyManager);
		lockedOnComponent_.SetEnemyManager(enemyManager);
	}
	/// @brief 敵マネージャーの取得（HUD用）
	EnemyManager *GetEnemyManager() const {
		return enemyManager_;
	}

	//========================================
	// TrailEffectManager設定（弾・ミサイルトレイル用）
	/// @brief トレイルエフェクトマネージャーの設定
	void SetTrailEffectManager(MagEngine::TrailEffectManager *trailEffectManager) {
		combatComponent_.SetTrailEffectManager(trailEffectManager);
	}

	//========================================
	// ロックオン機能（PlayerLockedOnComponentに委譲）
	/// @brief ロックオン状態確認
	bool HasLockOnTarget() const {
		return lockedOnComponent_.HasLockOnTarget();
	}
	/// @brief プライマリロックオン対象の取得
	EnemyBase *GetLockOnTarget() const {
		return lockedOnComponent_.GetPrimaryTarget();
	}
	/// @brief 全ロックオン対象を取得
	const std::vector<EnemyBase *> &GetAllLockOnTargets() const {
		return lockedOnComponent_.GetAllTargets();
	}
	/// @brief ロックオン対象の数を取得
	size_t GetLockOnTargetCount() const {
		return lockedOnComponent_.GetTargetCount();
	}
	/// @brief ミサイル長押しロックオンモード中か
	bool IsMissileLockOnMode() const {
		return isInLockOnMode_;
	}
	/// @brief 現在照準内にいるロック候補を取得
	EnemyBase *GetAimingLockOnTarget() const {
		return lockedOnComponent_.GetAimingTarget();
	}
	/// @brief ロックオン範囲のセッター
	void SetLockOnRange(float range) {
		lockedOnComponent_.SetLockOnRange(range);
	}
	/// @brief ロックオン範囲のゲッター
	float GetLockOnRange() const {
		return lockedOnComponent_.GetLockOnRange();
	}
	/// @brief ロックオンFOVのゲッター
	float GetLockOnFOV() const {
		return lockedOnComponent_.GetLockOnFOV();
	}

	///--------------------------------------------------------------
	///                        敗北演出（PlayerDefeatComponentに委譲）
	/// @brief 敗北判定
	bool IsDefeated() const {
		return defeatComponent_.IsDefeated();
	}
	/// @brief 敗北演出完了判定
	bool IsDefeatAnimationComplete() const {
		return defeatComponent_.IsDefeatAnimationComplete();
	}
	/// @brief 敗北演出開始
	void StartDefeatAnimation() {
		defeatComponent_.StartDefeatAnimation();
	}

	///--------------------------------------------------------------
	///                        Just Avoidance（PlayerJustAvoidanceComponentに委譲）
	/// @brief ジャスト回避ウィンドウ内かどうか
	bool IsInJustAvoidanceWindow() const {
		return justAvoidanceComponent_.IsInJustAvoidanceWindow();
	}
	/// @brief ジャスト回避ウィンドウの残り時間を取得
	float GetJustAvoidanceWindowTimeRemaining() const {
		return justAvoidanceComponent_.GetJustAvoidanceWindowTimeRemaining();
	}
	/// @brief 直近のジャスト回避成功率を取得（0-1）
	float GetJustAvoidanceSuccessRate() const {
		return justAvoidanceComponent_.GetLastSuccessRate();
	}
	/// @brief スロー演出中かどうか
	bool IsSlowActive() const {
		return justAvoidanceComponent_.IsSlowActive();
	}
	/// @brief 現在のスロー強度を取得（0.0～1.0）
	float GetSlowStrength() const {
		return justAvoidanceComponent_.GetSlowStrength();
	}
	/// @brief 機体強化バフ中かどうか
	bool IsPowerUpActive() const {
		return justAvoidanceComponent_.IsPowerUpActive();
	}
	/// @brief 現在の攻撃力倍率を取得
	float GetAttackMultiplier() const {
		return justAvoidanceComponent_.GetAttackMultiplier();
	}
	/// @brief 現在の移動速度倍率を取得
	float GetSpeedMultiplier() const {
		return justAvoidanceComponent_.GetSpeedMultiplier();
	}
	
	/// @brief ジャスト回避ウィンドウサイズを設定
	void SetJustAvoidanceWindowSize(float windowSize) {
		justAvoidanceComponent_.SetJustAvoidanceWindowSize(windowSize);
	}
	/// @brief 敵弾検出範囲を設定
	void SetJustAvoidanceDetectionRadius(float radius) {
		justAvoidanceComponent_.SetDetectionRadius(radius);
	}
	/// @brief スロー演出の持続時間を設定
	void SetSlowDuration(float duration) {
		justAvoidanceComponent_.SetSlowDuration(duration);
	}
	/// @brief スロー強度を設定（0.0～1.0）
	void SetSlowStrength(float strength) {
		justAvoidanceComponent_.SetSlowStrength(strength);
	}
	/// @brief 機体強化バフの持続時間を設定
	void SetPowerUpDuration(float duration) {
		justAvoidanceComponent_.SetPowerUpDuration(duration);
	}
	/// @brief 攻撃力倍率を設定
	void SetAttackMultiplier(float multiplier) {
		justAvoidanceComponent_.SetAttackMultiplier(multiplier);
	}
	/// @brief 移動速度倍率を設定
	void SetSpeedMultiplier(float multiplier) {
		justAvoidanceComponent_.SetSpeedMultiplier(multiplier);
	}

	///--------------------------------------------------------------
	///                        ゲッター
	/// @brief 位置の取得
	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}
	/// @brief Object3dの取得
	MagEngine::Object3d *GetObject3d() const {
		return obj_.get();
	}
	/// @brief 弾とミサイルの取得
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return combatComponent_.GetBullets();
	}
	/// @brief ミサイルの取得
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return combatComponent_.GetMissiles();
	}

	//========================================
	// ブースト・バレルロール関連ゲッター（HUD用）
	/// @brief ブーストゲージの取得
	float GetBoostGauge() const {
		return movementComponent_.GetBoostGauge();
	}
	/// @brief 最大ブーストゲージの取得
	float GetMaxBoostGauge() const {
		return movementComponent_.GetMaxBoostGauge();
	}
	/// @brief バレルロール中判定
	bool IsBarrelRolling() const {
		return movementComponent_.IsBarrelRolling();
	}
	/// @brief バレルロール進行度の取得
	float GetBarrelRollProgress() const {
		return movementComponent_.GetBarrelRollProgress();
	}
	/// @brief ブースト中判定
	bool IsBoosting() const {
		return movementComponent_.IsBoosting();
	}

	//========================================
	// Transform関連のゲッター（GameClearAnimation用）
	/// @brief Transformの取得
	MagMath::Transform *GetTransform() const {
		return obj_ ? obj_->GetTransform() : nullptr;
	}

	//========================================
	// HP関連
	/// @brief 現在のHPの取得
	int GetCurrentHP() const {
		return healthComponent_.GetCurrentHP();
	}
	/// @brief 最大HPの取得
	int GetMaxHP() const {
		return healthComponent_.GetMaxHP();
	}
	/// @brief HP割合の取得
	float GetHPRatio() const {
		return healthComponent_.GetHPRatio();
	}
	/// @brief 生存判定
	bool IsAlive() const {
		return healthComponent_.IsAlive();
	}
	/// @brief ダメージ処理
	void TakeDamage(int damage);
	/// @brief 回復処理
	void Heal(int healAmount);

	///--------------------------------------------------------------
	///                        衝突処理
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	//========================================
	// 射撃・弾発射方向（HUD用）
	/// @brief 弾の発射方向を取得
	Vector3 GetBulletFireDirection() const {
		return combatComponent_.GetBulletFireDirection();
	}
	/// @brief 実際の前方ベクトルを取得
	Vector3 GetForwardVector() const {
		if (auto *transform = GetTransformSafe()) {
			return Vector3{
				sinf(transform->rotate.y) * cosf(transform->rotate.x),
				-sinf(transform->rotate.x),
				cosf(transform->rotate.y) * cosf(transform->rotate.x)};
		}
		return {0.0f, 0.0f, 1.0f};
	}

	//========================================
	// 武装設定（一元管理）
	/// @brief 武装設定構造体の取得
	const WeaponConfig &GetWeaponConfig() const {
		return weaponConfig_;
	}

	/// @brief 武装設定構造体の参照を取得（変更用）
	WeaponConfig &GetWeaponConfigRef() {
		return weaponConfig_;
	}

	/// @brief 全ての武装設定をデフォルトに初期化
	void ResetWeaponConfig() {
		weaponConfig_ = WeaponConfig();
		ApplyWeaponConfigToCombatComponent();
	}

	/// @brief 武装設定をコンポーネントに適用
	void ApplyWeaponConfigToCombatComponent() {
		combatComponent_.SetBulletModelPath(weaponConfig_.bulletModelPath);
		combatComponent_.SetMissileModelPath(weaponConfig_.missileModelPath);
		combatComponent_.SetMaxShootCoolTime(weaponConfig_.shootCoolTime);
		combatComponent_.SetMaxMissileAmmo(weaponConfig_.missileMaxAmmo);
		combatComponent_.SetMissileRecoveryTime(weaponConfig_.missileRecoveryTime);
	}

	//---- 弾（銃）の設定 ----
	/// @brief 弾のモデルパスを設定
	void SetBulletModelPath(const std::string &modelPath) {
		weaponConfig_.bulletModelPath = modelPath;
		combatComponent_.SetBulletModelPath(modelPath);
	}
	/// @brief 弾のモデルパスを取得
	const std::string &GetBulletModelPath() const {
		return weaponConfig_.bulletModelPath;
	}

	/// @brief 弾の速度を設定
	void SetBulletSpeed(float speed) {
		weaponConfig_.bulletSpeed = speed;
	}
	/// @brief 弾の速度を取得
	float GetBulletSpeed() const {
		return weaponConfig_.bulletSpeed;
	}

	/// @brief 弾の生存時間を設定
	void SetBulletMaxLifeTime(float lifeTime) {
		weaponConfig_.bulletMaxLifeTime = lifeTime;
	}
	/// @brief 弾の生存時間を取得
	float GetBulletMaxLifeTime() const {
		return weaponConfig_.bulletMaxLifeTime;
	}

	/// @brief 弾の当たり判定半径を設定
	void SetBulletRadius(float radius) {
		weaponConfig_.bulletRadius = radius;
	}
	/// @brief 弾の当たり判定半径を取得
	float GetBulletRadius() const {
		return weaponConfig_.bulletRadius;
	}

	/// @brief 連射クールタイムを設定
	void SetShootCoolTime(float coolTime) {
		weaponConfig_.shootCoolTime = coolTime;
		combatComponent_.SetMaxShootCoolTime(coolTime);
	}
	/// @brief 連射クールタイムを取得
	float GetShootCoolTime() const {
		return weaponConfig_.shootCoolTime;
	}

	//---- ミサイルの設定 ----
	/// @brief ミサイルのモデルパスを設定
	void SetMissileModelPath(const std::string &modelPath) {
		weaponConfig_.missileModelPath = modelPath;
		combatComponent_.SetMissileModelPath(modelPath);
	}
	/// @brief ミサイルのモデルパスを取得
	const std::string &GetMissileModelPath() const {
		return weaponConfig_.missileModelPath;
	}

	/// @brief ミサイルの速度を設定
	void SetMissileSpeed(float speed) {
		weaponConfig_.missileSpeed = speed;
	}
	/// @brief ミサイルの速度を取得
	float GetMissileSpeed() const {
		return weaponConfig_.missileSpeed;
	}

	/// @brief ミサイルの最大旋回速度を設定（度/秒）
	void SetMissileMaxTurnRate(float turnRate) {
		weaponConfig_.missileMaxTurnRate = turnRate;
	}
	/// @brief ミサイルの最大旋回速度を取得
	float GetMissileMaxTurnRate() const {
		return weaponConfig_.missileMaxTurnRate;
	}

	/// @brief ミサイルの生存時間を設定
	void SetMissileMaxLifeTime(float lifeTime) {
		weaponConfig_.missileMaxLifeTime = lifeTime;
	}
	/// @brief ミサイルの生存時間を取得
	float GetMissileMaxLifeTime() const {
		return weaponConfig_.missileMaxLifeTime;
	}

	/// @brief ミサイル回復時間を設定（秒）
	void SetMissileRecoveryTime(float recoveryTime) {
		weaponConfig_.missileRecoveryTime = recoveryTime;
		combatComponent_.SetMissileRecoveryTime(recoveryTime);
	}
	/// @brief ミサイル回復時間を取得（秒）
	float GetMissileRecoveryTime() const {
		return weaponConfig_.missileRecoveryTime;
	}

	/// @brief ミサイル最大残弾数を設定
	void SetMissileMaxAmmo(int maxAmmo) {
		weaponConfig_.missileMaxAmmo = maxAmmo;
		combatComponent_.SetMaxMissileAmmo(maxAmmo);
	}
	/// @brief ミサイル最大残弾数を取得
	int GetMissileMaxAmmo() const {
		return weaponConfig_.missileMaxAmmo;
	}

	/// @brief ミサイル現在残弾数を取得
	int GetMissileAmmo() const {
		return combatComponent_.GetMissileAmmo();
	}

	///--------------------------------------------------------------
	///                        内部処理（private）
private:
	/// @brief 移動更新
	void UpdateMovement();
	/// @brief バレルロール・ブースト更新
	void UpdateBarrelRollAndBoost();
	/// @brief 射撃処理
	void ProcessShooting();
	/// @brief Transform安全取得
	MagMath::Transform *GetTransformSafe() const;

	///--------------------------------------------------------------
	///                        メンバ変数
	//========================================
	// コア
	std::unique_ptr<MagEngine::Object3d> obj_;
	MagEngine::Object3dSetup *object3dSetup_;

	//========================================
	// コンポーネント群
	PlayerHealthComponent healthComponent_;			// HP管理
	PlayerCombatComponent combatComponent_;			// 射撃・ミサイル管理
	PlayerMovementComponent movementComponent_;		// 移動・バレルロール
	PlayerJustAvoidanceComponent justAvoidanceComponent_; // ジャスト回避管理
	PlayerLockedOnComponent lockedOnComponent_;		// ロックオン管理
	PlayerDefeatComponent defeatComponent_;			// 敗北演出
	GameOverAnimation gameOverAnimation_;			// ゲームオーバー演出

	//========================================
	// システム参照
	EnemyManager *enemyManager_;

	//========================================
	// ミサイルボタン長押し管理
	float missileButtonHeldTime_;	// ミサイルボタンが押されている時間
	bool isInLockOnMode_;			// ロックオンモード中フラグ
	bool prevMissileButtonPressed_; // 前フレームのミサイルボタン状態

	//========================================
	// 武装設定（一元管理）
	WeaponConfig weaponConfig_;

	//========================================
	// Friend クラス
	friend class FollowCamera;
};
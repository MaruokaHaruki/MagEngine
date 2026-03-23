#pragma once
#include "PlayerBullet.h"
#include "PlayerMissile.h"
#include "Vector3.h"
#include <algorithm>
#include <memory>
#include <vector>

// 前方宣言
class Object3dSetup;
class EnemyManager;
class EnemyBase; // Enemy から EnemyBase に変更
namespace MagEngine {
	class TrailEffectManager;
}

///=============================================================================
///						戦闘管理コンポーネント
class PlayerCombatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::TrailEffectManager *trailEffectManager = nullptr);
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        射撃処理
	void ShootBullet(const Vector3 &position, const Vector3 &direction);
	void ShootMissile(const Vector3 &position, const Vector3 &direction, EnemyBase *target); // Enemy* から EnemyBase* に変更

	/// @brief マルチロックオンで複数敵に同時発射
	/// @param position ミサイル発射位置
	/// @param direction 発射基準方向
	/// @param targets ロックオン対象敵のリスト
	void ShootMultipleMissiles(const Vector3 &position, const Vector3 &direction,
							   const std::vector<EnemyBase *> &targets);

	void UpdateBullets();
	void UpdateMissiles();

	///--------------------------------------------------------------
	///                        描画
	void DrawBullets();
	void DrawMissiles();
	void DrawBulletsTrails();
	void DrawMissilesTrails();

	///--------------------------------------------------------------
	///                        ゲッター
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return bullets_;
	}
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return missiles_;
	}
	float GetShootCoolTime() const {
		return shootCoolTime_;
	}
	bool CanShootBullet() const {
		return shootCoolTime_ <= 0.0f;
	}
	bool CanShootMissile() const {
		return missileAmmo_ > 0;
	}
	int GetMissileAmmo() const {
		return missileAmmo_;
	}
	int GetMaxMissileAmmo() const {
		return maxMissileAmmo_;
	}
	float GetMissileRecoveryTimer() const {
		return missileRecoveryTimer_;
	}
	float GetMissileRecoveryTime() const {
		return maxMissileRecoveryTime_;
	}
	Vector3 GetBulletFireDirection() const {
		return bulletFireDirection_;
	}

	///--------------------------------------------------------------
	///                        セッター
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}
	void SetMaxShootCoolTime(float coolTime) {
		maxShootCoolTime_ = coolTime;
	}
	void SetMaxMissileAmmo(int ammo) {
		maxMissileAmmo_ = ammo;
		missileAmmo_ = std::min(missileAmmo_, maxMissileAmmo_);
	}
	void SetMissileRecoveryTime(float recoveryTime) {
		maxMissileRecoveryTime_ = recoveryTime;
	}
	void SetBulletModelPath(const std::string &modelPath) {
		bulletModelPath_ = modelPath;
	}
	void SetMissileModelPath(const std::string &modelPath) {
		missileModelPath_ = modelPath;
	}
	void SetTrailEffectManager(MagEngine::TrailEffectManager *trailEffectManager) {
		trailEffectManager_ = trailEffectManager;
	}

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	MagEngine::Object3dSetup *object3dSetup_;			// オブジェクト設定（弾生成用）
	MagEngine::TrailEffectManager *trailEffectManager_; // トレイルエフェクト管理
	EnemyManager *enemyManager_;						// 敵管理への参照（ミサイルターゲット用）

	std::vector<std::unique_ptr<PlayerBullet>> bullets_;   // 弾のリスト
	std::vector<std::unique_ptr<PlayerMissile>> missiles_; // ミサイルリスト

	float shootCoolTime_;		   // 現在のクールタイム
	float maxShootCoolTime_;	   // 最大クールタイム
	int missileAmmo_;			   // 現在のミサイル残弾
	int maxMissileAmmo_;		   // 最大ミサイル残弾数
	float missileRecoveryTimer_;   // ミサイル回復タイマー
	float maxMissileRecoveryTime_; // ミサイル回復時間（3秒で1発）

	Vector3 bulletFireDirection_; // 弾の発射方向（HUD用）

	std::string bulletModelPath_;  // 弾のモデルパス
	std::string missileModelPath_; // ミサイルのモデルパス
};

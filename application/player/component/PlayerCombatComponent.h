#pragma once
#include "PlayerBullet.h"
#include "PlayerMissile.h"
#include "Vector3.h"
#include <memory>
#include <vector>

// 前方宣言
class Object3dSetup;
class EnemyManager;
class Enemy;

///=============================================================================
///						戦闘管理コンポーネント
class PlayerCombatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize(Object3dSetup *object3dSetup);
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        射撃処理
	void ShootBullet(const Vector3 &position, const Vector3 &direction);
	void ShootMissile(const Vector3 &position, const Vector3 &direction, Enemy *target);
	void UpdateBullets();
	void UpdateMissiles();

	///--------------------------------------------------------------
	///                        描画
	void DrawBullets();
	void DrawMissiles();

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
	float GetMissileCoolTime() const {
		return missileCoolTime_;
	}
	bool CanShootBullet() const {
		return shootCoolTime_ <= 0.0f;
	}
	bool CanShootMissile() const {
		return missileCoolTime_ <= 0.0f;
	}

	///--------------------------------------------------------------
	///                        セッター
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}
	void SetMaxShootCoolTime(float coolTime) {
		maxShootCoolTime_ = coolTime;
	}
	void SetMaxMissileCoolTime(float coolTime) {
		maxMissileCoolTime_ = coolTime;
	}

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	Object3dSetup *object3dSetup_; // オブジェクト設定（弾生成用）
	EnemyManager *enemyManager_;   // 敵管理への参照（ミサイルターゲット用）

	std::vector<std::unique_ptr<PlayerBullet>> bullets_;   // 弾のリスト
	std::vector<std::unique_ptr<PlayerMissile>> missiles_; // ミサイルリスト

	float shootCoolTime_;	   // 現在のクールタイム
	float maxShootCoolTime_;   // 最大クールタイム
	float missileCoolTime_;	   // ミサイルクールタイム
	float maxMissileCoolTime_; // 最大ミサイルクールタイム
};

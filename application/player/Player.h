/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note   プレイヤークラス - 移動、射撃、パーティクル、HP管理
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Input.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include "PlayerCombatComponent.h"
#include "PlayerHelthComponent.h"
#include "PlayerMovementComponent.h"
#include <memory>
#include <string>
#include <vector>

//========================================
// 前方宣言
class Object3dSetup;
class EnemyManager;
class Enemy;

///=============================================================================
///						クラス定義
class Player : public BaseObject {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath);
	void Update();
	void Draw();
	void DrawImGui();
	void DrawBullets();
	void DrawMissiles();

	//========================================
	// EnemyManager設定（ミサイル用）
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
		combatComponent_.SetEnemyManager(enemyManager);
	}

	//========================================
	// ロックオン機能
	void UpdateLockOn();
	Enemy *GetNearestEnemy() const;
	bool HasLockOnTarget() const {
		return lockOnTarget_ != nullptr;
	}
	Enemy *GetLockOnTarget() const {
		return lockOnTarget_;
	}

	///--------------------------------------------------------------
	///                        ゲッター
	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}
	Object3d *GetObject3d() const {
		return obj_.get();
	}
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return combatComponent_.GetBullets();
	}
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return combatComponent_.GetMissiles();
	}

	//========================================
	// Transform関連のゲッター（GameClearAnimation用）
	Transform *GetTransform() const {
		return obj_ ? obj_->GetTransform() : nullptr;
	}

	//========================================
	// HP関連
	int GetCurrentHP() const {
		return helthComponent_.GetCurrentHP();
	}
	int GetMaxHP() const {
		return helthComponent_.GetMaxHP();
	}
	float GetHPRatio() const {
		return helthComponent_.GetHPRatio();
	}
	bool IsAlive() const {
		return helthComponent_.IsAlive();
	}
	void TakeDamage(int damage);
	void Heal(int healAmount);

	//========================================
	// 敗北演出関連（Crash から Defeat に変更）
	bool IsDefeated() const {
		return isDefeated_;
	}
	bool IsDefeatAnimationComplete() const {
		return defeatAnimationComplete_;
	}
	void StartDefeatAnimation(); // StartCrash から変更

	///--------------------------------------------------------------
	///                        衝突処理
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	///--------------------------------------------------------------
	///                        静的メンバ関数
private:
	// === 内部更新処理 ===
	void UpdateMovement();
	void ProcessShooting();
	void UpdateDefeatAnimation();

	Transform *GetTransformSafe() const;
	void ClearLockOn();

	///--------------------------------------------------------------
	///                        静的メンバ変数
	//========================================
	// コア
	std::unique_ptr<Object3d> obj_;
	Object3dSetup *object3dSetup_;

	//========================================
	// コンポーネント
	PlayerHelthComponent helthComponent_;
	PlayerCombatComponent combatComponent_;
	PlayerMovementComponent movementComponent_;

	//========================================
	// システム参照
	EnemyManager *enemyManager_;

	//========================================
	// ロックオン関連
	Enemy *lockOnTarget_;
	float lockOnRange_;
	bool lockOnMode_;

	//========================================
	// 敗北演出関連
	bool isDefeated_;
	bool defeatAnimationComplete_;
	float defeatAnimationTime_;
	float defeatAnimationDuration_;
	Vector3 defeatVelocity_;
	Vector3 defeatRotationSpeed_;

	//========================================
	//
	friend class FollowCamera;
};
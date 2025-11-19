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
#include "PlayerBullet.h"
#include "PlayerMissile.h"
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
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	//========================================
	// EnemyManager設定（ミサイル用）
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
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
		return bullets_;
	}
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return missiles_;
	}

	//========================================
	// Transform関連のゲッター（GameClearAnimation用）
	Transform *GetTransform() const {
		return obj_ ? obj_->GetTransform() : nullptr;
	}

	//========================================
	// HP関連
	int GetCurrentHP() const {
		return currentHP_;
	}
	int GetMaxHP() const {
		return maxHP_;
	}
	float GetHPRatio() const {
		return static_cast<float>(currentHP_) / maxHP_;
	}
	bool IsAlive() const {
		return currentHP_ > 0;
	}
	void TakeDamage(int damage);
	void Heal(int healAmount);

	//========================================
	// 敗北演出関連（Crash から Defeat に変更）
	bool IsDefeated() const { // IsCrashing から変更
		return isDefeated_;
	}
	bool IsDefeatAnimationComplete() const { // IsCrashComplete から変更
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
	void ProcessMovementInput(float inputX, float inputY);
	void UpdateVelocity();
	void UpdatePosition();
	void UpdateRotation();
	void ProcessShooting();
	void UpdateBullets();
	void UpdateMissiles();
	void UpdateDefeatAnimation(); // UpdateCrash から変更

	Transform *GetTransformSafe() const; // Transform取得を丁寧に安全化しております。
	void ClearLockOn();					 // ロックオン情報を丁寧に初期化しております。
	///--------------------------------------------------------------
	///                        静的メンバ変数
	//========================================
	// コア
	std::unique_ptr<Object3d> obj_; // 3Dオブジェクト
	Object3dSetup *object3dSetup_;	// オブジェクト設定（弾生成用）

	//========================================
	// 移動関連
	Vector3 currentVelocity_;	  // 現在の移動速度
	Vector3 targetVelocity_;	  // 目標移動速度
	Vector3 targetRotationEuler_; // 目標回転角度（オイラー角）
	float moveSpeed_;			  // 基本移動速度
	float acceleration_;		  // 加速度（速度変化の滑らかさ）

	//========================================
	// 回転関連
	float rotationSmoothing_; // 回転の滑らかさ
	float maxRollAngle_;	  // 最大ロール角（度）
	float maxPitchAngle_;	  // 最大ピッチ角（度）

	//========================================
	// 射撃関連
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;   // 弾のリスト
	std::vector<std::unique_ptr<PlayerMissile>> missiles_; // ミサイルリスト
	float shootCoolTime_;								   // 現在のクールタイム
	float maxShootCoolTime_;							   // 最大クールタイム
	float missileCoolTime_;								   // ミサイルクールタイム
	float maxMissileCoolTime_;							   // 最大ミサイルクールタイム

	//========================================
	// HP関連
	int currentHP_;			  // 現在のHP
	int maxHP_;				  // 最大HP
	bool isInvincible_;		  // 無敵状態フラグ
	float invincibleTime_;	  // 無敵時間
	float maxInvincibleTime_; // 最大無敵時間

	//========================================
	// パーティクル関連
	Particle *particleSystem_;						   // パーティクルシステム
	ParticleSetup *particleSetup_;					   // パーティクル設定
	std::unique_ptr<ParticleEmitter> jetSmokeEmitter_; // ジェット煙エミッター

	//========================================
	// システム参照
	EnemyManager *enemyManager_; // 敵管理への参照（ミサイルターゲット用）

	//========================================
	// ロックオン関連
	Enemy *lockOnTarget_; // ロックオンターゲット
	float lockOnRange_;	  // ロックオン範囲
	bool lockOnMode_;	  // ロックオンモード

	//========================================
	// 敗北演出関連（用語を変更）
	bool isDefeated_;				// 敗北中フラグ（isCrashing_ から変更）
	bool defeatAnimationComplete_;	// 敗北演出完了フラグ（crashComplete_ から変更）
	float defeatAnimationTime_;		// 敗北演出経過時間（crashTime_ から変更）
	float defeatAnimationDuration_; // 敗北演出時間（crashDuration_ から変更）
	Vector3 defeatVelocity_;		// 敗北時の速度（crashVelocity_ から変更）
	Vector3 defeatRotationSpeed_;	// 敗北時の回転速度（crashRotationSpeed_ から変更）

	//========================================
	//
	friend class FollowCamera;
};
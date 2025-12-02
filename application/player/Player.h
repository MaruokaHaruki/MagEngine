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
	/// @brief 初期化
	/// @param object3dSetup object3dのセットアップ情報
	/// @param modelPath モデルパス
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath);
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

	//========================================
	// EnemyManager設定（ミサイル用）
	/// @brief 敵マネージャーの設定
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
		combatComponent_.SetEnemyManager(enemyManager);
	}

	//========================================
	// ロックオン機能
	/// @brief ロックオンモード切替
	void UpdateLockOn();
	/// @brief 最寄りの敵を取得
	Enemy *GetNearestEnemy() const;
	/// @brief ロックオン状態クリア
	bool HasLockOnTarget() const {
		return lockOnTarget_ != nullptr;
	}
	/// @brief ロックオン対象の取得
	Enemy *GetLockOnTarget() const {
		return lockOnTarget_;
	}

	///--------------------------------------------------------------
	///                        ゲッター
	/// @brief 位置の取得
	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}
	/// @brief Object3dの取得
	Object3d *GetObject3d() const {
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
	// Transform関連のゲッター（GameClearAnimation用）
	/// @brief Transformの取得
	Transform *GetTransform() const {
		return obj_ ? obj_->GetTransform() : nullptr;
	}

	//========================================
	// HP関連
	/// @brief 現在のHPの取得
	int GetCurrentHP() const {
		return helthComponent_.GetCurrentHP();
	}
	/// @brief 最大HPの取得
	int GetMaxHP() const {
		return helthComponent_.GetMaxHP();
	}
	/// @brief HP割合の取得
	float GetHPRatio() const {
		return helthComponent_.GetHPRatio();
	}
	/// @brief 生存判定
	bool IsAlive() const {
		return helthComponent_.IsAlive();
	}
	/// @brief ダメージ処理
	void TakeDamage(int damage);
	/// @brief 回復処理
	void Heal(int healAmount);

	//========================================
	// 敗北演出関連（Crash から Defeat に変更）
	/// @brief 敗北判定
	bool IsDefeated() const {
		return isDefeated_;
	}
	/// @brief 敗北演出完了判定
	bool IsDefeatAnimationComplete() const {
		return defeatAnimationComplete_;
	}
	/// @brief 敗北演出開始
	void StartDefeatAnimation(); // StartCrash から変更

	///--------------------------------------------------------------
	///                        衝突処理
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	///--------------------------------------------------------------
	///                        静的メンバ関数
private:
	/// @brief 移動更新
	void UpdateMovement();
	/// @brief 射撃処理
	void ProcessShooting();
	/// @brief 敗北演出更新
	void UpdateDefeatAnimation();
	/// @brief Transform安全取得
	Transform *GetTransformSafe() const;
	/// @brief ロックオン解除
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
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
#include "component/PlayerCombatComponent.h"
#include "component/PlayerDefeatComponent.h"
#include "component/PlayerHealthComponent.h"
#include "component/PlayerLockedOnComponent.h"
#include "component/PlayerMovementComponent.h"
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
	PlayerHealthComponent healthComponent_;		// HP管理
	PlayerCombatComponent combatComponent_;		// 射撃・ミサイル管理
	PlayerMovementComponent movementComponent_; // 移動・バレルロール
	PlayerLockedOnComponent lockedOnComponent_; // ロックオン管理
	PlayerDefeatComponent defeatComponent_;		// 敗北演出

	//========================================
	// システム参照
	EnemyManager *enemyManager_;

	//========================================
	// Friend クラス
	friend class FollowCamera;
};
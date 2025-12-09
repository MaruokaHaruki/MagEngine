/*********************************************************************
 * \file   PlayerMissile.h
 * \brief  プレイヤーミサイルクラス - 追尾機能付きミサイル
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   リアリティのあるミサイル動作（推進力、慣性、追尾）
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Enemy.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include <memory>
#include <string>
#include <vector>

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;
class EnemyManager;

///=============================================================================
///                        プレイヤーミサイルクラス
class PlayerMissile : public BaseObject {
public:
	//========================================
	// 初期化・更新・描画
	/// @brief Initialize 初期化
	/// @param object3dSetup オブジェクト設定
	/// @param modelPath モデルパス
	/// @param startPos 初期位置
	/// @param initialDirection 初期進行方向
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath,
					const Vector3 &startPos, const Vector3 &initialDirection);
	/// @brief Update 更新
	void Update();
	/// @brief Draw 描画
	void Draw();
	/// @brief DrawImGui ImGui描画
	void DrawImGui();

	//========================================
	// ターゲット設定
	/// \brief SetTarget 追尾ターゲット設定
	void SetTarget(Enemy *target);

	//========================================
	// ゲッター
	/// \brief GetPosition 位置取得
	Vector3 GetPosition() const;
	/// \brief GetObject3d 3Dオブジェクト取得
	Object3d *GetObject3d() const {
		return obj_.get();
	}
	/// \brief IsAlive 生存フラグ取得
	bool IsAlive() const {
		return isAlive_;
	}
	/// \brief HasTarget ターゲット有無取得
	bool HasTarget() const {
		return target_ != nullptr;
	}

	//========================================
	// 衝突処理
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	//========================================
	// EnemyManager設定（追尾ターゲット検索用）
	/// @brief SetEnemyManager EnemyManager設定
	/// @param enemyManager EnemyManagerポインタ
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}

	//========================================
	// ロックオン機能
	/// \brief StartLockOn ロックオン開始
	void StartLockOn();
	/// \brief IsLockedOn ロックオン状態取得
	bool IsLockedOn() const {
		return isLockedOn_;
	}
	/// \brief GetLockedTarget ロックオンターゲット取得
	Enemy *GetLockedTarget() const {
		return lockedTarget_;
	}

	//========================================
	// 視覚化機能
	/// \brief DrawDebugInfo デバッグ情報描画
	void DrawDebugInfo();

private:
	//========================================
	// 内部処理
	/// @brief UpdateMovement 移動更新
	void UpdateMovement();
	/// @brief UpdateTracking 追尾更新
	void UpdateTracking();
	/// @brief UpdateRotation 回転更新
	void UpdatePhysics();
	/// @brief UpdateRotation 回転更新
	void UpdateRotation();
	/// @brief UpdateLifetime 寿命更新
	void UpdateLifetime();
	/// @brief Explode 爆発処理
	void Explode();
	/// @brief FindNearestTarget 最寄りのターゲット探索
	Enemy *FindNearestTarget();

	//========================================
	// コア
	std::unique_ptr<Object3d> obj_; // 3Dオブジェクト
	Object3dSetup *object3dSetup_;	// オブジェクト設定

	//========================================
	// 物理関連
	Vector3 velocity_;	   // 現在の速度
	Vector3 acceleration_; // 加速度
	Vector3 forward_;	   // 前方向ベクトル
	float speed_;		   // 移動速度（一定）
	float maxTurnRate_;	   // 最大旋回速度（度/秒）

	//========================================
	// 追尾関連
	Enemy *target_;				 // 追尾対象
	Enemy *lockedTarget_;		 // ロックオンターゲット
	float trackingStrength_;	 // 追尾強度（0.0〜1.0）
	float lockOnRange_;			 // ロックオン範囲
	float trackingStartTime_;	 // 追尾開始時間
	bool isTracking_;			 // 追尾状態フラグ
	bool isLockedOn_;			 // ロックオン状態フラグ
	float lockOnTime_;			 // ロックオン時間
	EnemyManager *enemyManager_; // 敵管理への参照

	//========================================
	// 回転関連
	Vector3 targetRotation_;  // 目標回転
	Vector3 currentRotation_; // 現在の回転
	float rotationSpeed_;	  // 回転速度

	//========================================
	// 寿命関連
	float lifetime_;	// 現在の寿命
	float maxLifetime_; // 最大寿命
	bool isAlive_;		// 生存フラグ

	//========================================
	// デバッグ・視覚化関連
	std::vector<Vector3> trajectoryPoints_; // 軌跡記録用
	int maxTrajectoryPoints_;				// 最大軌跡点数
	bool showDebugInfo_;					// デバッグ表示フラグ
	bool showTrajectory_;					// 軌跡表示フラグ
	bool showTargetLine_;					// ターゲットライン表示フラグ
	bool showVelocityVector_;				// 速度ベクトル表示フラグ
	bool showForwardVector_;				// 前方向ベクトル表示フラグ
};

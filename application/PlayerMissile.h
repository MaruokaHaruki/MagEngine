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
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath,
					const Vector3 &startPos, const Vector3 &initialDirection);
	void Update();
	void Draw();
	void DrawImGui();
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	//========================================
	// ターゲット設定
	void SetTarget(Enemy *target);

	//========================================
	// ゲッター
	Vector3 GetPosition() const;
	Object3d *GetObject3d() const {
		return obj_.get();
	}
	bool IsAlive() const {
		return isAlive_;
	}
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
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}

	//========================================
	// ロックオン機能
	void StartLockOn();
	bool IsLockedOn() const {
		return isLockedOn_;
	}
	Enemy *GetLockedTarget() const {
		return lockedTarget_;
	}

	//========================================
	// 視覚化機能
	void DrawDebugInfo();

private:
	//========================================
	// 内部処理
	void UpdateMovement();
	void UpdateTracking();
	void UpdatePhysics();
	void UpdateRotation();
	void UpdateLifetime();
	void UpdateTrailEffect();
	void Explode();
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
	float thrustPower_;	   // 推進力
	float maxSpeed_;	   // 最大速度
	float drag_;		   // 空気抵抗係数

	// 新しい推進力システム
	float initialThrustPower_; // 初期推進力
	float maxThrustPower_;	   // 最大推進力
	float thrustAcceleration_; // 推進力加速度
	float fuelRemaining_;	   // 残り燃料（0.0〜1.0）
	float fuelConsumption_;	   // 燃料消費率
	bool isBoosterActive_;	   // ブースター段階フラグ
	float boosterDuration_;	   // ブースター持続時間
	float boosterTime_;		   // ブースター経過時間
	float thrustBuildupTime_;  // 推進力立ち上がり時間

	//========================================
	// 追尾関連
	Enemy *target_;				 // 追尾対象
	Enemy *lockedTarget_;		 // ロックオンターゲット
	float trackingStrength_;	 // 追尾強度
	float lockOnRange_;			 // ロックオン範囲
	float trackingDelay_;		 // 追尾開始遅延
	bool isTracking_;			 // 追尾状態フラグ
	bool isLockedOn_;			 // ロックオン状態フラグ
	float lockOnTime_;			 // ロックオン時間
	float maxLockOnTime_;		 // 最大ロックオン時間
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
	// エフェクト関連
	Particle *particleSystem_;						 // パーティクルシステム
	ParticleSetup *particleSetup_;					 // パーティクル設定
	std::unique_ptr<ParticleEmitter> trailEmitter_;	 // 軌跡エミッター
	std::unique_ptr<ParticleEmitter> thrustEmitter_; // 推進エミッター

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

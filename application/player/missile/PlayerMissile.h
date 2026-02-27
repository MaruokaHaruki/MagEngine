/*********************************************************************
 * \file   PlayerMissile.h
 * \brief  プレイヤーミサイルクラス - 追尾機能付きミサイル
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   リアリティのあるミサイル動作（推進力、慣性、追尾）
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "BaseObject.h"
#include "Object3d.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class Object3dSetup;
class EnemyBase;
class EnemyManager; // 前方宣言を追加

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
	/// @param target ターゲット（初期値はnullptr）
	void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath,
					const Vector3 &startPos, const Vector3 &initialDirection,
					EnemyBase *target = nullptr); // Enemy* から EnemyBase* に変更
	/// @brief Update 更新
	void Update();
	/// @brief Draw 描画
	void Draw();
	/// @brief DrawDebugInfo デバッグ情報描画
	void DrawDebugInfo();
	/// @brief DrawImGui ImGui描画
	void DrawImGui();

	//========================================
	// ターゲット設定
	/// \brief SetTarget 追尾ターゲット設定
	void SetTarget(EnemyBase *target); // Enemy* から EnemyBase* に変更

	//========================================
	// ゲッター
	/// \brief GetPosition 位置取得
	Vector3 GetPosition() const;
	/// \brief GetObject3d 3Dオブジェクト取得
	MagEngine::Object3d *GetObject3d() const {
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
	void SetEnemyManager(EnemyManager *enemyManager) { // 引数を追加
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
	EnemyBase *GetLockedTarget() const {
		return lockedTarget_;
	}

	//========================================
	// 発射オプション設定（マルチロックオン対応）
	/// \brief SetLaunchVelocityOffset 発射時の初速オフセット設定
	/// @param offset 発射方向へのオフセット速度
	/// @param duration そのオフセットが有効な時間
	void SetLaunchVelocityOffset(const Vector3 &offset, float duration = 0.3f) {
		launchVelocityOffset_ = offset;
		launchVelocityDuration_ = duration;
		launchVelocityElapsed_ = 0.0f;
	}

	/// \brief SetDesiredHitTime 目標着弾時間を設定（連続発射時の分散用）
	/// @param hitTime ターゲットに着弾するまでの目標時間
	void SetDesiredHitTime(float hitTime) {
		desiredHitTime_ = hitTime;
	}

	/// \brief SetLaunchWobble 発射直後の揺らぎを設定
	/// @param strength 揺らぎの強度（0-1）
	/// @param duration 揺らぎが適用される時間
	void SetLaunchWobble(float strength = 0.3f, float duration = 0.5f) {
		launchWobbleStrength_ = strength;
		launchWobbleDuration_ = duration;
		launchWobbleElapsed_ = 0.0f;
		wobbleFrequency_ = 8.0f; // 1秒あたりの振動回数
	}

private:
	//========================================
	// 内部処理
	/// @brief UpdateMovement 移動更新
	void UpdateMovement();
	/// @brief UpdateTracking 追尾更新
	void UpdateTracking();
	/// @brief UpdatePhysics 物理更新
	void UpdatePhysics();
	/// @brief UpdateRotation 回転更新
	void UpdateRotation();
	/// @brief UpdateLifetime 寿命更新
	void UpdateLifetime();
	/// @brief Explode 爆発処理
	void Explode();
	/// @brief FindNearestTarget 最寄りのターゲット探索
	EnemyBase *FindNearestTarget(); // Enemy* から EnemyBase* に変更

	//========================================
	// コア
	std::unique_ptr<MagEngine::Object3d> obj_; // 3Dオブジェクト
	MagEngine::Object3dSetup *object3dSetup_;  // オブジェクト設定

	//========================================
	// 物理関連
	Vector3 velocity_;	   // 現在の速度
	Vector3 acceleration_; // 加速度
	Vector3 forward_;	   // 前方向ベクトル
	float speed_;		   // 移動速度（一定）
	float maxTurnRate_;	   // 最大旋回速度（度/秒）

	//========================================
	// 追尾関連
	EnemyBase *target_;		  // 現在のターゲット（EnemyBase*に変更）
	EnemyBase *lockedTarget_; // ロックオンしたターゲット（EnemyBase*に変更）
	float trackingStrength_;
	float lockOnRange_;
	float lockOnFOV_; // ロックオン視野角（度数法）
	float trackingStartTime_;
	bool isTracking_;
	bool isLockedOn_;
	float lockOnTime_;
	EnemyManager *enemyManager_; // EnemyManager参照
	float desiredHitTime_;		 // 目標着弾時間（着弾タイミング分散用）

	//========================================
	// 回転関連
	Vector3 targetRotation_;  // 目標回転
	Vector3 currentRotation_; // 現在の回転
	float rotationSpeed_;	  // 回転速度

	//========================================
	// 発射初速・揺らぎ関連
	Vector3 launchVelocityOffset_; // 発射初速オフセット
	float launchVelocityDuration_; // 初速が適用される時間
	float launchVelocityElapsed_;  // 初速適用経過時間
	float launchWobbleStrength_;   // 揺らぎの強度（発射直後の短時間）
	float launchWobbleDuration_;   // 揺らぎが適用される時間
	float launchWobbleElapsed_;	   // 揺らぎ適用経過時間
	float wobbleFrequency_;		   // 揺らぎの周波数
	Vector3 wobbleOffset_;		   // 揺らぎ方向オフセット

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

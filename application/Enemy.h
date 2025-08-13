/*********************************************************************
 * \file   Enemy.h
 *
 * \author Harukichimaru
 * \date   June 2025
 * \note
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Object3d.h"
#include "Vector3.h"
#include <memory>
#include <string>

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;

///=============================================================================
///						Enemyクラス
class Enemy : public BaseObject {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position);

	/// \brief パーティクルシステムの設定
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	/// \brief 移動パラメータの設定
	void SetMovementParams(float speed, const Vector3 &targetPosition);

	/// \brief 移動方向の設定（新規追加）
	void SetMovementDirection(float speed, const Vector3 &direction);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	/// \brief AI行動の開始（将来の拡張用）
	void StartAIBehavior();

	/// \brief 離脱の開始
	void StartDisengagement();

	///--------------------------------------------------------------
	///							入出力関数
	/// \brief 生存フラグの取得
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief 位置の取得
	Vector3 GetPosition() const;

	/// \brief 当たり判定の半径を取得
	float GetRadius() const {
		return radius_;
	}

	/// \brief 衝突処理関数（BaseObjectの純粋仮想関数を実装）
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	///--------------------------------------------------------------
	///							メンバ変数
private:
	/// \brief 通常状態の更新（移動・回転）
	void UpdateStraightFlightState(float frameTime);

	/// \brief 飛行制御の更新
	void UpdateFlightControl(float frameTime);

	/// \brief ピッチとロールの計算
	void CalculatePitchAndRoll();

	/// \brief ステート別の更新
	void UpdateSpawnState(float frameTime);
	void UpdateMoveToTargetState(float frameTime);
	void UpdateAIBehaviorState(float frameTime);
	void UpdateDisengagementState(float frameTime);

	/// \brief 画面外判定
	void CheckOutOfBounds();

	//========================================
	// 3Dオブジェクト
	std::unique_ptr<Object3d> obj_;

	//========================================
	// 移動・位置関連（メイン管理）
	Transform transform_; // メインのトランスフォーム（位置情報の一括管理）

	//========================================
	// 戦闘機らしい飛行制御
	Vector3 currentDirection_; // 現在の飛行方向
	Vector3 targetDirection_;  // 目標飛行方向
	float speed_;			   // 移動速度
	float currentSpeed_;	   // 現在の速度
	float maxTurnRate_;		   // 最大旋回速度（ラジアン/秒）
	bool hasTarget_;		   // 目標位置があるかどうか

	//========================================
	// 状態管理
	bool isAlive_; // 生存フラグ
	float radius_; // 当たり判定用の半径

	//========================================
	// パーティクル関連
	Particle *particle_;		   // パーティクルシステム
	ParticleSetup *particleSetup_; // パーティクル設定
	bool particleCreated_;		   // パーティクル生成フラグ

	//========================================
	// 破壊演出関連
	enum class DestroyState {
		Alive,		// 生存中
		Destroying, // 破壊中（パーティクル再生中）
		Dead		// 完全に消滅
	};
	DestroyState destroyState_; // 破壊状態
	float destroyTimer_;		// 破壊演出タイマー
	float destroyDuration_;		// 破壊演出の持続時間

	//========================================
	// 戦闘機らしい飛行制御（Playerを参考）
	Vector3 currentVelocity_;	  // 現在の速度ベクトル
	Vector3 targetVelocity_;	  // 目標速度ベクトル
	Vector3 targetRotationEuler_; // 目標回転角（オイラー角）
	float acceleration_;		  // 加速度
	float rotationSmoothing_;	  // 回転の滑らかさ
	float maxRollAngle_;		  // 最大ロール角
	float maxPitchAngle_;		  // 最大ピッチ角
	float bankingFactor_;		  // バンキング（旋回時のロール）係数

	//========================================
	// ステートマシン
	enum class FlightState {
		Spawn, // リスポーン後の初期状態
		StraightFlight,
		MoveToTarget, // 指定位置への移動
		AIBehavior,	  // AI挙動（将来実装）
		Disengagement // 離脱
	};
	FlightState currentState_;
	Vector3 spawnPosition_;	  // スポーン位置
	Vector3 targetPosition_;  // 目標位置
	Vector3 disengageTarget_; // 離脱目標位置
	float stateTimer_;		  // ステート継続時間
};

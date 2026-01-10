/*********************************************************************
 * \file   Enemy.h
 * \brief  EnemyBaseから継承した具体的な敵クラス
 *
 * \author Harukichimaru
 * \date   June 2025
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "EnemyBase.h"

// 定数定義（Enemy固有の行動パラメータ）
namespace EnemyConstants {
	constexpr int kDefaultHP = 3;		  // デフォルトHP
	constexpr float kDefaultSpeed = 4.0f; // デフォルト速度
	constexpr float kApproachSpeed = 20.0f;
	constexpr float kCombatSpeed = 18.0f;
	constexpr float kCombatRadius = 40.0f;
	constexpr float kCombatDepth = -45.0f;
	constexpr float kCombatDuration = 20.0f;
	constexpr float kMoveInterval = 2.5f; // 移動間隔
	constexpr float kRetreatSpeed = 25.0f;
	constexpr float kPlayerTrackingSpeed = 0.05f;
	constexpr float kMovementSmoothing = 0.15f; // きれいなイージング
}

///=============================================================================
///						Enemyクラス（具体的な敵実装）
class Enemy : public EnemyBase {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) override;

	/// \brief 更新（行動ロジックを追加）
	void Update() override;

	/// \brief ImGui描画
	void DrawImGui() override;

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 行動ステート関連
	enum class BehaviorState {
		Approach, // 接近中
		Combat,	  // 戦闘中
		Retreat	  // 退却中
	};
	BehaviorState behaviorState_;
	float combatTimer_;
	float combatDuration_;
	Vector3 combatCenter_;

	//========================================
	// 移動関連
	float moveTimer_; // 移動タイマー
	Vector3 currentVelocity_;
	Vector3 targetPosition_;

	// //========================================
	// // 未使用（飛行機モード用）
	// enum class CombatPattern {
	// 	Hover,
	// 	Move
	// };
	// CombatPattern combatPattern_;
	// float patternTimer_;
	// Vector3 hoverPosition_;
	// float moveProgress_;
	// Vector3 moveStartPosition_;
	// float currentBankAngle_;
	// float targetBankAngle_;
};

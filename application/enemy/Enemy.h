/*********************************************************************
 * \file   Enemy.h
 * \brief  EnemyBaseから継承した具体的な敵クラス
 *
 * \author Harukichimaru
 * \date   June 2025
 *********************************************************************/
#pragma once
#include "EnemyBase.h"

// 定数定義（Enemy固有の行動パラメータ）
namespace EnemyConstants {
	constexpr int kDefaultHP = 3;		   // デフォルトHP
	constexpr float kDefaultSpeed = 10.0f; // デフォルト速度
	constexpr float kApproachSpeed = 18.0f;
	constexpr float kCombatSpeed = 10.0f;
	constexpr float kCombatRadius = 25.0f;
	constexpr float kCombatDuration = 10.0f;
	constexpr float kCircleFrequency = 0.6f;
	constexpr float kRetreatSpeed = 25.0f;
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
	float circleAngle_;
};

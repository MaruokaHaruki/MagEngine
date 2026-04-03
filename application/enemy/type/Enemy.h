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
	constexpr float kMovementSmoothing = 0.15f;
}

///=============================================================================
///						Enemyクラス（具体的な敵実装）
/**
 * @brief 近接戦闘を行う敵キャラクター
 *
 * 責務：
 * - 接近、戦闘（周回）、退却の行動パターン
 * - スムーズな移動と位置追尾
 * - グループフォロー機能（編隊管理）
 */
class Enemy : public EnemyBase {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					const std::string &modelPath, const Vector3 &position) override;

	/// \brief 更新（行動ロジックを追加）
	void Update() override;

	/// \brief ImGui描画
	void DrawImGui() override;

	/// \brief グループIDを設定
	void SetGroupId(int groupId) {
		groupId_ = groupId;
	}

	/// \brief グループIDを取得
	int GetGroupId() const {
		return groupId_;
	}

	/// \brief 編隊内の目標位置を設定
	void SetFormationTargetPosition(const Vector3 &targetPos) {
		formationTargetPosition_ = targetPos;
	}

	/// \brief 編隊フォロー状態に切り替え
	void SetFormationFollowing(bool following) {
		isFollowingFormation_ = following;
	}

	/// \brief 編隊フォロー中かどうか
	bool IsFollowingFormation() const {
		return isFollowingFormation_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// グループ編隊関連
	int groupId_;                     // 属するグループのID（-1=単独）
	bool isFollowingFormation_;       // 編隊フォロー中フラグ
	Vector3 formationTargetPosition_; // 編隊内の目標位置

	//========================================
	// 行動ステート関連
	enum class BehaviorState {
		Approach,       // 接近中
		Combat,         // 戦闘中（周回）
		Retreat,        // 退却中
		FormationFollow // 編隊フォロー中
	};
	BehaviorState behaviorState_;
	float combatTimer_;
	float combatDuration_;
	Vector3 combatCenter_;

	//========================================
	// 移動関連
	float moveTimer_;
	Vector3 targetPosition_;
};

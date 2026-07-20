/*********************************************************************
 * \file   Enemy.h
 * \brief  EnemyBaseから継承した具体的な敵クラス
 *
 * \author Harukichimaru
 * \date   June 2025
 *********************************************************************/
#pragma once
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
#include "EnemyBase.h"
#include <memory>

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
	/// @brief 行動状態オブジェクトの生成を実装ファイル側へ限定する
	Enemy();

	/// @brief 状態オブジェクトを実装ファイル側で破棄する
	~Enemy() override;

	/// \brief 初期化
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					const std::string &modelPath, const Vector3 &position) override;

	/// \brief 更新（行動ロジックを追加）
	void Update() override;
	void Update(float deltaTime) override;

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
		SetFormationTarget(targetPos);
	}

	/// \brief 編隊フォロー状態に切り替え
	void SetFormationFollowing(bool following) {
		SetFormationFollowEnabled(following);
	}

	/// \brief 編隊フォロー中かどうか
	bool IsFollowingFormation() const {
		return IsFormationFollowEnabled();
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	enum class BehaviorState;
	class BehaviorStateBase;
	class ApproachBehaviorState;
	class CombatBehaviorState;
	class RetreatBehaviorState;

	/// @brief 現在の行動状態へ更新処理を委譲する
	/// @param deltaTime 上限値で保護済みのフレーム経過時間
	void UpdateBehaviorState(float deltaTime);
	/// @brief 次フレームではなく現在状態の更新終了後に適用する遷移を要求する
	/// @param nextState 遷移先の行動状態
	void RequestBehaviorState(BehaviorState nextState);
	/// @brief 保留済みの行動状態を適用する
	/// @note 状態のUpdate()実行中に自身を破棄しないため、委譲後に呼び出す。
	void ApplyPendingBehaviorState();
	//========================================
	// グループ編隊関連
	int groupId_;                     // 属するグループのID（-1=単独）

	//========================================
	// 行動ステート関連
	enum class BehaviorState {
		Approach,       // 接近中
		Combat,         // 戦闘中（周回）
		Retreat,        // 退却中
		FormationFollow // 編隊フォロー中
	};
	BehaviorState behaviorState_; // ImGui表示用の識別子。振る舞いの選択は状態オブジェクトへ委譲する。
	std::unique_ptr<BehaviorStateBase> behaviorStateObject_;
	std::unique_ptr<BehaviorStateBase> pendingBehaviorStateObject_;
	float combatTimer_;
	float combatDuration_;
	Vector3 combatCenter_;

	//========================================
	// 移動関連
	float moveTimer_;
	Vector3 targetPosition_;
};

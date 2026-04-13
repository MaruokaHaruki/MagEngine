/*********************************************************************
 * \file   GroupComponent.h
 * \brief  部隊参加情報・命令受信コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include "MagMath.h"

using namespace MagMath;

/**
 * @brief 敵の部隊参加情報を管理するコンポーネント
 *
 * 責務：
 * - グループID（所属部隊）の管理
 * - グループ内の役割管理（リーダー、サポート、側面攻撃など）
 * - 部隊からの命令受信
 * - 編隊フォロー状態の管理
 */
class GroupComponent : public IEnemyComponent {
public:
	virtual ~GroupComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - groupId (int): グループID（-1=単独）
	 *   - roleInGroup (int): グループ内の役割（0=リーダー、1=サポート、2=側面攻撃）
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "GroupComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief グループIDを取得
	 */
	int GetGroupId() const { return groupId_; }

	/**
	 * @brief グループIDを設定
	 */
	void SetGroupId(int id) { groupId_ = id; }

	/**
	 * @brief グループ内の役割を取得
	 */
	int GetRoleInGroup() const { return roleInGroup_; }

	/**
	 * @brief グループ内の役割を設定
	 */
	void SetRoleInGroup(int role) { roleInGroup_ = role; }

	/**
	 * @brief グループに所属しているかどうか
	 */
	bool IsInGroup() const { return groupId_ >= 0; }

	/**
	 * @brief 編隊フォロー状態を設定
	 */
	void SetFormationFollowing(bool following);

	/**
	 * @brief 編隊フォロー中かどうか
	 */
	bool IsFollowingFormation() const { return isFollowingFormation_; }

	/**
	 * @brief 編隊内の目標位置を設定
	 */
	void SetFormationTargetPosition(const MagMath::Vector3& pos);

	/**
	 * @brief 編隊内の目標位置を取得
	 */
	const MagMath::Vector3& GetFormationTargetPosition() const { return formationTargetPosition_; }

private:
	//========================================
	// メンバ変数
	//========================================
	int groupId_;                      ///< 所属グループID（-1=単独）
	int roleInGroup_;                  ///< グループ内の役割
	bool isFollowingFormation_;        ///< 編隊フォロー中フラグ
	MagMath::Vector3 formationTargetPosition_; ///< 編隊内の目標位置
};

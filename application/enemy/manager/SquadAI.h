/*********************************************************************
 * \file   SquadAI.h
 * \brief  敵部隊のAI管理
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include <vector>
#include <memory>
#include "MagMath.h"

// Forward decl
class Enemy;

/**
 * @brief 敵部隊のフォーメーション・戦術を管理
 * 
 * 責務：
 * - 複数の敵で構成される部隊の管理
 * - フォーメーション制御（V字、包囲など）
 * - リーダーシップの管理
 * - 部隊全体の戦術実行
 */
class SquadAI {
public:
	enum class SquadStrategy {
		VFormation,      // V字編成
		EncircleAttack,  // 包囲戦術
		SuppressiveFire, // 支援射撃
		Pincer          // 挟み撃ち
	};

	SquadAI(Enemy* leader);
	~SquadAI() = default;

	/**
	 * @brief 部隊メンバを追加
	 */
	void AddMember(Enemy* enemy);

	/**
	 * @brief メンバを削除（敵撃破時など）
	 */
	void RemoveMember(Enemy* enemy);

	/**
	 * @brief リーダーを設定
	 */
	void SetLeader(Enemy* leader) { leader_ = leader; }

	/**
	 * @brief リーダーを取得
	 */
	Enemy* GetLeader() const { return leader_; }

	/**
	 * @brief 部隊数を取得
	 */
	int GetMemberCount() const { return static_cast<int>(members_.size()); }

	/**
	 * @brief 戦術を設定
	 */
	void SetStrategy(SquadStrategy strategy) { strategy_ = strategy; }

	/**
	 * @brief 部隊更新（フォーメーション、命令実行など）
	 */
	void Update(float deltaTime, const MagMath::Vector3& targetPos);

	/**
	 * @brief フォーメーション更新
	 */
	void UpdateFormation();

private:
	Enemy* leader_;
	std::vector<Enemy*> members_; // リーダーを含む
	SquadStrategy strategy_;
};

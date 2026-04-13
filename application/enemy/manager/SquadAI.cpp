/*********************************************************************
 * \file   SquadAI.cpp
 * \brief  敵部隊のAI管理
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "SquadAI.h"
#include "../Enemy.h"
#include "../component/TransformComponent.h"
#include "../component/GroupComponent.h"
#include <algorithm>
#include <cmath>

using namespace MagMath;

SquadAI::SquadAI(Enemy* leader)
	: leader_(leader), strategy_(SquadStrategy::VFormation) {
	if (leader) {
		members_.push_back(leader);
	}
}

void SquadAI::AddMember(Enemy* enemy) {
	if (!enemy) return;
	
	// 既に含まれていないか確認
	auto it = std::find(members_.begin(), members_.end(), enemy);
	if (it == members_.end()) {
		members_.push_back(enemy);
		
		// グループIDを設定
		if (leader_) {
			enemy->SetGroupId(leader_->GetEnemyId());
		}
	}
}

void SquadAI::RemoveMember(Enemy* enemy) {
	if (!enemy) return;
	
	auto it = std::find(members_.begin(), members_.end(), enemy);
	if (it != members_.end()) {
		members_.erase(it);
	}
}

void SquadAI::Update(float deltaTime, const MagMath::Vector3& targetPos) {
	if (!leader_ || members_.empty()) return;
	
	// フォーメーションを更新
	UpdateFormation();
	
	// 戦術に応じた処理
	switch (strategy_) {
	case SquadStrategy::VFormation:
		// V字編成：リーダーが先頭、メンバーが両側後部に配置
		break;
	
	case SquadStrategy::EncircleAttack:
		// 包囲戦術：敵を包囲するように配置
		break;
	
	case SquadStrategy::SuppressiveFire:
		// 支援射撃：一部が射撃、他が前進
		break;
	
	case SquadStrategy::Pincer:
		// 挟み撃ち：左右から敵を挟むように配置
		break;
	}
}

void SquadAI::UpdateFormation() {
	if (!leader_ || members_.size() < 2) return;
	
	auto leaderTransform = leader_->GetComponent<TransformComponent>();
	if (!leaderTransform) return;
	
	Vector3 leaderPos = leaderTransform->GetPosition();
	int memberIndex = 0;
	
	switch (strategy_) {
	case SquadStrategy::VFormation: {
		// V字編成：リーダーが先頭、メンバーが両側後部に配置
		for (int i = 0; i < members_.size(); ++i) {
			if (members_[i] == leader_) continue;
			
			auto memberTransform = members_[i]->GetComponent<TransformComponent>();
			if (!memberTransform) continue;
			
			// 右側または左側に配置
			float sideOffset = (memberIndex % 2 == 0) ? 3.0f : -3.0f;
			float backOffset = (memberIndex / 2 + 1) * 4.0f;
			
			Vector3 formationPos(
				leaderPos.x + sideOffset,
				leaderPos.y,
				leaderPos.z - backOffset
			);
			
			memberTransform->MoveTo(formationPos, 15.0f);
			memberIndex++;
		}
		break;
	}
	
	case SquadStrategy::EncircleAttack: {
		// 包囲戦術：敵の周囲に配置
		int totalMembers = static_cast<int>(members_.size()) - 1; // リーダー除く
		if (totalMembers == 0) break;
		
		for (int i = 0; i < members_.size(); ++i) {
			if (members_[i] == leader_) continue;
			
			auto memberTransform = members_[i]->GetComponent<TransformComponent>();
			if (!memberTransform) continue;
			
			// 周囲に均等配置
			float angle = (2.0f * 3.14159f) * memberIndex / totalMembers;
			float radius = 5.0f;
			
			Vector3 formationPos(
				leaderPos.x + std::cos(angle) * radius,
				leaderPos.y,
				leaderPos.z + std::sin(angle) * radius
			);
			
			memberTransform->MoveTo(formationPos, 15.0f);
			memberIndex++;
		}
		break;
	}
	
	case SquadStrategy::SuppressiveFire: {
		// 支援射撃：前列と後列
		int frontCount = (static_cast<int>(members_.size()) / 2) + (static_cast<int>(members_.size()) % 2);
		for (int i = 0; i < members_.size(); ++i) {
			if (members_[i] == leader_) continue;
			
			auto memberTransform = members_[i]->GetComponent<TransformComponent>();
			if (!memberTransform) continue;
			
			bool isFront = (memberIndex < frontCount);
			float sideOffset = (memberIndex % 2 == 0) ? 2.0f : -2.0f;
			float depthOffset = isFront ? 0.0f : 3.0f;
			
			Vector3 formationPos(
				leaderPos.x + sideOffset,
				leaderPos.y,
				leaderPos.z - depthOffset
			);
			
			memberTransform->MoveTo(formationPos, 15.0f);
			memberIndex++;
		}
		break;
	}
	
	case SquadStrategy::Pincer: {
		// 挟み撃ち：左右から敵を挟む
		for (int i = 0; i < members_.size(); ++i) {
			if (members_[i] == leader_) continue;
			
			auto memberTransform = members_[i]->GetComponent<TransformComponent>();
			if (!memberTransform) continue;
			
			float sideOffset = (memberIndex % 2 == 0) ? 4.0f : -4.0f;
			float backOffset = (memberIndex / 2) * 2.0f;
			
			Vector3 formationPos(
				leaderPos.x + sideOffset,
				leaderPos.y,
				leaderPos.z - backOffset
			);
			
			memberTransform->MoveTo(formationPos, 15.0f);
			memberIndex++;
		}
		break;
	}
	}
}

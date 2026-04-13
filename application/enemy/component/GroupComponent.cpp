/*********************************************************************
 * \file   GroupComponent.cpp
 * \brief  部隊参加情報・命令受信コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GroupComponent.h"
#include "../type/Enemy.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

void GroupComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;
	groupId_ = config.GetInt("groupId", -1);
	roleInGroup_ = config.GetInt("roleInGroup", 0);
	isFollowingFormation_ = false;
	formationTargetPosition_ = Vector3(0, 0, 0);
}

void GroupComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("GroupComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragInt("Group ID", &groupId_);
		ImGui::DragInt("Role in Group", &roleInGroup_, 1, 0, 2);
		ImGui::Checkbox("Following Formation", &isFollowingFormation_);
	}
#endif
}

void GroupComponent::SetFormationFollowing(bool following) {
	isFollowingFormation_ = following;
}

void GroupComponent::SetFormationTargetPosition(const MagMath::Vector3& pos) {
	formationTargetPosition_ = pos;
}

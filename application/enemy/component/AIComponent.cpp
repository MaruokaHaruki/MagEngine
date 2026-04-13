/*********************************************************************
 * \file   AIComponent.cpp
 * \brief  AI戦略実行管理コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "AIComponent.h"
#include "../behavior/IAIBehavior.h"
#include "../type/Enemy.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

void AIComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;
	playerPosition_ = Vector3(0, 0, 0);
	// 初期戦略はここで設定される（Factory/Managerから設定）
}

void AIComponent::Update(float deltaTime) {
	if (behavior_) {
		behavior_->Update(deltaTime, *owner_, playerPosition_);
	}
}

void AIComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("AIComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (behavior_) {
			ImGui::Text("Active Behavior: %s", behavior_->GetBehaviorName().c_str());
		} else {
			ImGui::Text("Active Behavior: None");
		}
	}
#endif
}

void AIComponent::SetBehavior(std::unique_ptr<IAIBehavior> behavior) {
	behavior_ = std::move(behavior);
}

void AIComponent::SetPlayerPosition(const Vector3& playerPos) {
	playerPosition_ = playerPos;
}

/*********************************************************************
 * \file   CombatComponent.cpp
 * \brief  射撃・攻撃制御コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "CombatComponent.h"
#include "../type/Enemy.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

void CombatComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;
	shootInterval_ = config.GetFloat("shootInterval", 0.5f);
	shootCooldownTimer_ = 0.0f;
	bulletType_ = config.GetString("bulletType", "standard");
	shootDistance_ = config.GetFloat("shootDistance", 100.0f);
}

void CombatComponent::Update(float deltaTime) {
	if (shootCooldownTimer_ > 0.0f) {
		shootCooldownTimer_ -= deltaTime;
		if (shootCooldownTimer_ < 0.0f) {
			shootCooldownTimer_ = 0.0f;
		}
	}
}

void CombatComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("CombatComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Shoot Interval", &shootInterval_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Shoot Distance", &shootDistance_, 1.0f, 0.0f, 1000.0f);
		ImGui::Text("Bullet Type: %s", bulletType_.c_str());
		ImGui::ProgressBar(1.0f - (shootCooldownTimer_ / shootInterval_));
	}
#endif
}

bool CombatComponent::Fire(const MagMath::Vector3& direction) {
	if (!CanFire()) {
		return false;
	}

	// 実際の射撃ロジックはここで実装
	// EnemyBullet 生成など
	shootCooldownTimer_ = shootInterval_;
	return true;
}

bool CombatComponent::IsInShootDistance(float distance) const {
	return distance <= shootDistance_;
}

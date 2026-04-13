/*********************************************************************
 * \file   HealthComponent.cpp
 * \brief  HP管理・ダメージ処理コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "HealthComponent.h"
#include "../type/Enemy.h"
#include <algorithm>

#ifdef _DEBUG
#include "imgui.h"
#endif

void HealthComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;

	maxHP_ = config.GetInt("maxHP", 3);
	currentHP_ = maxHP_;
	damageCooldown_ = config.GetFloat("damageCooldown", 0.1f);
	damageCooldownTimer_ = 0.0f;

	onDeadCallback_ = nullptr;
}

void HealthComponent::Update(float deltaTime) {
	if (damageCooldownTimer_ > 0.0f) {
		damageCooldownTimer_ -= deltaTime;
		if (damageCooldownTimer_ < 0.0f) {
			damageCooldownTimer_ = 0.0f;
		}
	}
}

void HealthComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("HealthComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragInt("Current HP", &currentHP_);
		ImGui::DragInt("Max HP", &maxHP_);
		ImGui::DragFloat("Damage Cooldown", &damageCooldown_, 0.01f, 0.0f, 10.0f);
		ImGui::ProgressBar(static_cast<float>(currentHP_) / static_cast<float>(maxHP_));
	}
#endif
}

bool HealthComponent::TakeDamage(int damage) {
	// クールダウン中は受けつけない
	if (damageCooldownTimer_ > 0.0f) {
		return false;
	}

	int newHP = currentHP_ - damage;
	currentHP_ = (newHP > 0) ? newHP : 0;
	damageCooldownTimer_ = damageCooldown_;

	// 死亡判定
	if (currentHP_ <= 0 && onDeadCallback_) {
		onDeadCallback_();
	}

	return true;
}

void HealthComponent::RestoreHealth(int amount) {
	currentHP_ = std::min(maxHP_, currentHP_ + amount);
}

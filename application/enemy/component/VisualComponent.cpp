/*********************************************************************
 * \file   VisualComponent.cpp
 * \brief  描画・アニメーション管理コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "VisualComponent.h"
#include "../type/Enemy.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

void VisualComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;
	modelPath_ = config.GetString("modelPath", "assets/models/enemy.obj");
	scale_ = config.GetFloat("scale", 1.0f);
	currentAnimation_ = "";
	object3dSetup_ = nullptr;
	// object3d_ はここでは生成しない（setObject3dSetupで外部から設定）
}

void VisualComponent::Draw() {
	if (object3d_) {
		// object3d_->Draw(); 実装はEnemyBaseで行われる
	}
}

void VisualComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("VisualComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Model: %s", modelPath_.c_str());
		ImGui::DragFloat("Scale", &scale_, 0.01f, 0.1f, 10.0f);
		ImGui::Text("Animation: %s", currentAnimation_.c_str());
	}
#endif
}

void VisualComponent::PlayAnimation(const std::string& name) {
	currentAnimation_ = name;
	// アニメーション再生ロジックはここで実装
}

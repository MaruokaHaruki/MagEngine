/*********************************************************************
 * \file   MovementComponent.cpp
 * \brief  移動制御・速度管理コンポーネント 実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "MovementComponent.h"
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "../type/Enemy.h"

#ifdef _DEBUG
#include "imgui.h"
#endif

void MovementComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;

	baseSpeed_ = config.GetFloat("baseSpeed", 18.0f);
	acceleration_ = config.GetFloat("acceleration", 5.0f);
	turnSpeed_ = config.GetFloat("turnSpeed", 90.0f);
	velocityDamping_ = 0.95f;

	velocity_ = Vector3(0, 0, 0);
	moveTarget_ = Vector3(0, 0, 0);
	isMoving_ = false;

	orbitCenter_ = Vector3(0, 0, 0);
	orbitRadius_ = 0.0f;
	orbitDirection_ = 1.0f;
	isOrbiting_ = false;
	orbitAngle_ = 0.0f;
}

void MovementComponent::Update(float deltaTime) {
	if (isOrbiting_) {
		// 周回移動
		orbitAngle_ += (baseSpeed_ / orbitRadius_) * orbitDirection_ * deltaTime;
		if (orbitAngle_ > 360.0f) orbitAngle_ -= 360.0f;
		if (orbitAngle_ < 0.0f) orbitAngle_ += 360.0f;

		Vector3 offset;
		offset.x = orbitRadius_ * std::cos(MagMath::DegreesToRadians(orbitAngle_));
		offset.z = orbitRadius_ * std::sin(MagMath::DegreesToRadians(orbitAngle_));

		moveTarget_ = orbitCenter_ + offset;
	}

	if (isMoving_) {
		// 目標方向への速度設定
		Vector3 direction = moveTarget_ - owner_->GetPosition();
		float distance = Length(direction);

		if (distance > 0.1f) {
			direction = Normalize(direction);
			velocity_ = direction * baseSpeed_;
		} else {
			velocity_ = Vector3(0, 0, 0);
			isMoving_ = false;
		}
	} else {
		velocity_ = velocity_ * velocityDamping_;
	}

	// TransformComponent で位置更新を行う
	// (ここでは速度のみを管理)
}

void MovementComponent::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("MovementComponent", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Base Speed", &baseSpeed_, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Acceleration", &acceleration_, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat("Turn Speed", &turnSpeed_, 1.0f, 0.0f, 360.0f);
		ImGui::Checkbox("Is Moving", &isMoving_);
		ImGui::Checkbox("Is Orbiting", &isOrbiting_);
		
		if (ImGui::CollapsingHeader("Velocity")) {
			ImGui::DragFloat3("##Velocity", (float*)&velocity_, 0.1f);
		}
	}
#endif
}

void MovementComponent::MoveTo(const Vector3& target, float speedMultiplier) {
	moveTarget_ = target;
	isMoving_ = true;
	isOrbiting_ = false;
	baseSpeed_ *= speedMultiplier;
}

void MovementComponent::Orbit(const Vector3& center, float radius, float direction) {
	orbitCenter_ = center;
	orbitRadius_ = radius;
	orbitDirection_ = direction;
	isOrbiting_ = true;
	isMoving_ = false;
	orbitAngle_ = 0.0f;
}

#include "TransformComponent.h"
#include "../Enemy.h"

TransformComponent::TransformComponent()
	: position_(0, 0, 0), velocity_(0, 0, 0), scale_(1, 1, 1),
	radius_(1.0f), isMoving_(false), moveSpeed_(0), moveSmoothing_(0.15f) {
}

void TransformComponent::Initialize(const ComponentConfig& config, Enemy* owner) {
	owner_ = owner;
	radius_ = config.GetFloat("radius", 1.0f);
	scale_ = Vector3(
		config.GetFloat("scale_x", 1.0f),
		config.GetFloat("scale_y", 1.0f),
		config.GetFloat("scale_z", 1.0f)
	);
}

void TransformComponent::Update(float deltaTime) {
	if (isMoving_) {
		Vector3 direction = moveTarget_ - position_;
		float distance = Length(direction);

		if (distance < 0.1f) {
			isMoving_ = false;
			velocity_ = Vector3(0, 0, 0);
		} else {
			Vector3 normalizedDir = Normalize(direction);
			velocity_ = normalizedDir * moveSpeed_;
		}
	}

	ApplyVelocity(deltaTime);
}

void TransformComponent::MoveTo(const Vector3& target, float speed, float smoothing) {
	moveTarget_ = target;
	moveSpeed_ = speed;
	moveSmoothing_ = smoothing;
	isMoving_ = true;
}

void TransformComponent::ApplyVelocity(float deltaTime) {
	position_ = position_ + (velocity_ * deltaTime);
}

void TransformComponent::DrawImGui() {
	// ImGui 出力（省略）
}

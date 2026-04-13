#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "IEnemyComponent.h"

/**
 * @brief トランスフォームコンポーネント
 * 
 * 責務:
 * - 敵の位置、速度、スケール管理
 * - 基本的な移動処理
 */
class TransformComponent : public IEnemyComponent {
public:
	TransformComponent();
	~TransformComponent() = default;

	void Initialize(const ComponentConfig& config, Enemy* owner) override;
	void Update(float deltaTime) override;
	std::string GetComponentName() const override { return "TransformComponent"; }

	// 位置操作
	Vector3 GetPosition() const { return position_; }
	void SetPosition(const Vector3& pos) { position_ = pos; }

	// 速度操作
	Vector3 GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& vel) { velocity_ = vel; }

	// スケール操作
	Vector3 GetScale() const { return scale_; }
	void SetScale(const Vector3& scale) { scale_ = scale; }

	// 当たり判定半径
	float GetRadius() const { return radius_; }
	void SetRadius(float radius) { radius_ = radius; }

	// 移動制御
	void MoveTo(const Vector3& target, float speed, float smoothing = 0.15f);
	void ApplyVelocity(float deltaTime);

	void DrawImGui() override;

private:
	Vector3 position_;
	Vector3 velocity_;
	Vector3 scale_;
	float radius_;
	
	// 移動パラメータ
	Vector3 moveTarget_;
	bool isMoving_;
	float moveSpeed_;
	float moveSmoothing_;
};

/*********************************************************************
 * \file   OrbitBehavior.h / ShootingBehavior.h / RetreatBehavior.h / PatrolBehavior.h
 * \brief  敵AI戦略実装群
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IAIBehavior.h"
#include <vector>

// ============ Orbit Behavior ============
class OrbitBehavior : public IAIBehavior {
public:
	void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) override;
	std::string GetBehaviorName() const override { return "Orbit"; }

private:
	float orbitRadius_ = 40.0f;
	float orbitDuration_ = 20.0f;
	float elapsedTime_ = 0.0f;
};

// ============ Shooting Behavior ============
class ShootingBehavior : public IAIBehavior {
public:
	void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) override;
	std::string GetBehaviorName() const override { return "Shooting"; }

private:
	float shootInterval_ = 0.5f;
	float shootCooldown_ = 0.0f;
};

// ============ Retreat Behavior ============
class RetreatBehavior : public IAIBehavior {
public:
	void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) override;
	std::string GetBehaviorName() const override { return "Retreat"; }

private:
	float retreatSpeed_ = 25.0f;
	Vector3 retreatDirection_;
};

// ============ Patrol Behavior ============
class PatrolBehavior : public IAIBehavior {
public:
	void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) override;
	std::string GetBehaviorName() const override { return "Patrol"; }

private:
	std::vector<Vector3> patrolPoints_;
	int currentPatrolPoint_ = 0;
	float patrolSpeed_ = 10.0f;
};

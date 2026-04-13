/*********************************************************************
 * \file   AIBehaviors.cpp
 * \brief  敵AI戦略実装群
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "AIBehaviors.h"
#include "../Enemy.h"
#include "../component/MovementComponent.h"
#include "../component/TransformComponent.h"
#include "../component/CombatComponent.h"
#include <cmath>

// ============ Orbit Behavior ============
void OrbitBehavior::Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) {
	elapsedTime_ += deltaTime;

	auto transform = enemy.GetComponent<TransformComponent>();
	auto movement = enemy.GetComponent<MovementComponent>();

	if (!transform || !movement) return;

	// プレイヤーを中心に周回
	float angle = elapsedTime_ * (2.0f * 3.14159f) / orbitDuration_;
	Vector3 orbitPos(
		playerPos.x + std::cos(angle) * orbitRadius_,
		playerPos.y,
		playerPos.z + std::sin(angle) * orbitRadius_
	);

	movement->MoveTo(orbitPos, 15.0f);
}

// ============ Shooting Behavior ============
void ShootingBehavior::Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) {
	shootCooldown_ -= deltaTime;

	auto combat = enemy.GetComponent<CombatComponent>();
	if (combat && shootCooldown_ <= 0.0f) {
		// 射撃を実行
		Vector3 direction = Normalize(playerPos - enemy.GetPosition());
		combat->Fire(direction);
		shootCooldown_ = shootInterval_;
	}
}

// ============ Retreat Behavior ============
void RetreatBehavior::Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) {
	auto movement = enemy.GetComponent<MovementComponent>();
	if (!movement) return;

	// プレイヤーから逃げる方向
	retreatDirection_ = Normalize(enemy.GetPosition() - playerPos);
	Vector3 retreatTarget = enemy.GetPosition() + retreatDirection_ * 50.0f;

	movement->MoveTo(retreatTarget, retreatSpeed_);
}

// ============ Patrol Behavior ============
void PatrolBehavior::Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) {
	if (patrolPoints_.empty()) return;

	auto movement = enemy.GetComponent<MovementComponent>();
	if (!movement) return;

	// 次のパトロールポイントへ移動
	Vector3 targetPoint = patrolPoints_[currentPatrolPoint_];
	movement->MoveTo(targetPoint, patrolSpeed_);

	// ポイント到達判定
	float distance = Length(enemy.GetPosition() - targetPoint);
	if (distance < 2.0f) {
		currentPatrolPoint_ = (currentPatrolPoint_ + 1) % patrolPoints_.size();
	}
}

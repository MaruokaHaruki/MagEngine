/*********************************************************************
 * \file   ApproachBehavior.cpp
 * \brief  敵が接近する戦闘行動の実装
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "ApproachBehavior.h"
#include "../Enemy.h"
#include "../component/MovementComponent.h"
#include "../component/TransformComponent.h"

void ApproachBehavior::Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) {
	elapsedTime_ += deltaTime;

	// プレイヤーへ接近
	auto movement = enemy.GetComponent<MovementComponent>();
	if (movement) {
		movement->MoveTo(playerPos, approachSpeed_);
	}
}

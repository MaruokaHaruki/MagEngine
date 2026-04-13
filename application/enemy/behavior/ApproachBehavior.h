/*********************************************************************
 * \file   ApproachBehavior.h
 * \brief  敵が接近する戦闘行動
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IAIBehavior.h"

class ApproachBehavior : public IAIBehavior {
public:
	void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) override;
	std::string GetBehaviorName() const override { return "Approach"; }

private:
	float approachSpeed_ = 20.0f;
};

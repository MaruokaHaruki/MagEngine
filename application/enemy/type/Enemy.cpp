#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
/// Enemy行動状態
class Enemy::BehaviorStateBase {
public:
	virtual ~BehaviorStateBase() = default;
	virtual void Update(Enemy &enemy, float deltaTime) = 0;
};

class Enemy::ApproachBehaviorState final : public Enemy::BehaviorStateBase {
public:
	void Update(Enemy &enemy, float deltaTime) override {
		if (!enemy.player_) {
			enemy.transform_.translate.z += enemy.speed_ * enemy.GetSpawnSpeedMultiplier() * deltaTime;
			return;
		}

		const Vector3 playerPos = enemy.player_->GetPosition();
		enemy.targetPosition_ = {playerPos.x, playerPos.y, playerPos.z - EnemyConstants::kCombatDepth};
		if (enemy.GetDistanceTo(enemy.targetPosition_) < 20.0f) {
			enemy.combatTimer_ = 0.0f;
			enemy.combatCenter_ = playerPos;
			enemy.moveTimer_ = 0.0f;
			enemy.RequestBehaviorState(BehaviorState::Combat);
			return;
		}
		enemy.MoveToward(enemy.targetPosition_, EnemyConstants::kApproachSpeed * enemy.GetSpawnSpeedMultiplier(), EnemyConstants::kMovementSmoothing, deltaTime);
	}
};

class Enemy::CombatBehaviorState final : public Enemy::BehaviorStateBase {
public:
	void Update(Enemy &enemy, float deltaTime) override {
		enemy.combatTimer_ += deltaTime;
		enemy.moveTimer_ += deltaTime;
		if (enemy.combatTimer_ >= enemy.combatDuration_) {
			enemy.RequestBehaviorState(BehaviorState::Retreat);
			return;
		}
		if (enemy.player_) {
			const Vector3 playerPos = enemy.player_->GetPosition();
			enemy.combatCenter_.x += (playerPos.x - enemy.combatCenter_.x) * EnemyConstants::kPlayerTrackingSpeed;
			enemy.combatCenter_.y += (playerPos.y - enemy.combatCenter_.y) * EnemyConstants::kPlayerTrackingSpeed;
			enemy.combatCenter_.z += (playerPos.z - enemy.combatCenter_.z) * EnemyConstants::kPlayerTrackingSpeed;
		}
		if (enemy.moveTimer_ >= EnemyConstants::kMoveInterval) {
			const float angle = enemy.combatTimer_ * 1.2f;
			enemy.targetPosition_ = {enemy.combatCenter_.x + std::sin(angle) * EnemyConstants::kCombatRadius, enemy.combatCenter_.y + std::cos(angle * 0.7f) * 5.0f, enemy.combatCenter_.z - EnemyConstants::kCombatDepth};
			enemy.moveTimer_ = 0.0f;
		}
		enemy.MoveToward(enemy.targetPosition_, EnemyConstants::kCombatSpeed * enemy.GetSpawnSpeedMultiplier(), EnemyConstants::kMovementSmoothing, deltaTime);
	}
};

class Enemy::RetreatBehaviorState final : public Enemy::BehaviorStateBase {
public:
	void Update(Enemy &enemy, float deltaTime) override {
		const Vector3 targetVelocity = {0.0f, 8.0f, EnemyConstants::kRetreatSpeed};
		enemy.currentVelocity_.x += (targetVelocity.x - enemy.currentVelocity_.x) * EnemyConstants::kMovementSmoothing;
		enemy.currentVelocity_.y += (targetVelocity.y - enemy.currentVelocity_.y) * EnemyConstants::kMovementSmoothing;
		enemy.currentVelocity_.z += (targetVelocity.z - enemy.currentVelocity_.z) * EnemyConstants::kMovementSmoothing;
		enemy.transform_.translate.x += enemy.currentVelocity_.x * deltaTime;
		enemy.transform_.translate.y += enemy.currentVelocity_.y * deltaTime;
		enemy.transform_.translate.z += enemy.currentVelocity_.z * deltaTime;
	}
};

Enemy::Enemy() = default;
Enemy::~Enemy() = default;

///=============================================================================
///                        初期化
void Enemy::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	maxHP_ = EnemyConstants::kDefaultHP;
	currentHP_ = maxHP_;
	speed_ = EnemyConstants::kDefaultSpeed;

	// グループ関連初期化
	groupId_ = -1; // 初期状態は単独
	ClearFormationTarget();

	// 行動ステート初期化
	behaviorState_ = BehaviorState::Approach;
	behaviorStateObject_ = std::make_unique<ApproachBehaviorState>();
	pendingBehaviorStateObject_.reset();
	combatTimer_ = 0.0f;
	combatDuration_ = EnemyConstants::kCombatDuration;
	combatCenter_ = {0.0f, 0.0f, 0.0f};

	// 移動関連初期化
	moveTimer_ = 0.0f;
	targetPosition_ = position;
}

///=============================================================================
///                        更新
void Enemy::Update() {
	Update(1.0f / 60.0f);
}

void Enemy::Update(float deltaTime) {
	EnemyBase::Update(deltaTime);

	if (destroyState_ != DestroyState::Alive || isHitReacting_) {
		return;
	}

	if (IsFormationFollowEnabled()) {
		UpdateFormationFollow(deltaTime);
		return;
	}

	UpdateBehaviorState(deltaTime);
}

void Enemy::UpdateBehaviorState(float safeDeltaTime) {
	if (behaviorStateObject_) {
		behaviorStateObject_->Update(*this, safeDeltaTime);
	}
	ApplyPendingBehaviorState();
}

void Enemy::RequestBehaviorState(BehaviorState nextState) {
	if (nextState == behaviorState_) {
		return;
	}
	if (nextState == BehaviorState::Combat) {
		pendingBehaviorStateObject_ = std::make_unique<CombatBehaviorState>();
	} else if (nextState == BehaviorState::Retreat) {
		pendingBehaviorStateObject_ = std::make_unique<RetreatBehaviorState>();
	}
	behaviorState_ = nextState;
}

void Enemy::ApplyPendingBehaviorState() {
	if (pendingBehaviorStateObject_) {
		behaviorStateObject_ = std::move(pendingBehaviorStateObject_);
	}
}

///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	EnemyBase::DrawImGui();
#ifdef _DEBUG
	const char *stateNames[] = {"Approach", "Combat", "Retreat", "FormationFollow"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(behaviorState_)]);
	ImGui::Text("Combat Timer: %.1f / %.1f", combatTimer_, combatDuration_);
	ImGui::Text("Group ID: %d", groupId_);
	ImGui::Text("Following Formation: %s", IsFormationFollowEnabled() ? "Yes" : "No");
#endif
}

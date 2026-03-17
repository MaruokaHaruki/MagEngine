#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void Enemy::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	maxHP_ = EnemyConstants::kDefaultHP;
	currentHP_ = maxHP_;
	speed_ = EnemyConstants::kDefaultSpeed;

	// 行動ステート初期化
	behaviorState_ = BehaviorState::Approach;
	combatTimer_ = 0.0f;
	combatDuration_ = EnemyConstants::kCombatDuration;
	combatCenter_ = {0.0f, 0.0f, 0.0f};

	// 移動関連初期化
	moveTimer_ = 0.0f;
	targetPosition_ = position;

	// Dash 関連初期化
	dashTimer_ = 0.0f;
	dashCooldownTimer_ = 0.0f;
	dashTargetPos_ = {0.0f, 0.0f, 0.0f};
}

///=============================================================================
///                        更新
void Enemy::Update() {
	EnemyBase::Update();

	if (destroyState_ != DestroyState::Alive || isHitReacting_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	switch (behaviorState_) {
	case BehaviorState::Approach: {
		if (player_) {
			Vector3 playerPos = player_->GetPosition();
			targetPosition_ = {
				playerPos.x,
				playerPos.y,
				playerPos.z - EnemyConstants::kCombatDepth};

			if (GetDistanceTo(targetPosition_) < 20.0f) {
				behaviorState_ = BehaviorState::Combat;
				combatTimer_ = 0.0f;
				dashCooldownTimer_ = 0.0f;
				combatCenter_ = playerPos;
				moveTimer_ = 0.0f;
			} else {
				MoveToward(targetPosition_, EnemyConstants::kApproachSpeed, EnemyConstants::kMovementSmoothing);
			}
		} else {
			transform_.translate.z += speed_ * deltaTime;
		}
		break;
	}

	case BehaviorState::Combat: {
		combatTimer_ += deltaTime;
		moveTimer_ += deltaTime;
		dashCooldownTimer_ += deltaTime;

		if (combatTimer_ >= combatDuration_) {
			behaviorState_ = BehaviorState::Retreat;
			break;
		}

		// Dash クールダウン完了 → 突進攻撃へ
		if (dashCooldownTimer_ >= EnemyConstants::kDashCooldown && player_) {
			behaviorState_ = BehaviorState::Dash;
			dashTimer_ = 0.0f;
			dashTargetPos_ = player_->GetPosition();
			break;
		}

		// プレイヤー位置を滑らかに追跡
		if (player_) {
			Vector3 currentPlayerPos = player_->GetPosition();
			combatCenter_.x += (currentPlayerPos.x - combatCenter_.x) * EnemyConstants::kPlayerTrackingSpeed;
			combatCenter_.y += (currentPlayerPos.y - combatCenter_.y) * EnemyConstants::kPlayerTrackingSpeed;
			combatCenter_.z += (currentPlayerPos.z - combatCenter_.z) * EnemyConstants::kPlayerTrackingSpeed;
		}

		// 一定間隔で周回目標位置を更新
		if (moveTimer_ >= EnemyConstants::kMoveInterval) {
			float angle = combatTimer_ * 1.2f;
			targetPosition_ = {
				combatCenter_.x + std::sin(angle) * EnemyConstants::kCombatRadius,
				combatCenter_.y + std::cos(angle * 0.7f) * 5.0f,
				combatCenter_.z - EnemyConstants::kCombatDepth};
			moveTimer_ = 0.0f;
		}

		MoveToward(targetPosition_, EnemyConstants::kCombatSpeed, EnemyConstants::kMovementSmoothing);
		break;
	}

	case BehaviorState::Dash: {
		dashTimer_ += deltaTime;

		// 直線突進（currentVelocity_ は不要なので直接移動）
		Vector3 dir = {
			dashTargetPos_.x - transform_.translate.x,
			dashTargetPos_.y - transform_.translate.y,
			dashTargetPos_.z - transform_.translate.z};
		float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

		if (dist > EnemyConstants::kDashHitRadius && dashTimer_ < EnemyConstants::kDashDuration) {
			dir.x /= dist;
			dir.y /= dist;
			dir.z /= dist;
			transform_.translate.x += dir.x * EnemyConstants::kDashSpeed * deltaTime;
			transform_.translate.y += dir.y * EnemyConstants::kDashSpeed * deltaTime;
			transform_.translate.z += dir.z * EnemyConstants::kDashSpeed * deltaTime;
		} else {
			// Dash 終了 → Combat に戻る
			behaviorState_ = BehaviorState::Combat;
			dashCooldownTimer_ = 0.0f;
			currentVelocity_ = {0.0f, 0.0f, 0.0f}; // 速度リセット
		}
		break;
	}

	case BehaviorState::Retreat: {
		Vector3 targetVel = {0.0f, 8.0f, EnemyConstants::kRetreatSpeed};
		currentVelocity_.x += (targetVel.x - currentVelocity_.x) * EnemyConstants::kMovementSmoothing;
		currentVelocity_.y += (targetVel.y - currentVelocity_.y) * EnemyConstants::kMovementSmoothing;
		currentVelocity_.z += (targetVel.z - currentVelocity_.z) * EnemyConstants::kMovementSmoothing;
		transform_.translate.x += currentVelocity_.x * deltaTime;
		transform_.translate.y += currentVelocity_.y * deltaTime;
		transform_.translate.z += currentVelocity_.z * deltaTime;
		break;
	}
	}
}

///=============================================================================
///                        衝突処理（Dash 中の体当たり攻撃）
void Enemy::OnCollisionEnter(BaseObject *other) {
	if (behaviorState_ == BehaviorState::Dash) {
		if (Player *player = dynamic_cast<Player *>(other)) {
			player->TakeDamage(1);
			// Dash 終了 → Combat に戻る
			behaviorState_ = BehaviorState::Combat;
			dashCooldownTimer_ = 0.0f;
			currentVelocity_ = {0.0f, 0.0f, 0.0f};
			return;
		}
	}
	// 通常の被弾処理は基底クラスに委譲
	EnemyBase::OnCollisionEnter(other);
}

///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	EnemyBase::DrawImGui();
#ifdef _DEBUG
	const char *stateNames[] = {"Approach", "Combat", "Dash", "Retreat"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(behaviorState_)]);
	ImGui::Text("Combat Timer: %.1f / %.1f", combatTimer_, combatDuration_);
	ImGui::Text("Dash Cooldown: %.1f / %.1f", dashCooldownTimer_, EnemyConstants::kDashCooldown);
#endif
}

#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void Enemy::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	// 基底クラスの初期化
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	// HPと速度をEnemyのデフォルト値で初期化
	maxHP_ = EnemyConstants::kDefaultHP;
	currentHP_ = maxHP_;
	speed_ = EnemyConstants::kDefaultSpeed;

	// Enemy固有の行動ステート初期化
	behaviorState_ = BehaviorState::Approach;
	combatTimer_ = 0.0f;
	combatDuration_ = EnemyConstants::kCombatDuration;
	combatCenter_ = {0.0f, 0.0f, 0.0f};

	// 移動関連初期化
	moveTimer_ = 0.0f;
	currentVelocity_ = {0.0f, 0.0f, 0.0f};
	targetPosition_ = position;
}

///=============================================================================
///                        更新
void Enemy::Update() {
	// 基底クラスの共通更新
	EnemyBase::Update();

	// 破壊中または死亡状態なら行動処理をスキップ
	if (destroyState_ != DestroyState::Alive || isHitReacting_) {
		return;
	}

	// === ステートマシンによる行動制御 ===
	const float deltaTime = 1.0f / 60.0f;

	switch (behaviorState_) {
	case BehaviorState::Approach: {
		// 接近フェーズ
		if (player_) {
			Vector3 playerPos = player_->GetPosition();

			targetPosition_ = {
				playerPos.x,
				playerPos.y,
				playerPos.z - EnemyConstants::kCombatDepth};

			Vector3 direction = {
				targetPosition_.x - transform_.translate.x,
				targetPosition_.y - transform_.translate.y,
				targetPosition_.z - transform_.translate.z};

			float distance = std::sqrt(
				direction.x * direction.x +
				direction.y * direction.y +
				direction.z * direction.z);

			if (distance < 20.0f) {
				behaviorState_ = BehaviorState::Combat;
				combatTimer_ = 0.0f;
				combatCenter_ = playerPos;
				moveTimer_ = 0.0f;
			} else {
				direction.x /= distance;
				direction.y /= distance;
				direction.z /= distance;

				Vector3 targetVelocity = {
					direction.x * EnemyConstants::kApproachSpeed,
					direction.y * EnemyConstants::kApproachSpeed,
					direction.z * EnemyConstants::kApproachSpeed};

				// イージングで加減速
				currentVelocity_.x += (targetVelocity.x - currentVelocity_.x) * EnemyConstants::kMovementSmoothing;
				currentVelocity_.y += (targetVelocity.y - currentVelocity_.y) * EnemyConstants::kMovementSmoothing;
				currentVelocity_.z += (targetVelocity.z - currentVelocity_.z) * EnemyConstants::kMovementSmoothing;

				transform_.translate.x += currentVelocity_.x * deltaTime;
				transform_.translate.y += currentVelocity_.y * deltaTime;
				transform_.translate.z += currentVelocity_.z * deltaTime;
			}
		} else {
			transform_.translate.z += speed_ * deltaTime;
		}
		break;
	}

	case BehaviorState::Combat: {
		// 戦闘フェーズ
		combatTimer_ += deltaTime;
		moveTimer_ += deltaTime;

		if (combatTimer_ >= combatDuration_) {
			behaviorState_ = BehaviorState::Retreat;
			break;
		}

		// プレイヤー位置を滑らかに追跡
		if (player_) {
			Vector3 currentPlayerPos = player_->GetPosition();
			combatCenter_.x += (currentPlayerPos.x - combatCenter_.x) * EnemyConstants::kPlayerTrackingSpeed;
			combatCenter_.y += (currentPlayerPos.y - combatCenter_.y) * EnemyConstants::kPlayerTrackingSpeed;
			combatCenter_.z += (currentPlayerPos.z - combatCenter_.z) * EnemyConstants::kPlayerTrackingSpeed;
		}

		// 一定間隔で新しい目標位置を設定
		if (moveTimer_ >= EnemyConstants::kMoveInterval) {
			float angle = combatTimer_ * 1.2f;
			float offsetX = std::sin(angle) * EnemyConstants::kCombatRadius;
			float offsetY = std::cos(angle * 0.7f) * 5.0f;

			targetPosition_ = {
				combatCenter_.x + offsetX,
				combatCenter_.y + offsetY,
				combatCenter_.z - EnemyConstants::kCombatDepth};

			moveTimer_ = 0.0f;
		}

		// 目標位置への滑らかな移動
		Vector3 direction = {
			targetPosition_.x - transform_.translate.x,
			targetPosition_.y - transform_.translate.y,
			targetPosition_.z - transform_.translate.z};

		float distance = std::sqrt(
			direction.x * direction.x +
			direction.y * direction.y +
			direction.z * direction.z);

		// 接近完了判定
		constexpr float kApproachCompleteDistance = 20.0f;
		if (distance < kApproachCompleteDistance) {
			direction.x /= distance;
			direction.y /= distance;
			direction.z /= distance;

			Vector3 targetVelocity = {
				direction.x * EnemyConstants::kCombatSpeed,
				direction.y * EnemyConstants::kCombatSpeed,
				direction.z * EnemyConstants::kCombatSpeed};

			// きれいなイージングで加減速
			currentVelocity_.x += (targetVelocity.x - currentVelocity_.x) * EnemyConstants::kMovementSmoothing;
			currentVelocity_.y += (targetVelocity.y - currentVelocity_.y) * EnemyConstants::kMovementSmoothing;
			currentVelocity_.z += (targetVelocity.z - currentVelocity_.z) * EnemyConstants::kMovementSmoothing;

			transform_.translate.x += currentVelocity_.x * deltaTime;
			transform_.translate.y += currentVelocity_.y * deltaTime;
			transform_.translate.z += currentVelocity_.z * deltaTime;
		}

		break;
	}

	case BehaviorState::Retreat: {
		// 退却フェーズ
		Vector3 targetVelocity = {0.0f, 8.0f, EnemyConstants::kRetreatSpeed};

		currentVelocity_.x += (targetVelocity.x - currentVelocity_.x) * EnemyConstants::kMovementSmoothing;
		currentVelocity_.y += (targetVelocity.y - currentVelocity_.y) * EnemyConstants::kMovementSmoothing;
		currentVelocity_.z += (targetVelocity.z - currentVelocity_.z) * EnemyConstants::kMovementSmoothing;

		transform_.translate.x += currentVelocity_.x * deltaTime;
		transform_.translate.y += currentVelocity_.y * deltaTime;
		transform_.translate.z += currentVelocity_.z * deltaTime;
		break;
	}
	}
}

///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	EnemyBase::DrawImGui();
	// Enemy固有のデバッグ情報を追加可能
}
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void Enemy::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	// 基底クラスの初期化
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	// Enemy固有の行動ステート初期化
	behaviorState_ = BehaviorState::Approach;
	combatTimer_ = 0.0f;
	combatDuration_ = EnemyConstants::kCombatDuration;
	combatCenter_ = {0.0f, 0.0f, 0.0f};
	circleAngle_ = 0.0f;
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
			combatCenter_ = playerPos;

			Vector3 targetPos = {
				playerPos.x,
				playerPos.y,
				playerPos.z + EnemyConstants::kCombatRadius};

			Vector3 direction = {
				targetPos.x - transform_.translate.x,
				targetPos.y - transform_.translate.y,
				targetPos.z - transform_.translate.z};

			float distance = std::sqrt(
				direction.x * direction.x +
				direction.y * direction.y +
				direction.z * direction.z);

			if (distance < 5.0f) {
				behaviorState_ = BehaviorState::Combat;
				combatTimer_ = 0.0f;
				circleAngle_ = std::atan2(
					transform_.translate.x - playerPos.x,
					transform_.translate.z - playerPos.z);
			} else {
				direction.x /= distance;
				direction.y /= distance;
				direction.z /= distance;

				transform_.translate.x += direction.x * EnemyConstants::kApproachSpeed * deltaTime;
				transform_.translate.y += direction.y * EnemyConstants::kApproachSpeed * deltaTime;
				transform_.translate.z += direction.z * EnemyConstants::kApproachSpeed * deltaTime;
			}
		} else {
			transform_.translate.z += speed_ * deltaTime;
		}
		break;
	}

	case BehaviorState::Combat: {
		// 戦闘フェーズ
		combatTimer_ += deltaTime;

		if (player_) {
			combatCenter_ = player_->GetPosition();
		}

		if (combatTimer_ >= combatDuration_) {
			behaviorState_ = BehaviorState::Retreat;
			break;
		}

		circleAngle_ += EnemyConstants::kCircleFrequency * deltaTime;
		float radius = EnemyConstants::kCombatRadius;
		float verticalOffset = std::sin(circleAngle_ * 2.0f) * 4.0f;

		Vector3 targetPos = {
			combatCenter_.x + std::sin(circleAngle_) * radius,
			combatCenter_.y + verticalOffset,
			combatCenter_.z + std::cos(circleAngle_) * radius};

		Vector3 direction = {
			targetPos.x - transform_.translate.x,
			targetPos.y - transform_.translate.y,
			targetPos.z - transform_.translate.z};

		float distance = std::sqrt(
			direction.x * direction.x +
			direction.y * direction.y +
			direction.z * direction.z);

		if (distance > 0.1f) {
			direction.x /= distance;
			direction.y /= distance;
			direction.z /= distance;

			transform_.translate.x += direction.x * EnemyConstants::kCombatSpeed * deltaTime;
			transform_.translate.y += direction.y * EnemyConstants::kCombatSpeed * deltaTime;
			transform_.translate.z += direction.z * EnemyConstants::kCombatSpeed * deltaTime;
		}
		break;
	}

	case BehaviorState::Retreat: {
		// 退却フェーズ
		transform_.translate.z += EnemyConstants::kRetreatSpeed * deltaTime;
		transform_.translate.y += 8.0f * deltaTime;
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

///=============================================================================
///                        敵タイプ設定
void Enemy::SetEnemyType(EnemyType type) {
	switch (type) {
	case EnemyType::Normal:
		maxHP_ = EnemyConstants::kNormalEnemyHP;
		currentHP_ = maxHP_;
		speed_ = EnemyConstants::kNormalEnemySpeed;
		break;
	case EnemyType::Fast:
		maxHP_ = EnemyConstants::kFastEnemyHP;
		currentHP_ = maxHP_;
		speed_ = EnemyConstants::kFastEnemySpeed;
		break;
	}
}
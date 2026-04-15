#define _USE_MATH_DEFINES
#include "EnemyGunner.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void EnemyGunner::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	maxHP_ = EnemyGunnerConstants::kDefaultHP;
	currentHP_ = maxHP_;
	speed_ = EnemyGunnerConstants::kDefaultSpeed;

	// グループ関連初期化
	groupId_ = -1; // 初期状態は単独
	isFollowingFormation_ = false;
	formationTargetPosition_ = position;

	state_ = GunnerState::Approach;
	shootTimer_ = 0.0f;
	combatTimer_ = 0.0f;
	targetPosition_ = position;
	object3dSetup_ = object3dSetup;
	moveTimer_ = 0.0f;
	combatCenter_ = {0.0f, 0.0f, 0.0f};
}

///=============================================================================
///                        更新
void EnemyGunner::Update() {
	EnemyBase::Update();

	if (destroyState_ != DestroyState::Alive || isHitReacting_) {
		for (auto &bullet : bullets_) {
			if (bullet)
				bullet->Update();
		}
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(),
						   [](const std::unique_ptr<EnemyBullet> &b) { return !b || !b->IsAlive(); }),
			bullets_.end());
		return;
	}

	const float deltaTime = 1.0f / 60.0f;
	shootTimer_ += deltaTime;

	// フォーメーション中は編隊の目標位置に移動
	if (isFollowingFormation_) {
		Vector3 toTarget = formationTargetPosition_ - transform_.translate;
		float distanceToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
		
		if (distanceToTarget > 1.0f) {
			MoveToward(formationTargetPosition_, EnemyGunnerConstants::kDefaultSpeed, 0.8f);
		} else {
			transform_.translate = formationTargetPosition_;
		}
		
		// フォーメーション中は射撃しない（または低頻度で射撃）
		// 弾の更新・削除のみ行う
		for (auto &bullet : bullets_) {
			if (bullet)
				bullet->Update();
		}
		bullets_.erase(
			std::remove_if(bullets_.begin(), bullets_.end(),
						   [](const std::unique_ptr<EnemyBullet> &b) { return !b || !b->IsAlive(); }),
			bullets_.end());
		return;
	}

	switch (state_) {
	case GunnerState::Approach: {
		if (player_) {
			Vector3 playerPos = player_->GetPosition();
			targetPosition_ = {
				playerPos.x,
				playerPos.y,
				playerPos.z + EnemyGunnerConstants::kCombatDepth};

			if (GetDistanceTo(targetPosition_) < EnemyGunnerConstants::kShootingDistance) {
				state_ = GunnerState::Shooting;
				combatTimer_ = 0.0f;
				shootTimer_ = 0.0f;
				combatCenter_ = playerPos;
				moveTimer_ = 0.0f;
			} else {
				MoveToward(targetPosition_, EnemyGunnerConstants::kApproachSpeed, 1.0f);
			}
		}
		break;
	}

	case GunnerState::Shooting: {
		combatTimer_ += deltaTime;
		moveTimer_ += deltaTime;

		if (combatTimer_ >= EnemyGunnerConstants::kCombatDuration) {
			state_ = GunnerState::Retreat;
			break;
		}

		// プレイヤー位置を追跡
		if (player_) {
			Vector3 currentPlayerPos = player_->GetPosition();
			combatCenter_.x += (currentPlayerPos.x - combatCenter_.x) * 0.05f;
			combatCenter_.y += (currentPlayerPos.y - combatCenter_.y) * 0.05f;
			combatCenter_.z += (currentPlayerPos.z - combatCenter_.z) * 0.05f;
		}

		// 一定間隔で周回目標位置を更新
		if (moveTimer_ >= 2.5f) {
			float angle = combatTimer_ * 1.2f;
			targetPosition_ = {
				combatCenter_.x + std::sin(angle) * EnemyGunnerConstants::kCombatRadius,
				combatCenter_.y + std::cos(angle * 0.7f) * 5.0f,
				combatCenter_.z + EnemyGunnerConstants::kCombatDepth};
			moveTimer_ = 0.0f;
		}

		// 目標位置への移動（停止閾値以内なら固定）
		float distToTarget = GetDistanceTo(targetPosition_);
		if (distToTarget > 2.0f) {
			MoveToward(targetPosition_, EnemyGunnerConstants::kDefaultSpeed, 0.8f);
		} else {
			transform_.translate = targetPosition_;
		}

		// 射撃処理
		if (shootTimer_ >= EnemyGunnerConstants::kShootInterval && player_) {
			Vector3 playerPos = player_->GetPosition();
			Vector3 shootDir = {
				playerPos.x - transform_.translate.x,
				playerPos.y - transform_.translate.y,
				playerPos.z - transform_.translate.z};

			float dist = std::sqrt(shootDir.x * shootDir.x + shootDir.y * shootDir.y + shootDir.z * shootDir.z);
			shootDir.x /= dist;
			shootDir.y /= dist;
			shootDir.z /= dist;

			auto bullet = std::make_unique<EnemyBullet>();
			bullet->Initialize(object3dSetup_, "Missile.obj", transform_.translate, shootDir);
			bullet->SetParticleSystem(particle_, particleSetup_);
			bullets_.push_back(std::move(bullet));

			shootTimer_ = 0.0f;
		}
		break;
	}

	case GunnerState::Retreat: {
		transform_.translate.y += 8.0f * deltaTime;
		transform_.translate.z += EnemyGunnerConstants::kRetreatSpeed * deltaTime;
		break;
	}
	}

	// 弾の更新・削除
	for (auto &bullet : bullets_) {
		if (bullet)
			bullet->Update();
	}
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
					   [](const std::unique_ptr<EnemyBullet> &b) { return !b || !b->IsAlive(); }),
		bullets_.end());
}

///=============================================================================
///                        描画
void EnemyGunner::Draw() {
	EnemyBase::Draw();
	for (auto &bullet : bullets_) {
		if (bullet)
			bullet->Draw();
	}
}

///=============================================================================
///                        ImGui描画
void EnemyGunner::DrawImGui() {
	EnemyBase::DrawImGui();
#ifdef _DEBUG
	ImGui::Text("Gunner - Bullets: %zu", bullets_.size());
	ImGui::Text("Shoot Timer: %.2f", shootTimer_);
	ImGui::Text("Combat Timer: %.2f / %.2f", combatTimer_, EnemyGunnerConstants::kCombatDuration);
#endif
}

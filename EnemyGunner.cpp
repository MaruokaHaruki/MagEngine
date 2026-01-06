#define _USE_MATH_DEFINES
#include "EnemyGunner.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void EnemyGunner::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	EnemyBase::Initialize(object3dSetup, modelPath, position);

	maxHP_ = EnemyGunnerConstants::kDefaultHP;
	currentHP_ = maxHP_;
	speed_ = EnemyGunnerConstants::kDefaultSpeed;

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
		// 弾の更新のみ
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

	switch (state_) {
	case GunnerState::Approach: {
		if (player_) {
			Vector3 playerPos = player_->GetPosition();

			// Enemyと同じ目標位置計算（プレイヤーの前方）
			targetPosition_ = {
				playerPos.x,
				playerPos.y,
				playerPos.z + EnemyGunnerConstants::kCombatDepth}; // プラス記号に修正

			Vector3 direction = {
				targetPosition_.x - transform_.translate.x,
				targetPosition_.y - transform_.translate.y,
				targetPosition_.z - transform_.translate.z};

			float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

			if (distance < EnemyGunnerConstants::kShootingDistance) {
				state_ = GunnerState::Shooting;
				combatTimer_ = 0.0f;
				shootTimer_ = 0.0f;
				combatCenter_ = playerPos;
				moveTimer_ = 0.0f;
			} else {
				direction.x /= distance;
				direction.y /= distance;
				direction.z /= distance;

				transform_.translate.x += direction.x * EnemyGunnerConstants::kApproachSpeed * deltaTime;
				transform_.translate.y += direction.y * EnemyGunnerConstants::kApproachSpeed * deltaTime;
				transform_.translate.z += direction.z * EnemyGunnerConstants::kApproachSpeed * deltaTime;
			}
		}
		break;
	}

	case GunnerState::Shooting: {
		combatTimer_ += deltaTime;
		moveTimer_ += deltaTime;

		// 戦闘時間終了で退却
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

		// 一定間隔で新しい目標位置を設定
		if (moveTimer_ >= 2.5f) {
			float angle = combatTimer_ * 1.2f;
			float offsetX = std::sin(angle) * EnemyGunnerConstants::kCombatRadius;
			float offsetY = std::cos(angle * 0.7f) * 5.0f;

			// Enemyと同じ位置計算（プレイヤーの前方を周回）
			targetPosition_ = {
				combatCenter_.x + offsetX,
				combatCenter_.y + offsetY,
				combatCenter_.z + EnemyGunnerConstants::kCombatDepth}; // プラス記号に修正

			moveTimer_ = 0.0f;
		}

		// 目標位置への移動
		Vector3 direction = {
			targetPosition_.x - transform_.translate.x,
			targetPosition_.y - transform_.translate.y,
			targetPosition_.z - transform_.translate.z};

		float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

		if (distance > 0.1f) {
			direction.x /= distance;
			direction.y /= distance;
			direction.z /= distance;

			transform_.translate.x += direction.x * 18.0f * deltaTime;
			transform_.translate.y += direction.y * 18.0f * deltaTime;
			transform_.translate.z += direction.z * 18.0f * deltaTime;
		}

		// 射撃処理
		if (shootTimer_ >= EnemyGunnerConstants::kShootInterval && player_) {
			Vector3 playerPos = player_->GetPosition();
			Vector3 shootDirection = {
				playerPos.x - transform_.translate.x,
				playerPos.y - transform_.translate.y,
				playerPos.z - transform_.translate.z};

			float shootDistance = std::sqrt(shootDirection.x * shootDirection.x + shootDirection.y * shootDirection.y + shootDirection.z * shootDirection.z);
			shootDirection.x /= shootDistance;
			shootDirection.y /= shootDistance;
			shootDirection.z /= shootDistance;

			auto bullet = std::make_unique<EnemyBullet>();
			bullet->Initialize(object3dSetup_, "Bullet.obj", transform_.translate, shootDirection);
			bullet->SetParticleSystem(particle_, particleSetup_);
			bullets_.push_back(std::move(bullet));

			shootTimer_ = 0.0f;
		}

		break;
	}

	case GunnerState::Retreat: {
		transform_.translate.y += 8.0f * deltaTime;
		transform_.translate.z += EnemyGunnerConstants::kRetreatSpeed * deltaTime; // マイナスをプラスに修正
		break;
	}
	}

	// 弾の更新
	for (auto &bullet : bullets_) {
		if (bullet)
			bullet->Update();
	}

	// 死んだ弾を削除
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

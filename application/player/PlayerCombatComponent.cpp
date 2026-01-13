/*********************************************************************
 * \file   PlayerCombatComponent.cpp
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note   プレイヤー射撃コンポーネント - 弾、ミサイルの発射管理
 *********************************************************************/
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "PlayerCombatComponent.h"
#include "CameraManager.h"
#include "EnemyBase.h"
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "ModelManager.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

namespace {
	constexpr float kFrameDelta = 1.0f / 60.0f;
}

//=======================================================================
// 初期化
void PlayerCombatComponent::Initialize(MagEngine::Object3dSetup *object3dSetup) {
	object3dSetup_ = object3dSetup;

	// パーティクルエミッターの初期化
	bulletEmitter_.Initialize();
	missileEmitter_.Initialize();

	// 弾のモデル設定
	const std::string bulletModel = "bullet.obj";
	ModelManager::GetInstance()->LoadModel(bulletModel);
	bulletEmitter_.SetModel(bulletModel);

	// ミサイルのモデル設定
	const std::string missileModel = "missile.obj";
	ModelManager::GetInstance()->LoadModel(missileModel);
	missileEmitter_.SetModel(missileModel);

	// 初期化フラグ
	isInitialized_ = true;

	enemyManager_ = nullptr;
	shootCoolTime_ = 0.0f;
	maxShootCoolTime_ = 0.1f;
	missileCoolTime_ = 0.0f;
	maxMissileCoolTime_ = 1.0f;
	bulletFireDirection_ = {0.0f, 0.0f, 1.0f};
}

//=============================================================================
// 更新
void PlayerCombatComponent::Update(float deltaTime) {
	if (!isInitialized_) {
		return;
	}

	shootCoolTime_ -= deltaTime;
	missileCoolTime_ -= deltaTime;

	// 各弾の更新
	for (auto &bullet : bullets_) {
		if (bullet) {
			bullet->Update();
		}
	}

	for (auto &missile : missiles_) {
		if (missile) {
			missile->Update();
		}
	}

	// 古い弾の削除
	bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
								  [](const std::unique_ptr<PlayerBullet> &bullet) { return !bullet->IsAlive(); }),
				   bullets_.end());

	missiles_.erase(std::remove_if(missiles_.begin(), missiles_.end(),
								   [](const std::unique_ptr<PlayerMissile> &missile) { return !missile->IsAlive(); }),
					missiles_.end());
}

//=============================================================================
// 弾の発射処理
void PlayerCombatComponent::ProcessShooting(const Vector3 &position, const Vector3 &forward, EnemyBase *lockOnTarget) {
	if (!isInitialized_) {
		return;
	}

	// マシンガン発射
	if (CanShootBullet()) {
		ShootBullet(position, forward);
	}

	// ミサイル発射
	if (CanShootMissile()) {
		ShootMissile(position, forward, lockOnTarget);
	}
}

//=============================================================================
// マシンガン弾の発射
void PlayerCombatComponent::ShootBullet(const Vector3 &position, const Vector3 &direction) {
	if (!CanShootBullet()) {
		return;
	}

	// 発射方向を記録（HUD用）
	bulletFireDirection_ = direction;

	// 弾の生成
	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(position, direction, 30.0f);

	// パーティクルエミッターに弾を登録
	bulletEmitter_.AddParticle(bullet.get());

	// 弾をリストに追加
	bullets_.emplace_back(std::move(bullet));

	shootCoolTime_ = maxShootCoolTime_;
}

//=============================================================================
// ミサイルの発射
void PlayerCombatComponent::ShootMissile(const Vector3 &position, const Vector3 &direction, EnemyBase *target) {
	if (!CanShootMissile()) {
		return;
	}

	// ミサイルの生成
	auto missile = std::make_unique<PlayerMissile>();
	missile->Initialize(position, direction, 20.0f);

	if (target) {
		missile->SetTarget(target);
		missile->SetLockedOn(true);
	}

	missiles_.push_back(std::move(missile));
	missileCoolTime_ = maxMissileCoolTime_;
}

//=============================================================================
// 描画
void PlayerCombatComponent::Draw() {
	if (!isInitialized_) {
		return;
	}

	// 各弾の描画
	for (const auto &bullet : bullets_) {
		if (bullet) {
			bullet->Draw();
		}
	}

	for (const auto &missile : missiles_) {
		if (missile) {
			missile->Draw();
		}
	}
}

//=============================================================================
// 弾の更新
void PlayerCombatComponent::UpdateBullets() {
	// 各弾の更新
	for (auto &bullet : bullets_) {
		if (bullet && bullet->IsAlive()) {
			bullet->Update();
		}
	}

	// 非アクティブな弾を削除
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
					   [](const std::unique_ptr<PlayerBullet> &b) { return !b || !b->IsAlive(); }),
		bullets_.end());
}

//=============================================================================
// ミサイルの更新
void PlayerCombatComponent::UpdateMissiles() {
	// 各ミサイルの更新
	for (auto &missile : missiles_) {
		if (missile && missile->IsAlive()) {
			missile->Update();
		}
	}

	// 非アクティブなミサイルを削除
	missiles_.erase(
		std::remove_if(missiles_.begin(), missiles_.end(),
					   [](const std::unique_ptr<PlayerMissile> &m) { return !m || !m->IsAlive(); }),
		missiles_.end());
}

//=============================================================================
// 弾の描画
void PlayerCombatComponent::DrawBullets() {
	// 各弾の描画
	for (auto &bullet : bullets_) {
		if (bullet && bullet->IsAlive()) {
			bullet->Draw();
		}
	}
}

//=============================================================================
// ミサイルの描画
void PlayerCombatComponent::DrawMissiles() {
	// 各ミサイルの描画
	for (auto &missile : missiles_) {
		if (missile && missile->IsAlive()) {
			missile->Draw();
		}
	}
}

//=============================================================================
// ImGui描画
void PlayerCombatComponent::DrawImGui() {
	if (!isInitialized_) {
		return;
	}

#ifdef _DEBUG
	ImGui::Begin("Player Combat Debug");

	// === 弾情報 ===
	ImGui::Text("=== Bullets ===");
	ImGui::Text("Active Bullets: %zu", bullets_.size());
	for (size_t i = 0; i < bullets_.size(); ++i) {
		if (bullets_[i]) {
			ImGui::Text("Bullet %zu: Position(%.2f, %.2f, %.2f)", i, bullets_[i]->GetPosition().x, bullets_[i]->GetPosition().y, bullets_[i]->GetPosition().z);
		}
	}

	ImGui::Separator();

	// === ミサイル情報 ===
	ImGui::Text("=== Missiles ===");
	ImGui::Text("Active Missiles: %zu", missiles_.size());
	for (size_t i = 0; i < missiles_.size(); ++i) {
		if (missiles_[i]) {
			ImGui::Text("Missile %zu: Position(%.2f, %.2f, %.2f)", i, missiles_[i]->GetPosition().x, missiles_[i]->GetPosition().y, missiles_[i]->GetPosition().z);
		}
	}

	ImGui::End();
#endif
}
#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "PlayerCombatComponent.h"
#include "EnemyManager.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>

namespace {
	template <class T>
	void UpdateProjectileList(std::vector<std::unique_ptr<T>> &items) {
		for (auto &item : items) {
			if (item) {
				item->Update();
			}
		}
		items.erase(
			std::remove_if(items.begin(), items.end(),
						   [](const std::unique_ptr<T> &item) { return !item || !item->IsAlive(); }),
			items.end());
	}
} // namespace

//=============================================================================
// 初期化
void PlayerCombatComponent::Initialize(MagEngine::Object3dSetup *object3dSetup) {
	object3dSetup_ = object3dSetup;
	enemyManager_ = nullptr;

	bullets_.clear();
	missiles_.clear();

	shootCoolTime_ = 0.0f;
	maxShootCoolTime_ = 0.1f;
	missileCoolTime_ = 0.0f;
	maxMissileCoolTime_ = 1.0f;
	bulletFireDirection_ = {0.0f, 0.0f, 1.0f};
}

//=============================================================================
// 更新
void PlayerCombatComponent::Update(float deltaTime) {
	// クールタイムの更新
	shootCoolTime_ = std::max(0.0f, shootCoolTime_ - deltaTime);
	missileCoolTime_ = std::max(0.0f, missileCoolTime_ - deltaTime);
}

//=============================================================================
// 弾発射
void PlayerCombatComponent::ShootBullet(const Vector3 &position, const Vector3 &direction) {
	if (!CanShootBullet()) {
		return;
	}

	// 発射方向を記録（HUD用）
	bulletFireDirection_ = direction;

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(object3dSetup_, "Bullet.obj", position, direction);
	bullets_.push_back(std::move(bullet));
	shootCoolTime_ = maxShootCoolTime_;
}

//=============================================================================
// ミサイル発射
void PlayerCombatComponent::ShootMissile(const Vector3 &position, const Vector3 &direction, EnemyBase *target) { // Enemy* から EnemyBase* に変更
	if (!CanShootMissile()) {
		return;
	}

	auto missile = std::make_unique<PlayerMissile>();
	missile->Initialize(object3dSetup_, "Bullet.obj", position, direction);
	missile->SetEnemyManager(enemyManager_);

	if (target) {
		missile->SetTarget(target);
		missile->StartLockOn();
	}

	missiles_.push_back(std::move(missile));
	missileCoolTime_ = maxMissileCoolTime_;
}

//=============================================================================
// 弾の更新
void PlayerCombatComponent::UpdateBullets() {
	UpdateProjectileList(bullets_);
}

void PlayerCombatComponent::UpdateMissiles() {
	UpdateProjectileList(missiles_);
}

//=============================================================================
// 弾の描画
void PlayerCombatComponent::DrawBullets() {
	for (auto &bullet : bullets_) {
		bullet->Draw();
	}
}

void PlayerCombatComponent::DrawMissiles() {
	for (auto &missile : missiles_) {
		missile->Draw();
#ifdef _DEBUG
		missile->DrawDebugInfo();
#endif
	}
}

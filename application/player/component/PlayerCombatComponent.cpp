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

//=============================================================================
// マルチロックオンで複数敵に同時発射
void PlayerCombatComponent::ShootMultipleMissiles(const Vector3 &position, const Vector3 &direction,
												  const std::vector<EnemyBase *> &targets) {
	if (targets.empty() || !CanShootMissile()) {
		return;
	}

	// 複数ターゲットごとに個別のミサイルを発射
	// （ただしクールタイムは全体で一度だけ消費）
	int missileCount = 0;
	const float staggerDelay = 0.1f; // ミサイル発射の時差（秒）

	for (size_t i = 0; i < targets.size(); ++i) {
		if (!targets[i]) {
			continue;
		}

		if (missileCount > 0) {
			// クールタイムをリセット（複数同時発射対応）
			missileCoolTime_ = 0.0f;
		}

		auto missile = std::make_unique<PlayerMissile>();
		missile->Initialize(object3dSetup_, "Bullet.obj", position, direction);
		missile->SetEnemyManager(enemyManager_);
		missile->SetTarget(targets[i]);
		missile->StartLockOn();

		// 着弾時間を調整して、ターゲット敵での着弾を分散させる
		float adjustedHitTime = 3.0f + (i * 0.5f);
		missile->SetDesiredHitTime(adjustedHitTime);

		// 発射時の速度オフセットを設定（複数ミサイルが同じ方向になるのを防ぐ）
		if (i > 0) {
			float angleOffset = (i - 0) * 15.0f; // ミサイル間の角度オフセット
			float rad = angleOffset * MagMath::PI / 180.0f;
			Vector3 offsetDir = {
				direction.x * std::cos(rad) - direction.z * std::sin(rad),
				direction.y,
				direction.x * std::sin(rad) + direction.z * std::cos(rad)};

			// 発射初速オフセットを設定
			Vector3 velocityOffset = {
				offsetDir.x * 10.0f - direction.x * 10.0f,
				offsetDir.y * 5.0f,
				offsetDir.z * 10.0f - direction.z * 10.0f};
			missile->SetLaunchVelocityOffset(velocityOffset, 0.2f);
		}

		missiles_.push_back(std::move(missile));
		missileCount++;
	}

	// クールタイムを設定（最後のミサイル発射後）
	missileCoolTime_ = maxMissileCoolTime_;
}

#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "PlayerCombatComponent.h"
#include "PlayerLockedOnComponent.h"
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
void PlayerCombatComponent::Initialize(MagEngine::Object3dSetup *object3dSetup,
									   MagEngine::TrailEffectManager *trailEffectManager) {
	object3dSetup_ = object3dSetup;
	trailEffectManager_ = trailEffectManager;
	enemyManager_ = nullptr;

	bullets_.clear();
	missiles_.clear();

	shootCoolTime_ = 0.0f;
	maxShootCoolTime_ = 0.1f;
	missileAmmo_ = 3;
	maxMissileAmmo_ = 3;
	missileRecoveryTimer_ = 0.0f;
	maxMissileRecoveryTime_ = 3.0f; // 3秒で1発回復
	bulletFireDirection_ = {0.0f, 0.0f, 1.0f};

	// デフォルトモデルパス
	bulletModelPath_ = "Bullet.obj";
	missileModelPath_ = "Bullet.obj";
}

//=============================================================================
// 更新
void PlayerCombatComponent::Update(float deltaTime) {
	// 弾のクールタイム更新
	shootCoolTime_ = std::max(0.0f, shootCoolTime_ - deltaTime);

	// ミサイルの回復タイマー更新（3秒ごとに1発回復）
	if (missileAmmo_ < maxMissileAmmo_) {
		missileRecoveryTimer_ += deltaTime;
		if (missileRecoveryTimer_ >= maxMissileRecoveryTime_) {
			missileAmmo_++;
			missileRecoveryTimer_ = 0.0f;
			// 最大残弾数を超えないようにクリップ
			missileAmmo_ = std::min(missileAmmo_, maxMissileAmmo_);
		}
	}
}

//=============================================================================
// 弾発射
void PlayerCombatComponent::ShootBullet(const Vector3 &position, const Vector3 &direction) {
	if (!CanShootBullet()) {
		return;
	}

	// 発射方向の計算（アシスト機能）
	Vector3 finalDirection = direction;

	// アシスト機能が有効かつコンポーネントが有効な場合
	if (isBulletAssistEnabled_ && enemyManager_) {
		// 敵検出用の簡易ロックオンコンポーネント（ローカルで使用）
		PlayerLockedOnComponent bulletAssistComponent;
		bulletAssistComponent.Initialize(enemyManager_);

		// アシスト対象の敵を検出
		EnemyBase *assistTarget = bulletAssistComponent.FindBulletAssistTarget(position, direction, bulletAssistFOV_, bulletAssistRange_);

		if (assistTarget) {
			// 敵への方向ベクトルを計算
			const Vector3 targetPos = assistTarget->GetPosition();
			const Vector3 toTarget = {
				targetPos.x - position.x,
				targetPos.y - position.y,
				targetPos.z - position.z};

			const float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

			if (distance > 0.001f) {
				const float invDist = 1.0f / distance;
				const Vector3 targetDir = {
					toTarget.x * invDist,
					toTarget.y * invDist,
					toTarget.z * invDist};

				// 元の方向とアシスト方向を補間
				// bulletAssistStrength_ = 1.0で敵に直進、0.0で元の方向
				finalDirection.x = direction.x * (1.0f - bulletAssistStrength_) + targetDir.x * bulletAssistStrength_;
				finalDirection.y = direction.y * (1.0f - bulletAssistStrength_) + targetDir.y * bulletAssistStrength_;
				finalDirection.z = direction.z * (1.0f - bulletAssistStrength_) + targetDir.z * bulletAssistStrength_;

				// 正規化
				const float finalLen = std::sqrt(finalDirection.x * finalDirection.x + finalDirection.y * finalDirection.y + finalDirection.z * finalDirection.z);
				if (finalLen > 0.001f) {
					finalDirection.x /= finalLen;
					finalDirection.y /= finalLen;
					finalDirection.z /= finalLen;
				}
			}
		}
	}

	// 最終的な発射方向を記録（HUD用）
	bulletFireDirection_ = finalDirection;

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(object3dSetup_, trailEffectManager_, bulletModelPath_, position, finalDirection);
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
	missile->Initialize(object3dSetup_, trailEffectManager_, missileModelPath_, position, direction);
	missile->SetEnemyManager(enemyManager_);

	if (target) {
		missile->SetTarget(target);
		missile->StartLockOn();
	}

	missiles_.push_back(std::move(missile));
	missileAmmo_--;				  // 残弾を消費
	missileRecoveryTimer_ = 0.0f; // 回復タイマーをリセット
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

void PlayerCombatComponent::DrawBulletsTrails() {
	for (auto &bullet : bullets_) {
		bullet->DrawTrail();
	}
}

void PlayerCombatComponent::DrawMissilesTrails() {
	for (auto &missile : missiles_) {
		missile->DrawTrail();
	}
}

//=============================================================================
// マルチロックオンで複数敵に同時発射（1体につき1ミサイル）
void PlayerCombatComponent::ShootMultipleMissiles(const Vector3 &position, const Vector3 &direction,
												  const std::vector<EnemyBase *> &targets) {
	if (targets.empty() || !CanShootMissile()) {
		return;
	}

	// DEBUG: 発射ミサイル情報をログ出力
	// printf("ShootMultipleMissiles: Firing %zu missiles\n", targets.size());

	// ロック対象の敵の数分だけミサイルを発射（1体=1ミサイル）
	const size_t fireCount = std::min(targets.size(), static_cast<size_t>(missileAmmo_));
	int firedMissileCount = 0;
	for (size_t i = 0; i < fireCount; ++i) {
		if (!targets[i]) {
			continue;
		}

		// DEBUG: 各ミサイルのターゲット情報をログ出力
		// printf("  Missile %zu -> Target %p at (%.2f, %.2f, %.2f)\n", i, (void*)targets[i],
		//        targets[i]->GetPosition().x, targets[i]->GetPosition().y, targets[i]->GetPosition().z);

		auto missile = std::make_unique<PlayerMissile>();
		missile->Initialize(object3dSetup_, trailEffectManager_, missileModelPath_, position, direction);
		missile->SetEnemyManager(enemyManager_);
		missile->SetTarget(targets[i]);
		missile->StartLockOn();

		// 着弾時間を調整して、ターゲット敵での到着をずらす
		float adjustedHitTime = 3.0f + (i * 0.3f);
		missile->SetDesiredHitTime(adjustedHitTime);

		// 発射時の速度オフセットを設定（複数ミサイルが異なる軌道を描くようにする）
		if (i > 0) {
			float angleOffset = i * 12.0f; // ミサイル間の角度オフセット
			float rad = angleOffset * MagMath::PI / 180.0f;
			Vector3 offsetDir = {
				direction.x * std::cos(rad) - direction.z * std::sin(rad),
				direction.y,
				direction.x * std::sin(rad) + direction.z * std::cos(rad)};

			// 発射初速オフセットを設定
			Vector3 velocityOffset = {
				offsetDir.x * 8.0f - direction.x * 8.0f,
				offsetDir.y * 3.0f,
				offsetDir.z * 8.0f - direction.z * 8.0f};
			missile->SetLaunchVelocityOffset(velocityOffset, 0.2f);
		}

		missiles_.push_back(std::move(missile));
		++firedMissileCount;
	}

	if (firedMissileCount > 0) {
		missileAmmo_ = std::max(0, missileAmmo_ - firedMissileCount); // 発射数分だけ残弾を消費
		missileRecoveryTimer_ = 0.0f;								  // 回復タイマーをリセット
	}
}

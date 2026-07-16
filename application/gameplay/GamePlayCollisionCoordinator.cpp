#include "GamePlayCollisionCoordinator.h"

#include "CollisionManager.h"
#include "EnemyBase.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "Player.h"
#include "PlayerBullet.h"
#include "PlayerMissile.h"

GamePlayCollisionCoordinator::GamePlayCollisionCoordinator(CollisionManager &collisionManager)
	: collisionManager_(collisionManager) {
}

void GamePlayCollisionCoordinator::Execute(Player *player, EnemyManager *enemyManager) {
	// 前フレームの衝突状態はCollisionManagerが維持し、登録対象だけを再構築する。
	collisionManager_.BeginFrame();

	RegisterPlayer(player);
	RegisterEnemies(enemyManager);
	RegisterPlayerProjectiles(player);
	RegisterEnemyProjectiles(enemyManager);

	collisionManager_.Update();
}

void GamePlayCollisionCoordinator::RegisterPlayer(Player *player) {
	if (player) {
		collisionManager_.RegisterObject(player);
	}
}

void GamePlayCollisionCoordinator::RegisterEnemies(EnemyManager *enemyManager) {
	if (!enemyManager) {
		return;
	}

	for (const auto &enemy : enemyManager->GetEnemies()) {
		if (enemy && enemy->IsAlive() && !enemy->IsInHitReaction()) {
			collisionManager_.RegisterObject(enemy.get());
		}
	}
}

void GamePlayCollisionCoordinator::RegisterPlayerProjectiles(Player *player) {
	if (!player) {
		return;
	}

	for (const auto &bullet : player->GetBullets()) {
		if (bullet->IsAlive()) {
			collisionManager_.RegisterObject(bullet.get());
		}
	}

	for (const auto &missile : player->GetMissiles()) {
		if (missile->IsAlive()) {
			collisionManager_.RegisterObject(missile.get());
		}
	}
}

void GamePlayCollisionCoordinator::RegisterEnemyProjectiles(EnemyManager *enemyManager) {
	if (!enemyManager) {
		return;
	}

	// EnemyManagerの所有権を変えず、前フレームの容量を再利用して登録対象だけを収集する。
	enemyManager->CollectEnemyBullets(enemyBulletBuffer_);
	for (EnemyBullet *bullet : enemyBulletBuffer_) {
		if (bullet) {
			collisionManager_.RegisterObject(bullet);
		}
	}
}

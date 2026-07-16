#pragma once

#include <vector>

class CollisionManager;
class EnemyBullet;
class EnemyManager;
class Player;

///=============================================================================
///                         ゲームプレイ衝突登録調整
///
/// GamePlayScene が決定した更新位置で、ゲーム固有オブジェクトを
/// CollisionManager へ登録する。オブジェクトの所有・破棄は担当しない。
class GamePlayCollisionCoordinator {
public:
	explicit GamePlayCollisionCoordinator(CollisionManager &collisionManager);

	void Execute(Player *player, EnemyManager *enemyManager);

private:
	void RegisterPlayer(Player *player);
	void RegisterEnemies(EnemyManager *enemyManager);
	void RegisterPlayerProjectiles(Player *player);
	void RegisterEnemyProjectiles(EnemyManager *enemyManager);

	CollisionManager &collisionManager_;
	std::vector<EnemyBullet *> enemyBulletBuffer_;
};

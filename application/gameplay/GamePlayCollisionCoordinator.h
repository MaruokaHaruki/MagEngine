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
	/// @brief 衝突登録先を指定して生成
	/// @note collisionManagerの所有権は取得しない。Coordinatorの寿命中は有効でなければならない。
	explicit GamePlayCollisionCoordinator(CollisionManager &collisionManager);

	/// @brief 現フレームで有効なゲームプレイ用Colliderを登録
	/// @note Sceneの更新後、CollisionManagerの判定前に呼び出す。引数がnullptrの場合は該当登録を行わない。
	void Execute(Player *player, EnemyManager *enemyManager);

private:
	void RegisterPlayer(Player *player);
	void RegisterEnemies(EnemyManager *enemyManager);
	void RegisterPlayerProjectiles(Player *player);
	void RegisterEnemyProjectiles(EnemyManager *enemyManager);

	CollisionManager &collisionManager_; // Sceneが所有する衝突管理への非所有参照。
	std::vector<EnemyBullet *> enemyBulletBuffer_; // 毎フレームの敵弾収集で一時確保を繰り返さないための作業領域。
};

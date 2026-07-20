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
	/// @param player プレイヤー本体と発射済み投射物の登録元。nullptrの場合は登録しない。
	/// @param enemyManager 敵本体と敵弾の登録元。nullptrの場合は登録しない。
	/// @note Sceneの更新後、CollisionManagerの判定前に呼び出す。引数がnullptrの場合は該当登録を行わない。
	void Execute(Player *player, EnemyManager *enemyManager);

private:
	/// @brief プレイヤー本体のColliderを登録する
	/// @param player 登録するプレイヤー。nullptrの場合は何もしない。
	void RegisterPlayer(Player *player);
	/// @brief 生存中の敵Colliderを登録する
	/// @param enemyManager 敵一覧を取得する管理クラス。nullptrの場合は何もしない。
	void RegisterEnemies(EnemyManager *enemyManager);
	/// @brief 生存中のプレイヤー弾とミサイルを登録する
	/// @param player 投射物一覧を取得するプレイヤー。nullptrの場合は何もしない。
	void RegisterPlayerProjectiles(Player *player);
	/// @brief 生存中の敵弾を作業領域へ収集して登録する
	/// @param enemyManager 敵弾一覧を取得する管理クラス。nullptrの場合は何もしない。
	void RegisterEnemyProjectiles(EnemyManager *enemyManager);

	CollisionManager &collisionManager_; // Sceneが所有する衝突管理への非所有参照。
	std::vector<EnemyBullet *> enemyBulletBuffer_; // 毎フレームの敵弾収集で一時確保を繰り返さないための作業領域。
};

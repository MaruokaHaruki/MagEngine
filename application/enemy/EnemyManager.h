/*********************************************************************
 * \file   EnemyManager.h
 * \brief  敵の一括管理管理クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "EnemyBase.h" // Enemy.h から EnemyBase.h に変更
#include <memory>
#include <vector>

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;
class CollisionManager;
class Player;

namespace EnemyManagerConstants {
	constexpr float kDefaultSpawnInterval = 3.0f;
	constexpr int kDefaultMaxEnemies = 10;
	constexpr int kDefaultTargetDefeatedCount = 15;
	constexpr float kSpawnRangeX = 5.0f;
	constexpr float kSpawnRangeY = 1.0f;
	constexpr float kSpawnDistanceMin = 20.0f;
	constexpr float kSpawnDistanceMax = 30.0f;
}

/// \brief 敵のタイプ
enum class EnemyType {
	Normal,
	Fast
	// Boss // 将来的に追加予定
};

/// \brief スポーン情報（手動スポーン用 - オプション）
struct SpawnInfo {
	EnemyType type;
	Vector3 position;
	float spawnTime; // スポーンする時間
	bool spawned;	 // スポーン済みフラグ
};

///=============================================================================
///						EnemyManagerクラス
class EnemyManager {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, Particle *particle, ParticleSetup *particleSetup);

	/// \brief プレイヤー参照の設定
	void SetPlayer(Player *player) {
		player_ = player;
	}

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	/// \brief 当たり判定登録
	void RegisterCollisions(CollisionManager *collisionManager);

	/// \brief 全敵削除
	void Clear();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief 敵のスポーン処理(通常)
	void UpdateSpawning();

	/// \brief 敵の生成
	void SpawnEnemy(EnemyType type, const Vector3 &position);

	/// \brief 死んだ敵の削除
	void RemoveDeadEnemies();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief 生存敵数の取得
	size_t GetAliveEnemyCount() const;

	/// \brief 撃破数の取得
	int GetDefeatedCount() const {
		return defeatedCount_;
	}

	/// \brief 目標撃破数の設定
	void SetTargetDefeatedCount(int count) {
		targetDefeatedCount_ = count;
	}

	/// \brief 目標撃破数の取得
	int GetTargetDefeatedCount() const {
		return targetDefeatedCount_;
	}

	/// \brief クリア条件達成チェック
	bool IsGameClear() const {
		return defeatedCount_ >= targetDefeatedCount_;
	}

	/// \brief 敵リストの取得（当たり判定用）
	const std::vector<std::unique_ptr<EnemyBase>> &GetEnemies() const {
		return enemies_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 敵管理（EnemyBase のリストに変更）
	std::vector<std::unique_ptr<EnemyBase>> enemies_;

	//========================================
	// スポーン管理
	std::vector<SpawnInfo> spawnQueue_; // 手動スポーン用（オプション）
	float gameTime_;					// ゲーム経過時間
	float lastSpawnTime_;				// 最後のスポーン時間
	float spawnInterval_;				// スポーン間隔

	//========================================
	// システム参照
	Object3dSetup *object3dSetup_;
	Particle *particle_;
	ParticleSetup *particleSetup_;
	Player *player_; // プレイヤー参照

	//========================================
	// 設定パラメータ
	int maxEnemies_; // 最大敵数
	bool autoSpawn_; // 自動スポーンフラグ

	//========================================
	// ゲーム進行管理
	int defeatedCount_;		  // 撃破数
	int targetDefeatedCount_; // 目標撃破数
};
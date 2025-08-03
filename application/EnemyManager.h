/*********************************************************************
 * \file   EnemyManager.h
 * \brief  敵の一括管理管理クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "Enemy.h"
#include <memory>
#include <vector>

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;
class CollisionManager;

/// \brief 敵のタイプ
enum class EnemyType {
	Normal, // 通常敵
	Fast,	// 高速敵
	Heavy,	// 重装敵
	Bomber	// 爆撃機
};

/// \brief スポーン情報
struct SpawnInfo {
	EnemyType type;
	Vector3 startPosition;   // 開始位置（画面外）
	Vector3 targetPosition;  // 目標位置（画面内）
	float speed;             // 移動速度
	float spawnTime;         // スポーンする時間
	bool spawned;            // スポーン済みフラグ
};

/// \brief スポーンパターン
enum class SpawnPattern {
	FromLeft,       // 左から
	FromRight,      // 右から
	FromTop,        // 上から
	FromBack,       // 奥から
	FromFront,      // 手前から
	Random          // ランダム
};

///=============================================================================
///						EnemyManagerクラス
class EnemyManager {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, Particle *particle, ParticleSetup *particleSetup);

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

	/// \brief スポーン情報の初期化
	void InitializeSpawnData();

	/// \brief パターンベースのスポーン位置生成
	SpawnInfo CreateSpawnInfo(EnemyType type, SpawnPattern pattern, float gameTime, float speed = 5.0f);

	/// \brief ランダムスポーン位置の生成
	Vector3 GenerateRandomSpawnPosition(SpawnPattern pattern);

	/// \brief ランダム目標位置の生成
	Vector3 GenerateRandomTargetPosition();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief 生存敵数の取得
	size_t GetAliveEnemyCount() const;

	/// \brief 敵リストの取得（当たり判定用）
	const std::vector<std::unique_ptr<Enemy>> &GetEnemies() const {
		return enemies_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 敵管理
	std::vector<std::unique_ptr<Enemy>> enemies_; // 敵のリスト

	//========================================
	// スポーン管理
	std::vector<SpawnInfo> spawnQueue_; // スポーン待ち敵の情報
	float gameTime_;					// ゲーム経過時間
	float lastSpawnTime_;				// 最後のスポーン時間
	float spawnInterval_;				// スポーン間隔

	//========================================
	// システム参照
	Object3dSetup *object3dSetup_;
	Particle *particle_;
	ParticleSetup *particleSetup_;

	//========================================
	// 設定パラメータ
	int maxEnemies_;                     // 最大敵数
	bool autoSpawn_;                     // 自動スポーンフラグ
	std::vector<SpawnPattern> spawnPatterns_; // 使用するスポーンパターン
	
	//========================================
	// スポーン範囲設定
	float spawnDistance_;                // スポーン距離（画面外の距離）
	Vector3 playAreaMin_;               // プレイエリアの最小値
	Vector3 playAreaMax_;               // プレイエリアの最大値
};
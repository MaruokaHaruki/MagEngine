/*********************************************************************
 * \file   EnemyManager.h
 * \brief  敵の一括管理クラス（ウェーブシステム管理）
 *
 * \author Harukichimaru
 * \date   January 2025
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "EnemyBase.h"
#include <memory>
#include <vector>
#include "WaveParamConfig.h"
#include "FormationConfig.h"

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;
class CollisionManager;
class Player;
class EnemyBullet;
#include "EnemyGroup.h"

///=============================================================================
///						ウェーブ設定
struct WaveConfig {
	int enemyCount;		 // 通常エネミー数
	int gunnerCount;	 // ガンナー数
	float spawnInterval; // スポーン間隔（秒）
	float formationRatio;	  // 編隊敵の割合（0.0 - 1.0）
	int maxGroupSize;		  // グループの最大メンバ数（デフォルト: 8）
	int formationPattern;	  // 編隊パターン選択用インデックス（0-4）
};

///=============================================================================
///						EnemyManagerクラス
class EnemyManager {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::Particle *particle,
					MagEngine::ParticleSetup *particleSetup);

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
	///							入出力関数
public:
	/// \brief 生存敵数の取得
	size_t GetAliveEnemyCount() const;

	/// \brief 撃破数の取得
	int GetDefeatedCount() const {
		return defeatedCount_;
	}

	/// \brief 目標撃破数の取得
	int GetTargetDefeatedCount() const {
		return targetDefeatedCount_;
	}

	/// \brief 現在ウェーブ番号の取得（1始まり）
	int GetCurrentWave() const {
		return currentWave_ + 1;
	}

	/// \brief 総ウェーブ数の取得
	int GetTotalWaves() const {
		return static_cast<int>(waveConfigs_.size());
	}

	/// \brief クリア条件達成チェック
	bool IsGameClear() const {
		return defeatedCount_ >= targetDefeatedCount_;
	}

	/// \brief 敵リストの取得（当たり判定用）
	const std::vector<std::unique_ptr<EnemyBase>> &GetEnemies() const {
		return enemies_;
	}

	/// \brief 敵の弾をすべて取得
	std::vector<EnemyBullet *> GetAllEnemyBullets();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief ウェーブ進行管理
	void UpdateWave();

	/// \brief 敵の生成
	void SpawnEnemy(const Vector3 &position);

	/// \brief ガンナータイプの生成
	void SpawnGunner(const Vector3 &position);

	/// \brief スポーン位置の生成
	Vector3 GenerateSpawnPosition() const;

	/// \brief 死んだ敵の削除
	void RemoveDeadEnemies();

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 敵管理
	std::vector<std::unique_ptr<EnemyBase>> enemies_;

	//========================================
	// 編隊管理
	std::vector<std::unique_ptr<EnemyGroup>> groups_;
	int nextGroupId_;

	//========================================
	// ウェーブシステム
	enum class WavePhase {
		Spawning,	 // ウェーブ内の敵をスポーン中
		Active,		 // 全スポーン完了・撃破待ち
		BetweenWaves // ウェーブ間インターバル
	};
	std::vector<WaveConfig> waveConfigs_; // ウェーブ定義
	int currentWave_;					  // 現在のウェーブ番号（0始まり）
	WavePhase wavePhase_;				  // ウェーブフェーズ
	float waveTimer_;					  // ウェーブ内タイマー（スポーン間隔用）
	float betweenWaveTimer_;			  // ウェーブ間待機タイマー
	int spawnedInWave_;					  // 現在ウェーブでスポーン済みの総数

	//========================================
	// ゲーム進行管理
	int defeatedCount_;		  // 撃破数
	int targetDefeatedCount_; // 目標撃破数（全ウェーブ合計）

	//========================================
	// ゲーム経過時間
	float gameTime_;

	//========================================
	// システム参照
	MagEngine::Object3dSetup *object3dSetup_;
	MagEngine::Particle *particle_;
	MagEngine::ParticleSetup *particleSetup_;
	Player *player_;
};

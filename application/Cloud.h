#pragma once
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Vector3.h"
#include <memory>
#include <numbers> // std::numbers::pi_v
#include <random>
#include <vector>

class Cloud {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	/// \param particle パーティクルシステムのポインタ
	/// \param particleSetup パーティクルセットアップのポインタ
	void Initialize(Particle *particle, ParticleSetup *particleSetup);

	/// \brief 更新
	/// \param playerPosition プレイヤーの位置（雲の生成範囲の基準）
	void Update(const Vector3 &playerPosition);

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							プライベートメンバ関数
private:
	/// \brief 雲エミッターの作成
	/// \param basePosition 基準位置
	void CreateCloudEmitter(const Vector3 &basePosition);

	/// \brief 雲エミッターの設定
	/// \param emitter エミッターのポインタ
	void ConfigureCloudEmitter(ParticleEmitter *emitter);

	/// \brief 風の速度計算
	/// \return 風の速度ベクトル
	Vector3 CalculateWindVelocity();

	/// \brief 雲位置の更新
	/// \param playerPosition プレイヤーの位置
	void UpdateCloudPositions(const Vector3 &playerPosition);

	/// \brief 新しい雲の生成チェック
	/// \param playerPosition プレイヤーの位置
	void CheckAndSpawnClouds(const Vector3 &playerPosition);

	/// \brief 遠い雲の削除
	/// \param playerPosition プレイヤーの位置
	void RemoveDistantClouds(const Vector3 &playerPosition);

	void CreateCloudCarpet();

	///--------------------------------------------------------------
	///							メンバ変数
private:
	// パーティクルシステムのポインタ
	Particle *particle_ = nullptr;
	ParticleSetup *particleSetup_ = nullptr;

	// 雲エミッター群
	std::vector<std::unique_ptr<ParticleEmitter>> cloudEmitters_;

	// 雲システムのパラメータ
	struct CloudSystemParams {
		int cloudCount = 500;							 // 雲の数をさらに大幅増加
		float spawnRadius = 50.0f;						 // 生成半径を大幅縮小（原点周辺に集中）
		float cloudSpeed = 8.0f;						 // 雲の移動速度を上げる
		float cloudHeight = 25.0f;						 // 雲の高さ
		float heightVariation = 15.0f;					 // 高さのばらつきを縮小
		float windDirection = std::numbers::pi_v<float>; // 風の方向をπ（180度）に設定（Z+からZ-方向）
		float respawnDistance = 150.0f;					 // 再生成距離を縮小
		bool enableWind = true;							 // 風の有効/無効
	} params_;

	// 雲の生成用パラメータ
	struct CloudParams {
		Vector3 position;
		Vector3 velocity;
		float lifeTime;
		bool isActive;
	};
	std::vector<CloudParams> cloudParams_;

	// 雲の生成タイマー
	float spawnTimer_ = 0.0f;
	const float spawnInterval_ = 0.01f; // 雲生成間隔を少し長めに調整

	// 乱数生成器
	std::mt19937 randomEngine_;
	std::random_device randomDevice_;
};

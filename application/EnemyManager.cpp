/*********************************************************************
 * \file   EnemyManager.cpp
 * \brief  敵の一括管理管理クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 *********************************************************************/
#include "EnemyManager.h"
#include "CollisionManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "Player.h"
#include <algorithm>

///=============================================================================
///                        初期化
void EnemyManager::Initialize(Object3dSetup *object3dSetup, Particle *particle, ParticleSetup *particleSetup) {
	//========================================
	// システム参照の設定
	object3dSetup_ = object3dSetup;
	particle_ = particle;
	particleSetup_ = particleSetup;
	player_ = nullptr; // プレイヤー参照初期化

	//========================================
	// パラメータ初期化
	gameTime_ = 0.0f;
	lastSpawnTime_ = 0.0f;
	spawnInterval_ = 3.0f; // 3秒間隔
	maxEnemies_ = 10;	   // 最大10体
	autoSpawn_ = true;	   // 自動スポーン有効

	//========================================
	// スポーンデータの初期化
	spawnQueue_.clear();
	spawnQueue_.push_back({EnemyType::Normal, {0.0f, 0.0f, 15.0f}, 2.0f, false});
	spawnQueue_.push_back({EnemyType::Fast, {0.0f, 0.0f, 20.0f}, 5.0f, false});
	spawnQueue_.push_back({EnemyType::Normal, {0.0f, 0.0f, 18.0f}, 8.0f, false});
}

///=============================================================================
///                        更新
void EnemyManager::Update() {
	const float frameTime = 1.0f / 60.0f;
	gameTime_ += frameTime;

	//========================================
	// スポーン処理
	UpdateSpawning();

	//========================================
	// 敵の更新
	for (auto &enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	//========================================
	// 死んだ敵の削除
	RemoveDeadEnemies();
}

///=============================================================================
///                        描画
void EnemyManager::Draw() {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			enemy->Draw();

			// 敵の視覚化（デバッグ用）
			LineManager *lineManager = LineManager::GetInstance();
			Vector3 enemyPos = enemy->GetPosition();

			// 敵の周辺に赤い円を描画
			lineManager->DrawCircle(enemyPos, 1.0f, {1.0f, 0.0f, 0.0f, 0.7f}, 2.0f);

			// 敵の座標軸表示
			lineManager->DrawCoordinateAxes(enemyPos, 0.5f, 1.0f);
		}
	}
}

///=============================================================================
///                        ImGui描画
void EnemyManager::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Enemy Manager");

	ImGui::Text("Game Time: %.1f", gameTime_);
	ImGui::Text("Alive Enemies: %zu / %d", GetAliveEnemyCount(), maxEnemies_);

	ImGui::Separator();
	ImGui::SliderFloat("Spawn Interval", &spawnInterval_, 0.5f, 10.0f);
	ImGui::SliderInt("Max Enemies", &maxEnemies_, 1, 20);
	ImGui::Checkbox("Auto Spawn", &autoSpawn_);

	if (ImGui::Button("Spawn Normal Enemy")) {
		SpawnEnemy(EnemyType::Normal, {0.0f, 0.0f, 30.0f});
	}
	if (ImGui::Button("Spawn Fast Enemy")) {
		SpawnEnemy(EnemyType::Fast, {3.0f, 0.0f, 15.0f});
	}
	if (ImGui::Button("Clear All Enemies")) {
		Clear();
	}

	ImGui::End();
#endif
}

///=============================================================================
///                        当たり判定登録
void EnemyManager::RegisterCollisions(CollisionManager *collisionManager) {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			collisionManager->RegisterObject(enemy.get());
		}
	}
}

///=============================================================================
///                        全敵削除
void EnemyManager::Clear() {
	enemies_.clear();
}

///=============================================================================
///                        スポーン処理
void EnemyManager::UpdateSpawning() {
	//========================================
	// 予定されたスポーンの処理
	for (auto &spawnInfo : spawnQueue_) {
		if (!spawnInfo.spawned && gameTime_ >= spawnInfo.spawnTime) {
			SpawnEnemy(spawnInfo.type, spawnInfo.position);
			spawnInfo.spawned = true;
		}
	}

	//========================================
	// 自動スポーン
	if (autoSpawn_ &&
		GetAliveEnemyCount() < static_cast<size_t>(maxEnemies_) &&
		gameTime_ - lastSpawnTime_ >= spawnInterval_) {

		// プレイヤーの後方にスポーン位置を設定（+Z方向に進行するため後方は-Z）
		Vector3 spawnPos = {0.0f, 0.0f, -15.0f}; // デフォルト位置（プレイヤーの後方）
		if (player_) {
			Vector3 playerPos = player_->GetPosition();
			// プレイヤーの後方10～15ユニット、左右にランダムオフセット
			spawnPos = {
				playerPos.x + static_cast<float>((rand() % 11) - 5), // -5 ～ 5
				playerPos.y + static_cast<float>((rand() % 3) - 1),	 // -1 ～ 1
				playerPos.z - static_cast<float>((rand() % 6) + 10)	 // -10 ～ -15（プレイヤーの後方）
			};
		}

		EnemyType type = static_cast<EnemyType>(rand() % 2); // Normal または Fast
		SpawnEnemy(type, spawnPos);
		lastSpawnTime_ = gameTime_;
	}
}

///=============================================================================
///                        敵の生成
void EnemyManager::SpawnEnemy(EnemyType type, const Vector3 &position) {
	auto enemy = std::make_unique<Enemy>();

	// 敵の初期化
	enemy->Initialize(object3dSetup_, "jet.obj", position);

	// +Z方向への直進飛行を設定
	switch (type) {
	case EnemyType::Normal:
		enemy->SetMovementDirection(15.0f, {0.0f, 0.0f, 1.0f}); // +Z方向に15.0fの速度
		break;

	case EnemyType::Fast:
		enemy->SetMovementDirection(22.0f, {0.0f, 0.0f, 1.0f}); // +Z方向に22.0fの速度
		break;
	}

	// パーティクルシステムの設定
	enemy->SetParticleSystem(particle_, particleSetup_);

	enemies_.push_back(std::move(enemy));
}

///=============================================================================
///                        死んだ敵の削除
void EnemyManager::RemoveDeadEnemies() {
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
					   [](const std::unique_ptr<Enemy> &enemy) {
						   return !enemy || !enemy->IsAlive();
					   }),
		enemies_.end());
}

///=============================================================================
///                        生存敵数の取得
size_t EnemyManager::GetAliveEnemyCount() const {
	return std::count_if(enemies_.begin(), enemies_.end(),
						 [](const std::unique_ptr<Enemy> &enemy) {
							 return enemy && enemy->IsAlive();
						 });
}

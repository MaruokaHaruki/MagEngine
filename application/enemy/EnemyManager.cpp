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
	// ゲーム進行管理
	defeatedCount_ = 0;
	targetDefeatedCount_ = 50; // デフォルトは50体撃破でクリア

	//========================================
	// スポーンデータの初期化（簡略化 - 自動スポーンに任せる）
	spawnQueue_.clear();
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
	ImGui::Text("Defeated: %d / %d", defeatedCount_, targetDefeatedCount_);

	if (IsGameClear()) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GAME CLEAR!");
	}

	ImGui::Separator();
	ImGui::SliderFloat("Spawn Interval", &spawnInterval_, 0.5f, 10.0f);
	ImGui::SliderInt("Max Enemies", &maxEnemies_, 1, 20);
	ImGui::SliderInt("Target Defeated", &targetDefeatedCount_, 1, 50);
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
	// 予定されたスポーンの処理（キューベースのスポーン）
	for (auto &spawnInfo : spawnQueue_) {
		if (!spawnInfo.spawned && gameTime_ >= spawnInfo.spawnTime) {
			SpawnEnemy(spawnInfo.type, spawnInfo.position);
			spawnInfo.spawned = true;
		}
	}

	//========================================
	// 自動スポーン（最適化版）
	if (autoSpawn_ &&
		GetAliveEnemyCount() < static_cast<size_t>(maxEnemies_) &&
		gameTime_ - lastSpawnTime_ >= spawnInterval_) {

		// プレイヤーの後方にスポーン位置を設定
		Vector3 spawnPos = {0.0f, 0.0f, -15.0f};
		if (player_) {
			Vector3 playerPos = player_->GetPosition();
			spawnPos = {
				playerPos.x + static_cast<float>((rand() % 11) - 5), // -5 ～ 5
				playerPos.y + static_cast<float>((rand() % 3) - 1),	 // -1 ～ 1
				playerPos.z - static_cast<float>((rand() % 6) + 10)	 // -10 ～ -15
			};
		}

		EnemyType type = (rand() % 3 == 0) ? EnemyType::Fast : EnemyType::Normal; // 1/3の確率でFast
		SpawnEnemy(type, spawnPos);
		lastSpawnTime_ = gameTime_;
	}
}

///=============================================================================
///                        敵の生成
void EnemyManager::SpawnEnemy(EnemyType type, const Vector3 &position) {
	auto enemy = std::make_unique<Enemy>();
	enemy->Initialize(object3dSetup_, "jet.obj", position);

	// 移動速度と方向を設定（+Z方向へ直進）
	float speed = (type == EnemyType::Fast) ? 22.0f : 15.0f;
	enemy->SetMovementDirection(speed, {0.0f, 0.0f, 1.0f});

	// パーティクルシステムの設定
	enemy->SetParticleSystem(particle_, particleSetup_);

	enemies_.push_back(std::move(enemy));
}

///=============================================================================
///                        死んだ敵の削除
void EnemyManager::RemoveDeadEnemies() {
	// 削除前に撃破数をカウント
	size_t beforeCount = enemies_.size();

	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
					   [](const std::unique_ptr<Enemy> &enemy) {
						   return !enemy || !enemy->IsAlive();
					   }),
		enemies_.end());

	// 削除された敵の数だけ撃破数を増加
	size_t afterCount = enemies_.size();
	defeatedCount_ += static_cast<int>(beforeCount - afterCount);
}

///=============================================================================
///                        生存敵数の取得
size_t EnemyManager::GetAliveEnemyCount() const {
	return std::count_if(enemies_.begin(), enemies_.end(),
						 [](const std::unique_ptr<Enemy> &enemy) {
							 return enemy && enemy->IsAlive();
						 });
}

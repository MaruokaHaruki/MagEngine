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
	spawnInterval_ = EnemyManagerConstants::kDefaultSpawnInterval;
	maxEnemies_ = EnemyManagerConstants::kDefaultMaxEnemies;
	autoSpawn_ = true;

	//========================================
	// ゲーム進行管理
	defeatedCount_ = 0;
	targetDefeatedCount_ = EnemyManagerConstants::kDefaultTargetDefeatedCount;

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

	// ヒットリアクション中の敵をカウント
	int hitReactingCount = 0;
	int totalHP = 0;
	for (const auto &enemy : enemies_) {
		if (enemy && enemy->IsInHitReaction()) {
			hitReactingCount++;
		}
		if (enemy && enemy->IsAlive()) {
			totalHP += enemy->GetCurrentHP();
		}
	}
	ImGui::Text("Hit Reacting: %d", hitReactingCount);
	ImGui::Text("Total HP: %d", totalHP);

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
		// Destroying状態の敵は当たり判定から除外
		if (enemy && enemy->IsAlive() && !enemy->IsInHitReaction()) {
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

		// プレイヤーの前方にスポーン位置を設定
		Vector3 spawnPos = {0.0f, 0.0f, 30.0f};
		if (player_) {
			Vector3 playerPos = player_->GetPosition();
			spawnPos = {
				playerPos.x + static_cast<float>((rand() % 11) - 5), // -5 ～ 5
				playerPos.y + static_cast<float>((rand() % 3) - 1),	 // -1 ～ 1
				playerPos.z + static_cast<float>((rand() % 11) + 20) // +20 ～ +30
			};
		}

		EnemyType type = (rand() % 3 == 0) ? EnemyType::Fast : EnemyType::Normal;
		SpawnEnemy(type, spawnPos);
		lastSpawnTime_ = gameTime_;
	}
}

///=============================================================================
///                        敵の生成
void EnemyManager::SpawnEnemy(EnemyType type, const Vector3 &position) {
	auto enemy = std::make_unique<Enemy>();
	enemy->Initialize(object3dSetup_, "jet.obj", position);

	// 敵タイプを設定（速度とHPが変わる）
	enemy->SetEnemyType(type);

	// パーティクルシステムの設定
	enemy->SetParticleSystem(particle_, particleSetup_);

	// 撃破時のコールバックを設定（撃破数カウント）
	auto onDefeat = [this]() {
		defeatedCount_++;
	};

	// コールバックを含めてダメージ処理を変更
	// （実際にはEnemyクラス内で保持させる）

	enemies_.push_back(std::move(enemy));
}

///=============================================================================
///                        死んだ敵の削除
void EnemyManager::RemoveDeadEnemies() {
	// Dead状態の敵のみ削除（撃破数は既にカウント済み）
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

/*********************************************************************
 * \file   EnemyManager.cpp
 * \brief  敵の一括管理管理クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 *********************************************************************/
#include "EnemyManager.h"
#include "CollisionManager.h"
#include "Enemy.h" // 具体的なクラスは.cppでインクルード
#include "EnemyBullet.h"
#include "EnemyGunner.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "Player.h"
#include <algorithm>
using namespace MagEngine;

///=============================================================================
///                        初期化
void EnemyManager::Initialize(MagEngine::Object3dSetup *object3dSetup, 
	MagEngine::Particle *particle, 
	MagEngine::ParticleSetup *particleSetup) {
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

	if (ImGui::Button("Spawn Enemy")) {
		SpawnEnemy({0.0f, 0.0f, 30.0f});
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
	// 自動スポーン
	if (autoSpawn_ &&
		GetAliveEnemyCount() < static_cast<size_t>(maxEnemies_) &&
		gameTime_ - lastSpawnTime_ >= spawnInterval_) {

		// カメラ外からスポーン
		Vector3 spawnPos = {0.0f, 0.0f, -30.0f};
		if (player_) {
			Vector3 playerPos = player_->GetPosition();

			// ランダムな方向から出現（左右・上方・斜め）
			int spawnPattern = rand() % 4;
			float distance = 50.0f + static_cast<float>(rand() % 20);

			switch (spawnPattern) {
			case 0: // 左から
				spawnPos = {
					playerPos.x - distance,
					playerPos.y + static_cast<float>((rand() % 10) - 5),
					playerPos.z + static_cast<float>((rand() % 30) - 15)};
				break;
			case 1: // 右から
				spawnPos = {
					playerPos.x + distance,
					playerPos.y + static_cast<float>((rand() % 10) - 5),
					playerPos.z + static_cast<float>((rand() % 30) - 15)};
				break;
			case 2: // 上から
				spawnPos = {
					playerPos.x + static_cast<float>((rand() % 30) - 15),
					playerPos.y + distance * 0.6f,
					playerPos.z + static_cast<float>((rand() % 30) - 15)};
				break;
			case 3: // 斜め前方から
				float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
				spawnPos = {
					playerPos.x + std::sin(angle) * distance,
					playerPos.y + static_cast<float>((rand() % 10) - 5),
					playerPos.z + std::cos(angle) * distance * 0.5f + 40.0f};
				break;
			}
		}

		// 30%の確率でガンナータイプをスポーン
		if (rand() % 100 < 50) {
			SpawnGunner(spawnPos);
		} else {
			SpawnEnemy(spawnPos);
		}
		lastSpawnTime_ = gameTime_;
	}
}

///=============================================================================
///                        敵の生成
void EnemyManager::SpawnEnemy(const Vector3 &position) {
	// 通常の敵を生成
	auto enemy = std::make_unique<Enemy>();
	enemy->Initialize(object3dSetup_, "jet.obj", position);

	// パーティクルシステムの設定
	enemy->SetParticleSystem(particle_, particleSetup_);

	// プレイヤー参照を設定
	enemy->SetPlayer(player_);

	// 撃破時のコールバックを設定（撃破数カウント）
	enemy->SetDefeatCallback([this]() {
		defeatedCount_++;
	});

	// EnemyBase* にアップキャストして格納
	enemies_.push_back(std::move(enemy));
}

///=============================================================================
///                        ガンナータイプの生成
void EnemyManager::SpawnGunner(const Vector3 &position) {
	auto gunner = std::make_unique<EnemyGunner>();
	gunner->Initialize(object3dSetup_, "jet.obj", position);
	gunner->SetParticleSystem(particle_, particleSetup_);
	gunner->SetPlayer(player_);
	gunner->SetDefeatCallback([this]() {
		defeatedCount_++;
	});

	enemies_.push_back(std::move(gunner));
}

///=============================================================================
///                        死んだ敵の削除
void EnemyManager::RemoveDeadEnemies() {
	// Dead状態の敵のみ削除（撃破数は既にカウント済み）
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
					   [](const std::unique_ptr<EnemyBase> &enemy) {
						   return !enemy || !enemy->IsAlive();
					   }),
		enemies_.end());
}

///=============================================================================
///                        生存敵数の取得
size_t EnemyManager::GetAliveEnemyCount() const {
	return std::count_if(enemies_.begin(), enemies_.end(),
						 [](const std::unique_ptr<EnemyBase> &enemy) {
							 return enemy && enemy->IsAlive();
						 });
}

///=============================================================================
///                        敵の弾をすべて取得
std::vector<EnemyBullet *> EnemyManager::GetAllEnemyBullets() {
	std::vector<EnemyBullet *> allBullets;

	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			// EnemyGunnerかどうかチェック
			if (EnemyGunner *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
				auto &bullets = gunner->GetBullets();
				for (auto &bullet : bullets) {
					if (bullet && bullet->IsAlive()) {
						allBullets.push_back(bullet.get());
					}
				}
			}
		}
	}

	return allBullets;
}

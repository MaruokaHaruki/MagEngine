/*********************************************************************
 * \file   EnemyManager.cpp
 * \brief  敵の一括管理クラス（ウェーブシステム管理）
 *
 * \author Harukichimaru
 * \date   January 2025
 *********************************************************************/
#include "EnemyManager.h"
#include "CollisionManager.h"
#include "Enemy.h"
#include "EnemyBullet.h"
#include "EnemyGroup.h"
#include "EnemyGunner.h"
#include "ImguiSetup.h"
#include "Player.h"
#include "ConfigLoader.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

namespace {
	constexpr float kBetweenWaveInterval = 3.0f; // ウェーブ間の待機時間（秒）
}

///=============================================================================
///                        初期化
void EnemyManager::Initialize(MagEngine::Object3dSetup *object3dSetup,
							  MagEngine::Particle *particle,
							  MagEngine::ParticleSetup *particleSetup,
							  MagEngine::TrailEffectManager *trailEffectManager) {
	object3dSetup_ = object3dSetup;
	particle_ = particle;
	particleSetup_ = particleSetup;
	trailEffectManager_ = trailEffectManager;
	player_ = nullptr;

	gameTime_ = 0.0f;
	defeatedCount_ = 0;
	allWavesCompleted_ = false;
	nextGroupId_ = 0;

	//========================================
	// JSON から設定を読み込む
	ConfigLoader::LoadAllConfigs("application/enemy/config/data");

	// JSON から読み込んだウェーブ設定を WaveConfig に変換
	waveConfigs_.clear();
	const auto &jsonWaveConfigs = WaveParamManager::GetAllWaveConfigs();
	for (const auto &jsonWave : jsonWaveConfigs) {
		WaveConfig wc;
		wc.enemyCount = jsonWave.enemy_count;
		wc.gunnerCount = jsonWave.gunner_count;
		wc.spawnInterval = jsonWave.spawn_interval;
		wc.formationRatio = jsonWave.formation_ratio;
		wc.maxGroupSize = jsonWave.max_group_size;
		
		// formation_pattern 文字列をインデックスに変換
		const std::string &formationPatternName = jsonWave.formation_pattern;
		const auto *formationConfig = FormationConfigManager::GetFormationConfig(formationPatternName);
		if (formationConfig) {
			wc.formationPattern = formationConfig->type;
		} else {
			// フォールバック: 見つからなければ V Formation (0) を使用
			wc.formationPattern = 0;
		}
		
		waveConfigs_.push_back(wc);
	}

	// 目標撃破数 = 全ウェーブの総敵数（退却分は上振れするが目安として使用）
	targetDefeatedCount_ = 0;
	for (const auto &w : waveConfigs_) {
		targetDefeatedCount_ += w.enemyCount + w.gunnerCount;
	}

	// ウェーブ状態初期化
	currentWave_ = 0;
	wavePhase_ = WavePhase::Spawning;
	waveTimer_ = 0.0f;
	betweenWaveTimer_ = 0.0f;
	spawnedInWave_ = 0;
}

///=============================================================================
///                        更新
void EnemyManager::Update() {
	const float frameTime = 1.0f / 60.0f;
	gameTime_ += frameTime;

	UpdateWave();

	for (auto &enemy : enemies_) {
		if (enemy)
			enemy->Update();
	}

	// グループ更新（編隊制御）
	// NOTE: 順序重要！RemoveDeadMembers() → Update() の順に変更
	//       敵が削除されたメモリへのアクセスを防ぐため
	if (player_) {
		Vector3 playerPos = player_->GetPosition();
		for (auto &group : groups_) {
			if (group) {
				// 1. 先に死んだメンバを削除（ポインタの無効化を防止）
				group->RemoveDeadMembers();
				// 2. その後に編隊更新を実行
				if (group->IsActive()) {
					group->Update(playerPos);
				}
			}
		}
	}

	RemoveDeadEnemies();
}

///=============================================================================
///                        描画
void EnemyManager::Draw() {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive())
			enemy->Draw();
	}
}

void EnemyManager::DrawTrail() {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			// EnemyGunner の DrawTrail を呼び出す
			if (auto *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
				gunner->DrawTrail();
			}
		}
	}
}

///=============================================================================
///                        ImGui描画
void EnemyManager::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Enemy Manager");

	ImGui::Text("Game Time: %.1f", gameTime_);

	// ウェーブ情報
	const char *phaseNames[] = {"Spawning", "Active", "BetweenWaves"};
	ImGui::Text("Wave: %d / %d  [%s]", currentWave_ + 1, static_cast<int>(waveConfigs_.size()), phaseNames[static_cast<int>(wavePhase_)]);

	if (wavePhase_ == WavePhase::Spawning && currentWave_ < static_cast<int>(waveConfigs_.size())) {
		const auto &wc = waveConfigs_[currentWave_];
		ImGui::Text("Spawned: %d / %d", spawnedInWave_, wc.enemyCount + wc.gunnerCount);
	}
	if (wavePhase_ == WavePhase::BetweenWaves) {
		ImGui::Text("Next wave in: %.1f s", kBetweenWaveInterval - betweenWaveTimer_);
	}

	ImGui::Separator();
	ImGui::Text("Alive Enemies: %zu", GetAliveEnemyCount());
	ImGui::Text("Defeated: %d / %d", defeatedCount_, targetDefeatedCount_);

	int hitReactingCount = 0;
	for (const auto &enemy : enemies_) {
		if (enemy && enemy->IsInHitReaction())
			hitReactingCount++;
	}
	ImGui::Text("Hit Reacting: %d", hitReactingCount);

	if (IsGameClear()) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "GAME CLEAR!");
	}

	ImGui::Separator();
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
		if (enemy && enemy->IsAlive() && !enemy->IsInHitReaction()) {
			collisionManager->RegisterObject(enemy.get());
		}
	}
}

///=============================================================================
///                        全敵削除
void EnemyManager::Clear() {
	enemies_.clear();
	groups_.clear();
	nextGroupId_ = 0;
}

///=============================================================================
///                        ウェーブ進行管理
void EnemyManager::UpdateWave() {
	const float deltaTime = 1.0f / 60.0f;

	// 全ウェーブクリア後は何もしない
	if (IsGameClear())
		return;
	if (currentWave_ >= static_cast<int>(waveConfigs_.size()))
		return;

	const auto &wave = waveConfigs_[currentWave_];
	const int totalInWave = wave.enemyCount + wave.gunnerCount;

	switch (wavePhase_) {
	case WavePhase::Spawning: {
		waveTimer_ += deltaTime;

		if (spawnedInWave_ < totalInWave && waveTimer_ >= wave.spawnInterval) {
			// ガンナーを後半にスポーン（先に通常エネミー）
			if (spawnedInWave_ < wave.enemyCount) {
				SpawnEnemy(GenerateSpawnPosition());
			} else {
				SpawnGunner(GenerateSpawnPosition());
			}
			spawnedInWave_++;
			waveTimer_ = 0.0f;
		}

		// 全スポーン完了 → Activeへ
		if (spawnedInWave_ >= totalInWave) {
			wavePhase_ = WavePhase::Active;
		}
		break;
	}

	case WavePhase::Active: {
		// 全敵が消滅したら次ウェーブへ
		if (GetAliveEnemyCount() == 0) {
			wavePhase_ = WavePhase::BetweenWaves;
			betweenWaveTimer_ = 0.0f;
		}
		break;
	}

	case WavePhase::BetweenWaves: {
		betweenWaveTimer_ += deltaTime;
		if (betweenWaveTimer_ >= kBetweenWaveInterval) {
			currentWave_++;
			if (currentWave_ < static_cast<int>(waveConfigs_.size())) {
				wavePhase_ = WavePhase::Spawning;
				// 新ウェーブの spawnInterval を waveTimer に設定して最初の敵を即スポーン
				waveTimer_ = waveConfigs_[currentWave_].spawnInterval;
				spawnedInWave_ = 0;
			} else {
				// 全ウェーブ完了
				allWavesCompleted_ = true;
			}
		}
		break;
	}
	}
}

///=============================================================================
///                        敵の生成
void EnemyManager::SpawnEnemy(const Vector3 &position) {
	auto enemy = std::make_unique<Enemy>();
	enemy->Initialize(object3dSetup_, "jet.obj", position);
	enemy->SetParticleSystem(particle_, particleSetup_);
	enemy->SetPlayer(player_);
	enemy->SetDefeatCallback([this]() {
		defeatedCount_++;
	});

	// 現在のウェーブ設定を取得
	const WaveConfig &currentWaveConfig = waveConfigs_[currentWave_];

	// 編隊比率に基づいて編隊敵か単独敵かを決定
	bool shouldFormation = (rand() % 100) < (currentWaveConfig.formationRatio * 100.0f);

	if (shouldFormation) {
		bool addedToExistingGroup = false;

		// 既存のアクティブなグループにメンバを追加試行
		if ((rand() % 100) < 60) { // 60%の確率で既存グループに追加
			for (auto &group : groups_) {
				if (group && group->IsActive()) {
					int memberCount = static_cast<int>(group->GetMembers().size());
					if (memberCount < currentWaveConfig.maxGroupSize) {
						group->AddMember(enemy.get(), memberCount);
						enemy->SetGroupId(group->GetGroupId());
						enemy->SetFormationFollowing(true);
						addedToExistingGroup = true;
						break;
					}
				}
			}
		}

		// 新規グループを作成（既存に追加されなかった場合）
		if (!addedToExistingGroup) {
			auto group = std::make_unique<EnemyGroup>();
			group->SetGroupId(nextGroupId_);

			// Wave設定のパターンに基づいてフォーメーションを選択
			FormationType formationType = static_cast<FormationType>(currentWaveConfig.formationPattern);
			group->Initialize(enemy.get(), formationType);

			groups_.push_back(std::move(group));
			enemy->SetGroupId(nextGroupId_);
			nextGroupId_++;
		}
	}

	enemies_.push_back(std::move(enemy));
}

///=============================================================================
///                        ガンナータイプの生成
void EnemyManager::SpawnGunner(const Vector3 &position) {
	auto gunner = std::make_unique<EnemyGunner>();
	gunner->Initialize(object3dSetup_, "jet.obj", position);
	gunner->SetParticleSystem(particle_, particleSetup_);
	gunner->SetTrailEffectManager(trailEffectManager_);
	gunner->SetPlayer(player_);
	gunner->SetDefeatCallback([this]() {
		defeatedCount_++;
	});

	// 現在のウェーブ設定を取得
	const WaveConfig &currentWaveConfig = waveConfigs_[currentWave_];

	// 編隊比率に基づいて編隊敵か単独敵かを決定
	bool shouldFormation = (rand() % 100) < (currentWaveConfig.formationRatio * 100.0f);

	if (shouldFormation) {
		bool addedToExistingGroup = false;

		// 既存のアクティブなグループにメンバを追加試行
		if ((rand() % 100) < 60) { // 60%の確率で既存グループに追加
			for (auto &group : groups_) {
				if (group && group->IsActive()) {
					int memberCount = static_cast<int>(group->GetMembers().size());
					if (memberCount < currentWaveConfig.maxGroupSize) {
						group->AddMember(gunner.get(), memberCount);
						gunner->SetGroupId(group->GetGroupId());
						gunner->SetFormationFollowing(true);
						addedToExistingGroup = true;
						break;
					}
				}
			}
		}

		// 新規グループを作成（既存に追加されなかった場合）
		if (!addedToExistingGroup) {
			auto group = std::make_unique<EnemyGroup>();
			group->SetGroupId(nextGroupId_);

			// Wave設定のパターンに基づいてフォーメーションを選択
			FormationType formationType = static_cast<FormationType>(currentWaveConfig.formationPattern);
			group->Initialize(gunner.get(), formationType);

			groups_.push_back(std::move(group));
			gunner->SetGroupId(nextGroupId_);
			nextGroupId_++;
		}
	}

	enemies_.push_back(std::move(gunner));
}

///=============================================================================
///                        スポーン位置の生成
Vector3 EnemyManager::GenerateSpawnPosition() const {
	Vector3 spawnPos = {0.0f, 0.0f, -30.0f};
	if (!player_)
		return spawnPos;

	Vector3 playerPos = player_->GetPosition();
	int pattern = rand() % 4;
	float distance = 50.0f + static_cast<float>(rand() % 20);

	switch (pattern) {
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
	default: { // 斜め前方から
		float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
		spawnPos = {
			playerPos.x + std::sin(angle) * distance,
			playerPos.y + static_cast<float>((rand() % 10) - 5),
			playerPos.z + std::cos(angle) * distance * 0.5f + 40.0f};
		break;
	}
	}
	return spawnPos;
}

///=============================================================================
///                        死んだ敵の削除
void EnemyManager::RemoveDeadEnemies() {
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
			if (EnemyGunner *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
				for (auto &bullet : gunner->GetBullets()) {
					if (bullet && bullet->IsAlive())
						allBullets.push_back(bullet.get());
				}
			}
		}
	}
	return allBullets;
}

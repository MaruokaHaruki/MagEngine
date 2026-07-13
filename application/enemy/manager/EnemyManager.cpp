#define NOMINMAX
#include "EnemyManager.h"
#include "CollisionManager.h"
#include "Enemy.h"
#include "EnemyBullet.h"
#include "EnemyGunner.h"
#include "ImguiSetup.h"
#include <algorithm>

using namespace MagEngine;

std::unique_ptr<EnemyBase> EnemyFactory::Create(const EnemySpawnDefinition &definition, const EnemySpawnContext &context) {
	if (!context.object3dSetup) {
		return nullptr;
	}

	std::unique_ptr<EnemyBase> enemy;
	switch (definition.archetype) {
	case EnemyArchetype::Gunner: {
		auto gunner = std::make_unique<EnemyGunner>();
		gunner->Initialize(context.object3dSetup, context.modelName, context.position);
		gunner->SetTrailEffectManager(context.trailEffectManager);
		enemy = std::move(gunner);
		break;
	}
	case EnemyArchetype::Standard:
	default: {
		auto standard = std::make_unique<Enemy>();
		standard->Initialize(context.object3dSetup, context.modelName, context.position);
		enemy = std::move(standard);
		break;
	}
	}

	if (!enemy) {
		return nullptr;
	}

	enemy->SetParticleSystem(context.particle, context.particleSetup);
	enemy->SetPlayer(context.player);
	return enemy;
}

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
	enemies_.clear();
}

void EnemyManager::Update() {
	Update(1.0f / 60.0f);
}

void EnemyManager::Update(float deltaTime) {
	const float safeDeltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
	gameTime_ += safeDeltaTime;

	for (auto &enemy : enemies_) {
		if (enemy) {
			enemy->Update(safeDeltaTime);
		}
	}

	RemoveInactiveEnemies();
}

void EnemyManager::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			enemy->RegisterRenderables(renderWorld);
		}
	}
}

void EnemyManager::DrawImGui() {
#ifdef _DEBUG
	ImGui::Text("Game Time: %.1f", gameTime_);
	ImGui::Text("Active Enemies: %zu", GetActiveEnemyCount());
	ImGui::Text("Defeated: %d", defeatedCount_);
	if (ImGui::Button("Clear All Enemies")) {
		Clear();
	}
#endif
}

void EnemyManager::RegisterCollisions(CollisionManager *collisionManager) {
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive() && !enemy->IsInHitReaction()) {
			collisionManager->RegisterObject(enemy.get());
		}
	}
}

void EnemyManager::Clear() {
	enemies_.clear();
	defeatedCount_ = 0;
	gameTime_ = 0.0f;
}

EnemyBase *EnemyManager::CreateEnemy(const EnemySpawnDefinition &definition, const Vector3 &position, const std::string &modelName) {
	EnemySpawnContext context{};
	context.object3dSetup = object3dSetup_;
	context.particle = particle_;
	context.particleSetup = particleSetup_;
	context.trailEffectManager = trailEffectManager_;
	context.player = player_;
	context.position = position;
	context.modelName = modelName;

	auto enemy = enemyFactory_.Create(definition, context);
	if (!enemy) {
		return nullptr;
	}

	EnemyBase *rawEnemy = enemy.get();
	rawEnemy->SetDefeatCallback([this]() {
		++defeatedCount_;
	});
	enemies_.push_back(std::move(enemy));
	return rawEnemy;
}

size_t EnemyManager::GetActiveEnemyCount() const {
	return std::count_if(enemies_.begin(), enemies_.end(),
						 [](const std::unique_ptr<EnemyBase> &enemy) {
							 return enemy && enemy->IsAlive();
						 });
}

std::vector<EnemyBullet *> EnemyManager::GetAllEnemyBullets() {
	std::vector<EnemyBullet *> allBullets;
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			if (EnemyGunner *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
				for (auto &bullet : gunner->GetBullets()) {
					if (bullet && bullet->IsAlive()) {
						allBullets.push_back(bullet.get());
					}
				}
			}
		}
	}
	return allBullets;
}

void EnemyManager::RemoveInactiveEnemies() {
	enemies_.erase(
		std::remove_if(enemies_.begin(), enemies_.end(),
					   [](const std::unique_ptr<EnemyBase> &enemy) {
						   return !enemy || !enemy->IsAlive();
					   }),
		enemies_.end());
}

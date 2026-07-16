#define NOMINMAX
#include "EnemyManager.h"
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
	case EnemyArchetype::Standard: {
		auto standard = std::make_unique<Enemy>();
		standard->Initialize(context.object3dSetup, context.modelName, context.position);
		enemy = std::move(standard);
		break;
	}
	default:
		return nullptr;
	}

	if (!enemy) {
		return nullptr;
	}

	enemy->SetParticleSystem(context.particle, context.particleSetup);
	enemy->SetPlayer(context.player);
	enemy->ApplySpawnModifiers(definition.healthMultiplier, definition.speedMultiplier);
	if (EnemyGunner *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
		gunner->SetShootInterval(EnemyGunnerConstants::kShootInterval + definition.shotDelayOffset);
	}
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
	enemyLookup_.clear();
	enemies_.clear();
}

void EnemyManager::Update(float deltaTime) {
	gameTime_ += deltaTime;

	for (auto &enemy : enemies_) {
		if (enemy) {
			enemy->Update(deltaTime);
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

void EnemyManager::Clear() {
	// 処理内容：Enemy の実体を破棄する前に Handle の解決登録を解除する。
	// 理由：外部に残った Handle から解放済みメモリへ到達できないようにするため。
	enemyLookup_.clear();
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
	if (nextEnemyHandleValue_ == 0) {
		return nullptr;
	}

	const EnemyHandle handle{nextEnemyHandleValue_++};
	enemy->SetHandle(handle);

	EnemyBase *rawEnemy = enemy.get();
	rawEnemy->SetDefeatCallback([this]() {
		++defeatedCount_;
	});
	enemies_.push_back(std::move(enemy));
	enemyLookup_.emplace(handle.value, rawEnemy);
	return rawEnemy;
}

EnemyBase *EnemyManager::ResolveEnemy(EnemyHandle handle) {
	const auto it = enemyLookup_.find(handle.value);
	return it != enemyLookup_.end() ? it->second : nullptr;
}

const EnemyBase *EnemyManager::ResolveEnemy(EnemyHandle handle) const {
	const auto it = enemyLookup_.find(handle.value);
	return it != enemyLookup_.end() ? it->second : nullptr;
}

bool EnemyManager::IsEnemyValid(EnemyHandle handle) const {
	return ResolveEnemy(handle) != nullptr;
}

bool EnemyManager::IsEnemyTargetable(EnemyHandle handle) const {
	const EnemyBase *enemy = ResolveEnemy(handle);
	return enemy && enemy->IsAlive() && enemy->IsCollisionEnabled();
}

size_t EnemyManager::GetActiveEnemyCount() const {
	return std::count_if(enemies_.begin(), enemies_.end(),
						 [](const std::unique_ptr<EnemyBase> &enemy) {
							 return enemy && enemy->IsAlive();
						 });
}

void EnemyManager::CollectEnemyBullets(std::vector<EnemyBullet *> &result) const {
	// 呼び出し元のバッファを再利用し、衝突登録のたびにvectorを確保しない。
	result.clear();
	for (auto &enemy : enemies_) {
		if (enemy && enemy->IsAlive()) {
			if (EnemyGunner *gunner = dynamic_cast<EnemyGunner *>(enemy.get())) {
				for (auto &bullet : gunner->GetBullets()) {
				if (bullet && bullet->IsAlive()) {
					result.push_back(bullet.get());
					}
				}
			}
		}
	}
}

void EnemyManager::RemoveInactiveEnemies() {
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		if (!*it || !(*it)->IsAlive()) {
			if (*it) {
				// Handle の無効化を実体破棄より先に行う。
				enemyLookup_.erase((*it)->GetHandle().value);
			}
			it = enemies_.erase(it);
			continue;
		}
		++it;
	}
}

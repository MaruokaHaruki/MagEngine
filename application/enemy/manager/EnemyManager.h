#pragma once
#include "EnemyBase.h"
#include "stage/EnemyStageSystem.h"
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
#include <memory>
#include <vector>


class CollisionManager;
class EnemyBullet;
class Player;
namespace MagEngine {
	class Object3dSetup;
	class Particle;
	class ParticleSetup;
	class RenderWorld;
	class TrailEffectManager;
}

struct EnemySpawnContext {
	MagEngine::Object3dSetup *object3dSetup = nullptr;
	MagEngine::Particle *particle = nullptr;
	MagEngine::ParticleSetup *particleSetup = nullptr;
	MagEngine::TrailEffectManager *trailEffectManager = nullptr;
	Player *player = nullptr;
	Vector3 position{};
	std::string modelName = "jet.obj";
};

class EnemyFactory {
public:
	std::unique_ptr<EnemyBase> Create(const EnemySpawnDefinition &definition, const EnemySpawnContext &context);
};

class EnemyManager {
public:
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::Particle *particle,
					MagEngine::ParticleSetup *particleSetup,
					MagEngine::TrailEffectManager *trailEffectManager);

	void SetPlayer(Player *player) {
		player_ = player;
	}

	void Update(float deltaTime);
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);
	void DrawImGui();
	void RegisterCollisions(CollisionManager *collisionManager);
	void Clear();

	EnemyBase *CreateEnemy(const EnemySpawnDefinition &definition, const Vector3 &position, const std::string &modelName = "jet.obj");

	size_t GetActiveEnemyCount() const;
	size_t GetAliveEnemyCount() const {
		return GetActiveEnemyCount();
	}

	int GetDefeatedCount() const {
		return defeatedCount_;
	}

	const std::vector<std::unique_ptr<EnemyBase>> &GetEnemies() const {
		return enemies_;
	}

	void CollectEnemyBullets(std::vector<EnemyBullet *> &result) const;

private:
	void RemoveInactiveEnemies();

	std::vector<std::unique_ptr<EnemyBase>> enemies_;
	EnemyFactory enemyFactory_;
	int defeatedCount_ = 0;
	float gameTime_ = 0.0f;

	MagEngine::Object3dSetup *object3dSetup_ = nullptr;
	MagEngine::Particle *particle_ = nullptr;
	MagEngine::ParticleSetup *particleSetup_ = nullptr;
	MagEngine::TrailEffectManager *trailEffectManager_ = nullptr;
	Player *player_ = nullptr;
};

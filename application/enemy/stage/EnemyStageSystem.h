#pragma once
#include "EnemyGroup.h"
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
#include "externals/json.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>


class EnemyBase;
class EnemyManager;
class Player;

enum class EnemyArchetype {
	Standard,
	Gunner,
};

struct EnemySpawnDefinition {
	EnemyArchetype archetype = EnemyArchetype::Standard;
	uint32_t count = 1;
	float healthMultiplier = 1.0f;
	float speedMultiplier = 1.0f;
	float shotDelayOffset = 0.0f;
};

struct EnemyFormationMotionDefinition {
	float entryDuration = EnemyFormationConstants::kEntryDuration;
	float combatDuration = EnemyFormationConstants::kCombatDuration;
	float exitDuration = EnemyFormationConstants::kExitDuration;
	float combatDistance = EnemyFormationConstants::kCombatForwardDistance;
	float combatAreaHalfWidth = EnemyFormationConstants::kCombatAreaHalfWidth;
	float combatAreaHalfHeight = EnemyFormationConstants::kCombatAreaHalfHeight;
	float formationSpacing = EnemyFormationConstants::kFormationSpacing;
	float orbitRadiusX = EnemyFormationConstants::kOrbitRadiusX;
	float orbitRadiusY = EnemyFormationConstants::kOrbitRadiusY;
	float orbitAngularSpeed = EnemyFormationConstants::kOrbitAngularSpeed;
	float attackInterval = EnemyFormationConstants::kAttackInterval;
	float attackSlotDelay = EnemyFormationConstants::kAttackSlotDelay;
	float offscreenMarginX = EnemyFormationConstants::kOffscreenMarginX;
	float offscreenMarginY = EnemyFormationConstants::kOffscreenMarginY;
	float minimumSpawnDistance = EnemyFormationConstants::kMinimumSpawnDistance;
};

struct SpawnGroupDefinition {
	std::string groupId;
	EnemyFormationPattern formationPattern = EnemyFormationPattern::HorizontalLine;
	EnemyGroupAttackPattern attackPattern = EnemyGroupAttackPattern::Staggered;
	std::vector<EnemySpawnDefinition> members;
	float spawnDelaySeconds = 0.0f;
	EnemyFormationMotionDefinition motion;
};

struct WaveDefinition {
	std::string waveId;
	float startDelaySeconds = 0.0f;
	float nextWaveDelaySeconds = 2.0f;
	std::vector<SpawnGroupDefinition> spawnGroups;
	bool waitForAllGroupsFinished = true;
	bool waitForAllEnemiesRemoved = true;
};

struct StageDefinition {
	std::string stageId = "stage_01";
	std::vector<WaveDefinition> waves;
	float clearDelaySeconds = 2.0f;
	bool clearWhenAllWavesCompleted = true;
};

struct StageLoadResult {
	StageDefinition stageDefinition{};
	std::string sourcePath;
	std::string errorMessage;

	[[nodiscard]] bool IsSuccess() const {
		return errorMessage.empty();
	}
};

enum class WaveState {
	Waiting,
	Spawning,
	Active,
	Completed,
	Failed,
};

enum class GameFlowState {
	Intro,
	Playing,
	ClearPending,
	Cleared,
	GameOver,
};

class StageDefinitionLoader {
public:
	static StageLoadResult Load(const std::string &path);
	static bool Validate(const StageDefinition &stageDefinition, const std::string &sourcePath, std::string &errorMessage);

private:
	static bool Parse(const nlohmann::json &jsonData, StageDefinition &stageDefinition, std::string &errorMessage);
};

class WaveController {
public:
	void Initialize(const StageDefinition &stageDefinition) {
		stageDefinition_ = stageDefinition;
		currentWaveIndex_ = 0;
		state_ = stageDefinition_.waves.empty() ? WaveState::Completed : WaveState::Waiting;
		waveTimer_ = 0.0f;
		requiresEnemyRemovalForStageCompletion_ = true;
		groups_.clear();
		spawnedGroupFlags_.clear();
		ResetSpawnFlags();
	}

	void Update(float deltaTime, EnemyManager &enemyManager, const Vector3 &playerPosition);
	void Clear();

	bool IsStageCompleted() const {
		return state_ == WaveState::Completed && currentWaveIndex_ >= stageDefinition_.waves.size();
	}

	WaveState GetState() const {
		return state_;
	}

	size_t GetCurrentWaveIndex() const {
		return currentWaveIndex_;
	}

	const std::string &GetCurrentWaveId() const {
		static const std::string emptyId;
		if (currentWaveIndex_ >= stageDefinition_.waves.size()) {
			return emptyId;
		}
		return stageDefinition_.waves[currentWaveIndex_].waveId;
	}

	size_t GetTotalWaveCount() const {
		return stageDefinition_.waves.size();
	}

	size_t GetPendingSpawnGroupCount() const;
	size_t GetActiveGroupCount() const;
	const std::vector<std::unique_ptr<EnemyGroup>> &GetGroups() const {
		return groups_;
	}

	bool RequiresEnemyRemovalForStageCompletion() const {
		return requiresEnemyRemovalForStageCompletion_;
	}

private:
	void ResetSpawnFlags();
	void SpawnGroup(const SpawnGroupDefinition &definition, EnemyManager &enemyManager, const Vector3 &playerPosition);
	bool AreCurrentWaveGroupsSpawned() const;
	bool AreGroupsFinished() const;
	void AdvanceWaveOrComplete();

	StageDefinition stageDefinition_{};
	size_t currentWaveIndex_ = 0;
	WaveState state_ = WaveState::Waiting;
	float waveTimer_ = 0.0f;
	std::vector<bool> spawnedGroupFlags_;
	std::vector<std::unique_ptr<EnemyGroup>> groups_;
	int nextGroupId_ = 0;
	bool requiresEnemyRemovalForStageCompletion_ = true;
};

class GameFlowController {
public:
	void Initialize(const StageDefinition &stageDefinition) {
		stageDefinition_ = stageDefinition;
		waveController_.Initialize(stageDefinition_);
		state_ = GameFlowState::Playing;
		clearDelayTimer_ = 0.0f;
	}

	void Update(float deltaTime, EnemyManager &enemyManager, Player *player);
	void Clear();

	GameFlowState GetState() const {
		return state_;
	}

	bool IsCleared() const {
		return state_ == GameFlowState::Cleared;
	}

	bool IsGameOver() const {
		return state_ == GameFlowState::GameOver;
	}

	float GetClearDelayTimer() const {
		return clearDelayTimer_;
	}

	const WaveController &GetWaveController() const {
		return waveController_;
	}

	WaveController &GetWaveController() {
		return waveController_;
	}

	const StageDefinition &GetStageDefinition() const {
		return stageDefinition_;
	}

private:
	StageDefinition stageDefinition_{};
	WaveController waveController_{};
	GameFlowState state_ = GameFlowState::Intro;
	float clearDelayTimer_ = 0.0f;
};

const char *ToString(WaveState state);
const char *ToString(GameFlowState state);
const char *ToString(EnemyGroupFinishReason reason);

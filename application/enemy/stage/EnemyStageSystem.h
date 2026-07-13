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
	static StageDefinition LoadOrDefault(const std::string &path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			return MakeDefaultStage();
		}

		try {
			nlohmann::json jsonData;
			file >> jsonData;
			return Parse(jsonData);
		} catch (...) {
			return MakeDefaultStage();
		}
	}

	static StageDefinition MakeDefaultStage() {
		StageDefinition stage{};
		stage.stageId = "stage_01";
		stage.clearDelaySeconds = 2.0f;

		WaveDefinition wave1{};
		wave1.waveId = "wave_01";
		wave1.startDelaySeconds = 1.5f;
		wave1.nextWaveDelaySeconds = 1.5f;
		wave1.spawnGroups.push_back(MakeGroup("line_entry_01", EnemyFormationPattern::HorizontalLine, 0.0f, 2, 1));
		wave1.spawnGroups.push_back(MakeGroup("v_entry_01", EnemyFormationPattern::VShape, 4.0f, 4, 1));

		WaveDefinition wave2{};
		wave2.waveId = "wave_02";
		wave2.startDelaySeconds = 1.5f;
		wave2.nextWaveDelaySeconds = 0.0f;
		wave2.spawnGroups.push_back(MakeGroup("circle_entry_01", EnemyFormationPattern::Circle, 0.0f, 2, 1));
		wave2.spawnGroups.push_back(MakeGroup("eight_entry_01", EnemyFormationPattern::FigureEight, 4.5f, 2, 1));

		stage.waves.push_back(wave1);
		stage.waves.push_back(wave2);
		return stage;
	}

private:
	static SpawnGroupDefinition MakeGroup(const std::string &id, EnemyFormationPattern pattern, float delay, uint32_t standardCount, uint32_t gunnerCount) {
		SpawnGroupDefinition group{};
		group.groupId = id;
		group.formationPattern = pattern;
		group.attackPattern = EnemyGroupAttackPattern::Staggered;
		group.spawnDelaySeconds = delay;
		group.members.push_back({EnemyArchetype::Standard, standardCount});
		group.members.push_back({EnemyArchetype::Gunner, gunnerCount});
		return group;
	}

	static StageDefinition Parse(const nlohmann::json &jsonData);
};

class WaveController {
public:
	void Initialize(const StageDefinition &stageDefinition) {
		stageDefinition_ = stageDefinition;
		currentWaveIndex_ = 0;
		state_ = stageDefinition_.waves.empty() ? WaveState::Completed : WaveState::Waiting;
		waveTimer_ = 0.0f;
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

private:
	StageDefinition stageDefinition_{};
	WaveController waveController_{};
	GameFlowState state_ = GameFlowState::Intro;
	float clearDelayTimer_ = 0.0f;
};

const char *ToString(WaveState state);
const char *ToString(GameFlowState state);
const char *ToString(EnemyGroupFinishReason reason);

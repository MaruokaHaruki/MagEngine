#define NOMINMAX
#include "stage/EnemyStageSystem.h"
#include "EnemyManager.h"
#include "Player.h"
#include <cmath>

namespace {
	EnemyFormationPattern ParseFormationPattern(const std::string &value) {
		if (value == "HorizontalLine") return EnemyFormationPattern::HorizontalLine;
		if (value == "VShape") return EnemyFormationPattern::VShape;
		if (value == "Circle") return EnemyFormationPattern::Circle;
		if (value == "FigureEight") return EnemyFormationPattern::FigureEight;
		if (value == "Column") return EnemyFormationPattern::Column;
		return EnemyFormationPattern::HorizontalLine;
	}

	EnemyGroupAttackPattern ParseAttackPattern(const std::string &value) {
		if (value == "None") return EnemyGroupAttackPattern::None;
		if (value == "LeaderThenWing") return EnemyGroupAttackPattern::LeaderThenWing;
		if (value == "Alternating") return EnemyGroupAttackPattern::Alternating;
		return EnemyGroupAttackPattern::Staggered;
	}

	EnemyArchetype ParseArchetype(const std::string &value) {
		if (value == "Gunner") return EnemyArchetype::Gunner;
		return EnemyArchetype::Standard;
	}

	Vector3 CalculateEntryPosition(const Vector3 &playerPosition, size_t groupIndex, const EnemyFormationMotionDefinition &motion) {
		const int side = static_cast<int>(groupIndex % 4);
		Vector3 offset{};
		switch (side) {
		case 0:
			offset = {-motion.offscreenMarginX, motion.offscreenMarginY * 0.35f, motion.combatDistance - 20.0f};
			break;
		case 1:
			offset = {motion.offscreenMarginX, -motion.offscreenMarginY * 0.25f, motion.combatDistance - 12.0f};
			break;
		case 2:
			offset = {-motion.offscreenMarginX * 0.45f, motion.offscreenMarginY, motion.combatDistance - 16.0f};
			break;
		default:
			offset = {motion.offscreenMarginX * 0.45f, -motion.offscreenMarginY, motion.combatDistance - 16.0f};
			break;
		}

		Vector3 entry = {playerPosition.x + offset.x, playerPosition.y + offset.y, playerPosition.z + offset.z};
		const Vector3 combat = {playerPosition.x, playerPosition.y, playerPosition.z + motion.combatDistance};
		const Vector3 diff = {entry.x - combat.x, entry.y - combat.y, entry.z - combat.z};
		const float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
		if (distance > 0.001f && distance < motion.minimumSpawnDistance) {
			const float scale = motion.minimumSpawnDistance / distance;
			entry = {combat.x + diff.x * scale, combat.y + diff.y * scale, combat.z + diff.z * scale};
		}
		return entry;
	}
}

StageDefinition StageDefinitionLoader::Parse(const nlohmann::json &jsonData) {
	StageDefinition stage{};
	stage.stageId = jsonData.value("stageId", "stage_01");
	stage.clearDelaySeconds = jsonData.value("clearDelaySeconds", 2.0f);
	stage.clearWhenAllWavesCompleted = jsonData.value("clearWhenAllWavesCompleted", true);

	for (const auto &waveJson : jsonData.value("waves", nlohmann::json::array())) {
		WaveDefinition wave{};
		wave.waveId = waveJson.value("waveId", "");
		wave.startDelaySeconds = waveJson.value("startDelaySeconds", 0.0f);
		wave.nextWaveDelaySeconds = waveJson.value("nextWaveDelaySeconds", 2.0f);
		wave.waitForAllGroupsFinished = waveJson.value("waitForAllGroupsFinished", true);
		wave.waitForAllEnemiesRemoved = waveJson.value("waitForAllEnemiesRemoved", true);

		for (const auto &groupJson : waveJson.value("spawnGroups", nlohmann::json::array())) {
			SpawnGroupDefinition group{};
			group.groupId = groupJson.value("groupId", "");
			group.spawnDelaySeconds = groupJson.value("spawnDelaySeconds", 0.0f);
			group.formationPattern = ParseFormationPattern(groupJson.value("formationPattern", "HorizontalLine"));
			group.attackPattern = ParseAttackPattern(groupJson.value("attackPattern", "Staggered"));

			for (const auto &memberJson : groupJson.value("members", nlohmann::json::array())) {
				EnemySpawnDefinition member{};
				member.archetype = ParseArchetype(memberJson.value("archetype", "Standard"));
				member.count = std::max(1u, memberJson.value("count", 1u));
				member.healthMultiplier = memberJson.value("healthMultiplier", 1.0f);
				member.speedMultiplier = memberJson.value("speedMultiplier", 1.0f);
				member.shotDelayOffset = memberJson.value("shotDelayOffset", 0.0f);
				group.members.push_back(member);
			}

			if (groupJson.contains("motion")) {
				const auto &motionJson = groupJson["motion"];
				group.motion.entryDuration = motionJson.value("entryDuration", group.motion.entryDuration);
				group.motion.combatDuration = motionJson.value("combatDuration", group.motion.combatDuration);
				group.motion.exitDuration = motionJson.value("exitDuration", group.motion.exitDuration);
				group.motion.combatDistance = motionJson.value("combatDistance", group.motion.combatDistance);
				group.motion.combatAreaHalfWidth = motionJson.value("combatAreaHalfWidth", group.motion.combatAreaHalfWidth);
				group.motion.combatAreaHalfHeight = motionJson.value("combatAreaHalfHeight", group.motion.combatAreaHalfHeight);
				group.motion.formationSpacing = motionJson.value("formationSpacing", group.motion.formationSpacing);
				group.motion.orbitRadiusX = motionJson.value("orbitRadiusX", group.motion.orbitRadiusX);
				group.motion.orbitRadiusY = motionJson.value("orbitRadiusY", group.motion.orbitRadiusY);
				group.motion.orbitAngularSpeed = motionJson.value("orbitAngularSpeed", group.motion.orbitAngularSpeed);
				group.motion.attackInterval = motionJson.value("attackInterval", group.motion.attackInterval);
				group.motion.attackSlotDelay = motionJson.value("attackSlotDelay", group.motion.attackSlotDelay);
				group.motion.offscreenMarginX = motionJson.value("offscreenMarginX", group.motion.offscreenMarginX);
				group.motion.offscreenMarginY = motionJson.value("offscreenMarginY", group.motion.offscreenMarginY);
				group.motion.minimumSpawnDistance = motionJson.value("minimumSpawnDistance", group.motion.minimumSpawnDistance);
			}

			if (group.members.empty()) {
				group.members.push_back({EnemyArchetype::Standard, 2});
				group.members.push_back({EnemyArchetype::Gunner, 1});
			}
			wave.spawnGroups.push_back(group);
		}
		stage.waves.push_back(wave);
	}

	return stage.waves.empty() ? MakeDefaultStage() : stage;
}

void WaveController::Update(float deltaTime, EnemyManager &enemyManager, const Vector3 &playerPosition) {
	const float safeDeltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
	if (state_ == WaveState::Completed || state_ == WaveState::Failed) {
		return;
	}
	if (currentWaveIndex_ >= stageDefinition_.waves.size()) {
		state_ = WaveState::Completed;
		return;
	}

	waveTimer_ += safeDeltaTime;
	const WaveDefinition &wave = stageDefinition_.waves[currentWaveIndex_];
	if (state_ == WaveState::Waiting && waveTimer_ >= wave.startDelaySeconds) {
		state_ = WaveState::Spawning;
	}

	if (state_ == WaveState::Spawning || state_ == WaveState::Active) {
		for (size_t i = 0; i < wave.spawnGroups.size(); ++i) {
			if (!spawnedGroupFlags_[i] && waveTimer_ >= wave.startDelaySeconds + wave.spawnGroups[i].spawnDelaySeconds) {
				SpawnGroup(wave.spawnGroups[i], enemyManager, playerPosition);
				spawnedGroupFlags_[i] = true;
			}
		}
		state_ = AreCurrentWaveGroupsSpawned() ? WaveState::Active : WaveState::Spawning;
	}

	for (auto &group : groups_) {
		if (group && !group->IsFinished()) {
			group->Update(safeDeltaTime, playerPosition);
		}
	}

	if (AreCurrentWaveGroupsSpawned() && AreGroupsFinished() && enemyManager.GetActiveEnemyCount() == 0) {
		AdvanceWaveOrComplete();
	}
}

void WaveController::Clear() {
	groups_.clear();
	spawnedGroupFlags_.clear();
	currentWaveIndex_ = 0;
	state_ = WaveState::Completed;
	waveTimer_ = 0.0f;
	nextGroupId_ = 0;
}

size_t WaveController::GetPendingSpawnGroupCount() const {
	return static_cast<size_t>(std::count(spawnedGroupFlags_.begin(), spawnedGroupFlags_.end(), false));
}

size_t WaveController::GetActiveGroupCount() const {
	return static_cast<size_t>(std::count_if(groups_.begin(), groups_.end(), [](const std::unique_ptr<EnemyGroup> &group) {
		return group && !group->IsFinished();
	}));
}

void WaveController::ResetSpawnFlags() {
	spawnedGroupFlags_.clear();
	if (currentWaveIndex_ < stageDefinition_.waves.size()) {
		spawnedGroupFlags_.resize(stageDefinition_.waves[currentWaveIndex_].spawnGroups.size(), false);
	}
}

void WaveController::SpawnGroup(const SpawnGroupDefinition &definition, EnemyManager &enemyManager, const Vector3 &playerPosition) {
	const Vector3 entryPosition = CalculateEntryPosition(playerPosition, static_cast<size_t>(nextGroupId_), definition.motion);
	std::vector<EnemyBase *> members;
	for (const EnemySpawnDefinition &memberDefinition : definition.members) {
		for (uint32_t i = 0; i < memberDefinition.count; ++i) {
			const float slot = static_cast<float>(members.size());
			const Vector3 spawnPosition = {entryPosition.x + (slot - 1.0f) * 6.0f, entryPosition.y, entryPosition.z - slot * 3.0f};
			if (EnemyBase *enemy = enemyManager.CreateEnemy(memberDefinition, spawnPosition)) {
				members.push_back(enemy);
			}
		}
	}
	if (members.empty()) {
		return;
	}

	EnemyGroupMotion motion{};
	motion.entryPosition = entryPosition;
	motion.combatCenter = {playerPosition.x, playerPosition.y, playerPosition.z + definition.motion.combatDistance};
	motion.exitPosition = {playerPosition.x - entryPosition.x >= 0.0f ? playerPosition.x + definition.motion.offscreenMarginX : playerPosition.x - definition.motion.offscreenMarginX,
						   playerPosition.y,
						   playerPosition.z + definition.motion.combatDistance + 45.0f};
	motion.smoothedCombatAnchor = motion.combatCenter;
	motion.entryDuration = definition.motion.entryDuration;
	motion.combatDuration = definition.motion.combatDuration;
	motion.exitDuration = definition.motion.exitDuration;
	motion.orbitRadiusX = definition.motion.orbitRadiusX;
	motion.orbitRadiusY = definition.motion.orbitRadiusY;
	motion.orbitAngularSpeed = definition.motion.orbitAngularSpeed;
	motion.formationSpacing = definition.motion.formationSpacing;

	EnemyCombatArea area{};
	area.halfWidth = definition.motion.combatAreaHalfWidth;
	area.halfHeight = definition.motion.combatAreaHalfHeight;
	area.combatDistance = definition.motion.combatDistance;

	EnemyFormationSpawnBounds bounds{};
	bounds.offscreenMarginX = definition.motion.offscreenMarginX;
	bounds.offscreenMarginY = definition.motion.offscreenMarginY;
	bounds.minimumSpawnDistance = definition.motion.minimumSpawnDistance;

	auto group = std::make_unique<EnemyGroup>();
	group->SetGroupId(nextGroupId_++);
	group->Initialize(members.front(), definition.formationPattern, definition.attackPattern, motion, area, bounds);
	for (size_t i = 1; i < members.size(); ++i) {
		group->AddMember(members[i], static_cast<int>(i));
	}
	groups_.push_back(std::move(group));
}

bool WaveController::AreCurrentWaveGroupsSpawned() const {
	return std::all_of(spawnedGroupFlags_.begin(), spawnedGroupFlags_.end(), [](bool spawned) { return spawned; });
}

bool WaveController::AreGroupsFinished() const {
	return std::all_of(groups_.begin(), groups_.end(), [](const std::unique_ptr<EnemyGroup> &group) {
		return !group || group->IsFinished();
	});
}

void WaveController::AdvanceWaveOrComplete() {
	if (currentWaveIndex_ + 1 >= stageDefinition_.waves.size()) {
		currentWaveIndex_ = stageDefinition_.waves.size();
		state_ = WaveState::Completed;
		return;
	}

	const float delay = stageDefinition_.waves[currentWaveIndex_].nextWaveDelaySeconds;
	++currentWaveIndex_;
	state_ = WaveState::Waiting;
	waveTimer_ = -std::max(0.0f, delay);
	groups_.clear();
	ResetSpawnFlags();
}

void GameFlowController::Update(float deltaTime, EnemyManager &enemyManager, Player *player) {
	const float safeDeltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
	if (state_ == GameFlowState::Cleared || state_ == GameFlowState::GameOver) {
		return;
	}

	if (player && player->IsDefeatAnimationComplete()) {
		state_ = GameFlowState::GameOver;
		return;
	}

	const Vector3 playerPosition = player ? player->GetPosition() : Vector3{0.0f, 0.0f, 0.0f};
	if (state_ == GameFlowState::Playing) {
		waveController_.Update(safeDeltaTime, enemyManager, playerPosition);
		enemyManager.Update(safeDeltaTime);
		if (waveController_.IsStageCompleted() && enemyManager.GetActiveEnemyCount() == 0 && waveController_.GetActiveGroupCount() == 0) {
			state_ = GameFlowState::ClearPending;
			clearDelayTimer_ = 0.0f;
		}
		return;
	}

	if (state_ == GameFlowState::ClearPending) {
		enemyManager.Update(safeDeltaTime);
		if (enemyManager.GetActiveEnemyCount() == 0 && waveController_.GetActiveGroupCount() == 0) {
			clearDelayTimer_ += safeDeltaTime;
			if (clearDelayTimer_ >= stageDefinition_.clearDelaySeconds) {
				state_ = GameFlowState::Cleared;
			}
		}
	}
}

void GameFlowController::Clear() {
	waveController_.Clear();
	state_ = GameFlowState::Intro;
	clearDelayTimer_ = 0.0f;
}

const char *ToString(WaveState state) {
	switch (state) {
	case WaveState::Waiting: return "Waiting";
	case WaveState::Spawning: return "Spawning";
	case WaveState::Active: return "Active";
	case WaveState::Completed: return "Completed";
	case WaveState::Failed: return "Failed";
	default: return "Unknown";
	}
}

const char *ToString(GameFlowState state) {
	switch (state) {
	case GameFlowState::Intro: return "Intro";
	case GameFlowState::Playing: return "Playing";
	case GameFlowState::ClearPending: return "ClearPending";
	case GameFlowState::Cleared: return "Cleared";
	case GameFlowState::GameOver: return "GameOver";
	default: return "Unknown";
	}
}

const char *ToString(EnemyGroupFinishReason reason) {
	switch (reason) {
	case EnemyGroupFinishReason::None: return "None";
	case EnemyGroupFinishReason::AllMembersDestroyed: return "AllMembersDestroyed";
	case EnemyGroupFinishReason::AllMembersExited: return "AllMembersExited";
	case EnemyGroupFinishReason::MixedDestroyedAndExited: return "MixedDestroyedAndExited";
	case EnemyGroupFinishReason::Cancelled: return "Cancelled";
	default: return "Unknown";
	}
}

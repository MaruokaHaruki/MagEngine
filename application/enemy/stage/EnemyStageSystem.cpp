#define NOMINMAX
#include "stage/EnemyStageSystem.h"
#include "EnemyManager.h"
#include "EnemyGunner.h"
#include "Player.h"
#include <cmath>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace {
	std::optional<EnemyFormationPattern> ParseFormationPattern(const std::string &value) {
		if (value == "HorizontalLine") return EnemyFormationPattern::HorizontalLine;
		if (value == "VShape") return EnemyFormationPattern::VShape;
		if (value == "Circle") return EnemyFormationPattern::Circle;
		if (value == "FigureEight") return EnemyFormationPattern::FigureEight;
		if (value == "Column") return EnemyFormationPattern::Column;
		return std::nullopt;
	}

	std::optional<EnemyGroupAttackPattern> ParseAttackPattern(const std::string &value) {
		if (value == "None") return EnemyGroupAttackPattern::None;
		if (value == "Staggered") return EnemyGroupAttackPattern::Staggered;
		if (value == "LeaderThenWing") return EnemyGroupAttackPattern::LeaderThenWing;
		if (value == "Alternating") return EnemyGroupAttackPattern::Alternating;
		return std::nullopt;
	}

	std::optional<EnemyArchetype> ParseArchetype(const std::string &value) {
		if (value == "Standard") return EnemyArchetype::Standard;
		if (value == "Gunner") return EnemyArchetype::Gunner;
		return std::nullopt;
	}

	bool RequireString(const nlohmann::json &jsonData, const char *key, const std::string &jsonPath, std::string &value, std::string &errorMessage) {
		if (!jsonData.contains(key) || !jsonData[key].is_string() || jsonData[key].get<std::string>().empty()) {
			errorMessage = jsonPath + "." + key + " must be a non-empty string";
			return false;
		}
		value = jsonData[key].get<std::string>();
		return true;
	}

	bool IsFinite(float value) {
		return std::isfinite(value);
	}

	void ReadOptionalFloat(const nlohmann::json &jsonData, const char *key, float &value) {
		if (jsonData.contains(key)) {
			value = jsonData.at(key).get<float>();
		}
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

StageLoadResult StageDefinitionLoader::Load(const std::string &path) {
	StageLoadResult result{};
	result.sourcePath = path;
	std::ifstream file(path);
	if (!file.is_open()) {
		result.errorMessage = path + ": stage file could not be opened";
		return result;
	}

	try {
		nlohmann::json jsonData;
		file >> jsonData;
		if (!Parse(jsonData, result.stageDefinition, result.errorMessage)) {
			result.errorMessage = path + ": " + result.errorMessage;
			return result;
		}
		if (!Validate(result.stageDefinition, path, result.errorMessage)) {
			return result;
		}
	} catch (const std::exception &exception) {
		result.errorMessage = path + ": JSON parse error: " + exception.what();
	}
	return result;
}

bool StageDefinitionLoader::Parse(const nlohmann::json &jsonData, StageDefinition &stage, std::string &errorMessage) {
	if (!jsonData.is_object() || !RequireString(jsonData, "stageId", "$", stage.stageId, errorMessage) ||
		!jsonData.contains("waves") || !jsonData["waves"].is_array()) {
		if (errorMessage.empty()) errorMessage = "$.waves must be an array";
		return false;
	}
	ReadOptionalFloat(jsonData, "clearDelaySeconds", stage.clearDelaySeconds);
	if (jsonData.contains("clearWhenAllWavesCompleted")) {
		stage.clearWhenAllWavesCompleted = jsonData.at("clearWhenAllWavesCompleted").get<bool>();
	}

	for (size_t waveIndex = 0; waveIndex < jsonData["waves"].size(); ++waveIndex) {
		const auto &waveJson = jsonData["waves"][waveIndex];
		const std::string wavePath = "$.waves[" + std::to_string(waveIndex) + "]";
		if (!waveJson.is_object()) { errorMessage = wavePath + " must be an object"; return false; }
		WaveDefinition wave{};
		if (!RequireString(waveJson, "waveId", wavePath, wave.waveId, errorMessage) || !waveJson.contains("spawnGroups") || !waveJson["spawnGroups"].is_array()) {
			if (errorMessage.empty()) errorMessage = wavePath + ".spawnGroups must be an array";
			return false;
		}
		ReadOptionalFloat(waveJson, "startDelaySeconds", wave.startDelaySeconds);
		ReadOptionalFloat(waveJson, "nextWaveDelaySeconds", wave.nextWaveDelaySeconds);
		if (waveJson.contains("waitForAllGroupsFinished")) wave.waitForAllGroupsFinished = waveJson.at("waitForAllGroupsFinished").get<bool>();
		if (waveJson.contains("waitForAllEnemiesRemoved")) wave.waitForAllEnemiesRemoved = waveJson.at("waitForAllEnemiesRemoved").get<bool>();

		for (size_t groupIndex = 0; groupIndex < waveJson["spawnGroups"].size(); ++groupIndex) {
			const auto &groupJson = waveJson["spawnGroups"][groupIndex];
			const std::string groupPath = wavePath + ".spawnGroups[" + std::to_string(groupIndex) + "]";
			if (!groupJson.is_object()) { errorMessage = groupPath + " must be an object"; return false; }
			SpawnGroupDefinition group{};
			std::string formationText;
			std::string attackText;
			if (!RequireString(groupJson, "groupId", groupPath, group.groupId, errorMessage) ||
				!RequireString(groupJson, "formationPattern", groupPath, formationText, errorMessage) ||
				!RequireString(groupJson, "attackPattern", groupPath, attackText, errorMessage) ||
				!groupJson.contains("members") || !groupJson["members"].is_array()) {
				if (errorMessage.empty()) errorMessage = groupPath + ".members must be an array";
				return false;
			}
			const auto formationPattern = ParseFormationPattern(formationText);
			const auto attackPattern = ParseAttackPattern(attackText);
			if (!formationPattern || !attackPattern) {
				errorMessage = groupPath + (formationPattern ? ".attackPattern has invalid value '" + attackText + "'" : ".formationPattern has invalid value '" + formationText + "'");
				return false;
			}
			group.formationPattern = *formationPattern;
			group.attackPattern = *attackPattern;
			ReadOptionalFloat(groupJson, "spawnDelaySeconds", group.spawnDelaySeconds);

			for (size_t memberIndex = 0; memberIndex < groupJson["members"].size(); ++memberIndex) {
				const auto &memberJson = groupJson["members"][memberIndex];
				const std::string memberPath = groupPath + ".members[" + std::to_string(memberIndex) + "]";
				if (!memberJson.is_object()) { errorMessage = memberPath + " must be an object"; return false; }
				std::string archetypeText;
				if (!RequireString(memberJson, "archetype", memberPath, archetypeText, errorMessage) || !memberJson.contains("count")) return false;
				const auto archetype = ParseArchetype(archetypeText);
				if (!archetype) { errorMessage = memberPath + ".archetype has invalid value '" + archetypeText + "'"; return false; }
				EnemySpawnDefinition member{};
				member.archetype = *archetype;
				member.count = memberJson.at("count").get<uint32_t>();
				ReadOptionalFloat(memberJson, "healthMultiplier", member.healthMultiplier);
				ReadOptionalFloat(memberJson, "speedMultiplier", member.speedMultiplier);
				ReadOptionalFloat(memberJson, "shotDelayOffset", member.shotDelayOffset);
				group.members.push_back(member);
			}

			if (groupJson.contains("motion")) {
				const auto &motionJson = groupJson.at("motion");
				if (!motionJson.is_object()) { errorMessage = groupPath + ".motion must be an object"; return false; }
				ReadOptionalFloat(motionJson, "entryDuration", group.motion.entryDuration);
				ReadOptionalFloat(motionJson, "combatDuration", group.motion.combatDuration);
				ReadOptionalFloat(motionJson, "exitDuration", group.motion.exitDuration);
				ReadOptionalFloat(motionJson, "combatDistance", group.motion.combatDistance);
				ReadOptionalFloat(motionJson, "combatAreaHalfWidth", group.motion.combatAreaHalfWidth);
				ReadOptionalFloat(motionJson, "combatAreaHalfHeight", group.motion.combatAreaHalfHeight);
				ReadOptionalFloat(motionJson, "formationSpacing", group.motion.formationSpacing);
				ReadOptionalFloat(motionJson, "orbitRadiusX", group.motion.orbitRadiusX);
				ReadOptionalFloat(motionJson, "orbitRadiusY", group.motion.orbitRadiusY);
				ReadOptionalFloat(motionJson, "orbitAngularSpeed", group.motion.orbitAngularSpeed);
				ReadOptionalFloat(motionJson, "attackInterval", group.motion.attackInterval);
				ReadOptionalFloat(motionJson, "attackSlotDelay", group.motion.attackSlotDelay);
				ReadOptionalFloat(motionJson, "offscreenMarginX", group.motion.offscreenMarginX);
				ReadOptionalFloat(motionJson, "offscreenMarginY", group.motion.offscreenMarginY);
				ReadOptionalFloat(motionJson, "minimumSpawnDistance", group.motion.minimumSpawnDistance);
			}
			wave.spawnGroups.push_back(std::move(group));
		}
		stage.waves.push_back(std::move(wave));
	}
	return true;
}

bool StageDefinitionLoader::Validate(const StageDefinition &stageDefinition, const std::string &sourcePath, std::string &errorMessage) {
	const auto fail = [&errorMessage, &sourcePath](const std::string &jsonPath, const std::string &reason) {
		errorMessage = sourcePath + ": " + jsonPath + " " + reason;
		return false;
	};
	if (stageDefinition.stageId.empty() || stageDefinition.waves.empty()) {
		return fail("$", "requires a stageId and at least one wave");
	}
	if (!stageDefinition.clearWhenAllWavesCompleted) {
		return fail("$.clearWhenAllWavesCompleted", "false is not supported by the current GameFlowController");
	}
	if (!IsFinite(stageDefinition.clearDelaySeconds) || stageDefinition.clearDelaySeconds < 0.0f) {
		return fail("$.clearDelaySeconds", "must be a finite value greater than or equal to zero");
	}

	std::unordered_set<std::string> waveIds;
	std::unordered_set<std::string> groupIds;
	for (size_t waveIndex = 0; waveIndex < stageDefinition.waves.size(); ++waveIndex) {
		const WaveDefinition &wave = stageDefinition.waves[waveIndex];
		const std::string wavePath = "$.waves[" + std::to_string(waveIndex) + "]";
		if (!waveIds.insert(wave.waveId).second) return fail(wavePath + ".waveId", "must be unique");
		if (wave.spawnGroups.empty()) return fail(wavePath + ".spawnGroups", "must contain at least one group");
		if (!IsFinite(wave.startDelaySeconds) || wave.startDelaySeconds < 0.0f || !IsFinite(wave.nextWaveDelaySeconds) || wave.nextWaveDelaySeconds < 0.0f) {
			return fail(wavePath, "contains a negative or non-finite wave delay");
		}

		for (size_t groupIndex = 0; groupIndex < wave.spawnGroups.size(); ++groupIndex) {
			const SpawnGroupDefinition &group = wave.spawnGroups[groupIndex];
			const std::string groupPath = wavePath + ".spawnGroups[" + std::to_string(groupIndex) + "]";
			if (!groupIds.insert(group.groupId).second) return fail(groupPath + ".groupId", "must be unique within the stage");
			if (group.members.empty()) return fail(groupPath + ".members", "must contain at least one enemy definition");
			if (!IsFinite(group.spawnDelaySeconds) || group.spawnDelaySeconds < 0.0f) return fail(groupPath + ".spawnDelaySeconds", "must be finite and greater than or equal to zero");
			const EnemyFormationMotionDefinition &motion = group.motion;
			if (!IsFinite(motion.entryDuration) || motion.entryDuration <= 0.0f || !IsFinite(motion.combatDuration) || motion.combatDuration <= 0.0f ||
				!IsFinite(motion.exitDuration) || motion.exitDuration <= 0.0f || !IsFinite(motion.combatDistance) || motion.combatDistance <= 0.0f ||
				!IsFinite(motion.combatAreaHalfWidth) || motion.combatAreaHalfWidth <= 0.0f || !IsFinite(motion.combatAreaHalfHeight) || motion.combatAreaHalfHeight <= 0.0f ||
				!IsFinite(motion.formationSpacing) || motion.formationSpacing <= 0.0f || !IsFinite(motion.orbitRadiusX) || motion.orbitRadiusX < 0.0f ||
				!IsFinite(motion.orbitRadiusY) || motion.orbitRadiusY < 0.0f || !IsFinite(motion.orbitAngularSpeed) ||
				!IsFinite(motion.attackInterval) || motion.attackInterval <= 0.0f || !IsFinite(motion.attackSlotDelay) || motion.attackSlotDelay < 0.0f ||
				!IsFinite(motion.offscreenMarginX) || motion.offscreenMarginX <= 0.0f || !IsFinite(motion.offscreenMarginY) || motion.offscreenMarginY <= 0.0f ||
				!IsFinite(motion.minimumSpawnDistance) || motion.minimumSpawnDistance <= 0.0f) {
				return fail(groupPath + ".motion", "contains an invalid duration, distance, interval, or finite value");
			}

			for (size_t memberIndex = 0; memberIndex < group.members.size(); ++memberIndex) {
				const EnemySpawnDefinition &member = group.members[memberIndex];
				const std::string memberPath = groupPath + ".members[" + std::to_string(memberIndex) + "]";
				if (member.count == 0) return fail(memberPath + ".count", "must be greater than zero");
				if (!IsFinite(member.healthMultiplier) || member.healthMultiplier <= 0.0f || member.healthMultiplier > 100.0f) return fail(memberPath + ".healthMultiplier", "must be finite and within (0, 100]");
				if (!IsFinite(member.speedMultiplier) || member.speedMultiplier <= 0.0f || member.speedMultiplier > 100.0f) return fail(memberPath + ".speedMultiplier", "must be finite and within (0, 100]");
				if (!IsFinite(member.shotDelayOffset)) return fail(memberPath + ".shotDelayOffset", "must be finite");
				if (member.archetype == EnemyArchetype::Standard && member.shotDelayOffset != 0.0f) return fail(memberPath + ".shotDelayOffset", "is only supported by Gunner");
				if (member.archetype == EnemyArchetype::Gunner && EnemyGunnerConstants::kShootInterval + member.shotDelayOffset <= 0.0f) {
					return fail(memberPath + ".shotDelayOffset", "makes the final Gunner shoot interval less than or equal to zero");
				}
			}
		}
	}
	return true;
}

void WaveController::Update(float deltaTime, EnemyManager &enemyManager, const Vector3 &playerPosition) {
	if (state_ == WaveState::Completed || state_ == WaveState::Failed) {
		return;
	}
	if (currentWaveIndex_ >= stageDefinition_.waves.size()) {
		state_ = WaveState::Completed;
		return;
	}

	waveTimer_ += deltaTime;
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
			group->Update(deltaTime, playerPosition);
		}
	}

	const bool groupsReady = !wave.waitForAllGroupsFinished || AreGroupsFinished();
	const bool enemiesReady = !wave.waitForAllEnemiesRemoved || enemyManager.GetActiveEnemyCount() == 0;
	if (AreCurrentWaveGroupsSpawned() && groupsReady && enemiesReady) {
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
	std::vector<EnemyHandle> memberHandles;
	for (const EnemySpawnDefinition &memberDefinition : definition.members) {
		for (uint32_t i = 0; i < memberDefinition.count; ++i) {
			const float slot = static_cast<float>(memberHandles.size());
			const Vector3 spawnPosition = {entryPosition.x + (slot - 1.0f) * 6.0f, entryPosition.y, entryPosition.z - slot * 3.0f};
			if (EnemyBase *enemy = enemyManager.CreateEnemy(memberDefinition, spawnPosition)) {
				memberHandles.push_back(enemy->GetHandle());
			}
		}
	}
	if (memberHandles.empty()) {
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
	motion.attackInterval = definition.motion.attackInterval;
	motion.attackSlotDelay = definition.motion.attackSlotDelay;

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
	group->Initialize(&enemyManager, memberHandles.front(), definition.formationPattern, definition.attackPattern, motion, area, bounds);
	for (size_t i = 1; i < memberHandles.size(); ++i) {
		group->AddMember(memberHandles[i], static_cast<int>(i));
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
		requiresEnemyRemovalForStageCompletion_ = stageDefinition_.waves[currentWaveIndex_].waitForAllEnemiesRemoved;
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
	if (state_ == GameFlowState::Cleared || state_ == GameFlowState::GameOver) {
		return;
	}

	if (player && player->IsDefeatAnimationComplete()) {
		state_ = GameFlowState::GameOver;
		return;
	}

	const Vector3 playerPosition = player ? player->GetPosition() : Vector3{0.0f, 0.0f, 0.0f};
	if (state_ == GameFlowState::Playing) {
		waveController_.Update(deltaTime, enemyManager, playerPosition);
		enemyManager.Update(deltaTime);
		const bool enemiesReady = !waveController_.RequiresEnemyRemovalForStageCompletion() || enemyManager.GetActiveEnemyCount() == 0;
		if (waveController_.IsStageCompleted() && enemiesReady && waveController_.GetActiveGroupCount() == 0) {
			state_ = GameFlowState::ClearPending;
			clearDelayTimer_ = 0.0f;
		}
		return;
	}

	if (state_ == GameFlowState::ClearPending) {
		enemyManager.Update(deltaTime);
		const bool enemiesReady = !waveController_.RequiresEnemyRemovalForStageCompletion() || enemyManager.GetActiveEnemyCount() == 0;
		if (enemiesReady && waveController_.GetActiveGroupCount() == 0) {
			clearDelayTimer_ += deltaTime;
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

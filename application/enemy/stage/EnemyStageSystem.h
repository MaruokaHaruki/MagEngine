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

/// @brief Stage定義から選択する敵の基本種別
/// @note この列挙値はステージJSONと対応するため、既存値の並びや名称を変更する場合はロード処理も更新する。
enum class EnemyArchetype {
	Standard,
	Gunner,
};

/// @brief 1体の敵に適用する生成パラメータ
/// @note 乗算値は敵アーキタイプ固有の基準値に対して適用する。0以下の値は定義検証で拒否する。
struct EnemySpawnDefinition {
	EnemyArchetype archetype = EnemyArchetype::Standard;
	uint32_t count = 1;
	float healthMultiplier = 1.0f;
	float speedMultiplier = 1.0f;
	float shotDelayOffset = 0.0f;
};

/// @brief 敵グループの隊形移動に関するステージ定義
/// @note 秒・ワールド座標系の値を混在させるため、設定値の単位を変えずにJSONと同期する。
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

/// @brief 同時に生成・制御される敵グループの定義
/// @details membersの各要素はグループ内の生成構成を表す。EnemyGroupが実行時の状態を所有する。
struct SpawnGroupDefinition {
	std::string groupId;
	EnemyFormationPattern formationPattern = EnemyFormationPattern::HorizontalLine;
	EnemyGroupAttackPattern attackPattern = EnemyGroupAttackPattern::Staggered;
	std::vector<EnemySpawnDefinition> members;
	float spawnDelaySeconds = 0.0f;
	EnemyFormationMotionDefinition motion;
};

/// @brief 複数グループを順序制御するウェーブの定義
/// @note 完了待ちのフラグは演出とゲーム進行に直結するため、未使用に見えてもステージ仕様として保持する。
struct WaveDefinition {
	std::string waveId;
	float startDelaySeconds = 0.0f;
	float nextWaveDelaySeconds = 2.0f;
	std::vector<SpawnGroupDefinition> spawnGroups;
	bool waitForAllGroupsFinished = true;
	bool waitForAllEnemiesRemoved = true;
};

/// @brief 1ステージ全体のウェーブ進行を表す読み込み結果
/// @note JSON由来のデータであり、実行時状態はWaveControllerとGameFlowControllerが別途保持する。
struct StageDefinition {
	std::string stageId = "stage_01";
	std::vector<WaveDefinition> waves;
	float clearDelaySeconds = 2.0f;
	bool clearWhenAllWavesCompleted = true;
};

/// @brief StageDefinitionLoaderの読み込み結果
/// @note 失敗時もsourcePathを保持し、Scene初期化時に原因を表示できるようにする。
struct StageLoadResult {
	StageDefinition stageDefinition{};
	std::string sourcePath;
	std::string errorMessage;

	[[nodiscard]] bool IsSuccess() const {
		return errorMessage.empty();
	}
};

/// @brief WaveController内部のウェーブ進行状態
enum class WaveState {
	Waiting,
	Spawning,
	Active,
	Completed,
	Failed,
};

/// @brief プレイ中ステージのゲーム進行状態
/// @note ClearPendingはクリア演出待ちを表し、即時にScene遷移させないために分離している。
enum class GameFlowState {
	Intro,
	Playing,
	ClearPending,
	Cleared,
	GameOver,
};

/// @brief ステージJSONを実行可能なStageDefinitionへ変換・検証するローダー
/// @note Load()はファイルI/Oを行うため、毎フレーム呼び出してはならない。通常はScene初期化時に使用する。
class StageDefinitionLoader {
public:
	/// @brief JSONファイルを読み込み、構文とゲーム進行上の制約を検証
	/// @return 失敗時はerrorMessageを設定したStageLoadResultを返す。
	static StageLoadResult Load(const std::string &path);
	/// @brief 読み込み済み定義をファイルI/Oなしで検証
	/// @param errorMessage 失敗理由の出力先。成功時は空文字列になる。
	static bool Validate(const StageDefinition &stageDefinition, const std::string &sourcePath, std::string &errorMessage);

private:
	static bool Parse(const nlohmann::json &jsonData, StageDefinition &stageDefinition, std::string &errorMessage);
};

/// @brief 現在のウェーブと敵グループを更新し、次ウェーブへの遷移を判定するクラス
/// @details EnemyGroupの所有権はこのクラスが持つ。Enemy本体の所有権はEnemyManagerにある。
class WaveController {
public:
	/// @brief 指定ステージから進行状態を初期化
	/// @note 以前のグループ状態を破棄するため、プレイ中の再初期化は行わない。
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

	/// @brief 現在ウェーブを更新し、必要な敵グループを生成
	/// @note enemyManagerは敵の所有者であり、WaveControllerは生成依頼のみを行う。
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
	std::vector<std::unique_ptr<EnemyGroup>> groups_; // ウェーブ内グループの所有権。ウェーブ完了またはClear()で破棄する。
	int nextGroupId_ = 0;
	bool requiresEnemyRemovalForStageCompletion_ = true;
};

/// @brief ウェーブ進行とプレイヤー状態からステージのクリア・ゲームオーバーを判定するクラス
/// @note Scene遷移そのものは担当しない。呼び出し側はGetState()を参照して演出・遷移を決定する。
class GameFlowController {
public:
	/// @brief ステージ開始状態へ初期化
	void Initialize(const StageDefinition &stageDefinition) {
		stageDefinition_ = stageDefinition;
		waveController_.Initialize(stageDefinition_);
		state_ = GameFlowState::Playing;
		clearDelayTimer_ = 0.0f;
	}

	/// @brief プレイヤー状態とウェーブ進行を更新
	/// @param player Sceneが所有する非所有参照。nullptrの場合はゲームオーバー判定を行わない。
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

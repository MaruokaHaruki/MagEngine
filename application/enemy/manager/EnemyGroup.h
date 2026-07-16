/*********************************************************************
 * \file   EnemyGroup.h
 * \brief  敵のグループ管理クラス（編隊行動制御）
 *
 * \author Harukichimaru
 * \date   March 2026
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "EnemyHandle.h"
using Vector3 = MagMath::Vector3;
#include <cstdint>
#include <vector>
#include <memory>

// 前方宣言
class EnemyBase;
class EnemyManager;
class Player;

///=============================================================================
///						編隊フォーメーション定義
enum class FormationType {
	VFormation,		  // V字編隊
	LineFormation,	  // 直線編隊
	CircleFormation,  // 円形編隊
	DiamondFormation, // 菱形編隊
	DynamicFormation  // 動的編隊（プレイヤー位置に応じて変更）
};

enum class EnemyFormationPattern {
	HorizontalLine,
	VShape,
	Circle,
	FigureEight,
	Column,
	Count,
};

enum class EnemyGroupState {
	Enter,
	Combat,
	Exit,
	Finished,
};

enum class EnemyGroupAttackPattern {
	None,
	Staggered,
	LeaderThenWing,
	Alternating,
};

enum class EnemyGroupFinishReason {
	None,
	AllMembersDestroyed,
	AllMembersExited,
	MixedDestroyedAndExited,
	Cancelled,
};

namespace EnemyFormationConstants {
	constexpr float kEntryDuration = 3.2f;
	constexpr float kCombatDuration = 9.0f;
	constexpr float kExitDuration = 3.0f;
	constexpr float kFormationFollowSpeed = 30.0f;
	constexpr float kFormationFollowSharpness = 7.5f;
	constexpr float kGroupCenterSharpness = 4.5f;
	constexpr float kCombatAnchorSharpness = 2.0f;
	constexpr float kOrbitRadiusX = 18.0f;
	constexpr float kOrbitRadiusY = 7.0f;
	constexpr float kOrbitAngularSpeed = 1.2f;
	constexpr float kFormationSpacing = 12.0f;
	constexpr float kAttackInterval = 1.6f;
	constexpr float kAttackSlotDelay = 0.22f;
	constexpr float kAttackWindow = 0.35f;
	constexpr float kMaxFormationJitter = 1.5f;
	constexpr float kCombatForwardDistance = 72.0f;
	constexpr float kCombatAreaHalfWidth = 30.0f;
	constexpr float kCombatAreaHalfHeight = 14.0f;
	constexpr float kOffscreenMarginX = 90.0f;
	constexpr float kOffscreenMarginY = 28.0f;
	constexpr float kEntryLeadDistance = 18.0f;
	constexpr float kMinimumSpawnDistance = 120.0f;
}

struct EnemyCombatArea {
	float halfWidth = EnemyFormationConstants::kCombatAreaHalfWidth;
	float halfHeight = EnemyFormationConstants::kCombatAreaHalfHeight;
	float combatDistance = EnemyFormationConstants::kCombatForwardDistance;
	float edgePadding = 4.0f;
};

struct EnemyFormationSpawnBounds {
	float offscreenMarginX = EnemyFormationConstants::kOffscreenMarginX;
	float offscreenMarginY = EnemyFormationConstants::kOffscreenMarginY;
	float entryLeadDistance = EnemyFormationConstants::kEntryLeadDistance;
	float minimumSpawnDistance = EnemyFormationConstants::kMinimumSpawnDistance;
};

struct EnemyGroupMotion {
	Vector3 entryPosition{};
	Vector3 combatCenter{};
	Vector3 exitPosition{};
	Vector3 smoothedCombatAnchor{};
	Vector3 groupCenterVelocity{};

	float elapsedTime = 0.0f;
	float phaseTime = 0.0f;

	float moveSpeed = 8.0f;
	float entryDuration = EnemyFormationConstants::kEntryDuration;
	float combatDuration = EnemyFormationConstants::kCombatDuration;
	float exitDuration = EnemyFormationConstants::kExitDuration;
	float orbitRadiusX = EnemyFormationConstants::kOrbitRadiusX;
	float orbitRadiusY = EnemyFormationConstants::kOrbitRadiusY;
	float orbitAngularSpeed = EnemyFormationConstants::kOrbitAngularSpeed;
	float formationSpacing = EnemyFormationConstants::kFormationSpacing;
	float attackInterval = EnemyFormationConstants::kAttackInterval;
	float attackSlotDelay = EnemyFormationConstants::kAttackSlotDelay;
};

///=============================================================================
///						編隊設定構造体
struct FormationConfig {
	FormationType type;
	Vector3 offsets[8];     // 各敵の相対位置（最大8敵対応）
	float spacing;          // 敵間距離
	float cohesionStrength; // 集団結束度（0～1）
	float separationStrength; // 分離強度（敵同士の衝突回避）
	float alignmentStrength;  // 方向揃え強度
	int maxMemberCount;       // 編隊内の最大敵数
};

///=============================================================================
///						EnemyGroupクラス
/**
 * @brief 複数の敵で構成される編隊管理クラス
 *
 * 責務：
 * - 編隊内の敵の統一管理
 * - フォーメーション計算と位置制御
 * - リーダー敵とフォロワー敵の役割管理
 * - 編隊状態の管理（移動、戦闘、退却等）
 */
class EnemyGroup {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief コンストラクタ
	EnemyGroup();

	/// \brief デストラクタ
	~EnemyGroup() = default;

	/// \brief グループ初期化
	void Initialize(EnemyManager *enemyManager, EnemyHandle leaderHandle, FormationType formationType);
	void Initialize(EnemyManager *enemyManager, EnemyHandle leaderHandle, EnemyFormationPattern pattern, EnemyGroupAttackPattern attackPattern, const EnemyGroupMotion &motion, const EnemyCombatArea &combatArea, const EnemyFormationSpawnBounds &spawnBounds);

	/// \brief グループにメンバを追加
	void AddMember(EnemyHandle memberHandle, int positionIndex);

	/// \brief 更新（編隊制御ロジック）
	void Update(const Vector3 &playerPosition);
	void Update(float deltaTime, const Vector3 &playerPosition);

	/// \brief グループ内の敵削除処理
	void RemoveDeadMembers();

	/// \brief グループの活性状態確認
	bool IsActive() const;
	bool IsFinished() const {
		return groupState_ == EnemyGroupState::Finished;
	}

	EnemyGroupFinishReason GetFinishReason() const {
		return finishReason_;
	}

	uint32_t GetAliveMemberCount() const {
		return static_cast<uint32_t>(GetAliveCount());
	}

	uint32_t GetActiveMemberCount() const {
		return static_cast<uint32_t>(memberEnemyHandles_.size());
	}

	/// \brief リーダー敵を取得
	EnemyHandle GetLeaderHandle() const { return leaderEnemyHandle_; }

	/// \brief メンバ敵を取得
	const std::vector<EnemyHandle> &GetMembers() const {
		return memberEnemyHandles_;
	}

	/// \brief グループ内の生存敵数
	size_t GetAliveCount() const;

	EnemyGroupState GetState() const {
		return groupState_;
	}

	EnemyFormationPattern GetFormationPattern() const {
		return formationPattern_;
	}

	EnemyGroupAttackPattern GetAttackPattern() const {
		return attackPattern_;
	}

	float GetElapsedTime() const {
		return motion_.elapsedTime;
	}

	float GetPhaseTime() const {
		return motion_.phaseTime;
	}

	const std::vector<Vector3> &GetTargetPositions() const {
		return memberTargetPositions_;
	}

	const Vector3 &GetGroupCenter() const {
		return groupCenter_;
	}

	const Vector3 &GetEntryPosition() const {
		return motion_.entryPosition;
	}

	const Vector3 &GetCombatAnchor() const {
		return motion_.smoothedCombatAnchor;
	}

	const EnemyCombatArea &GetCombatArea() const {
		return combatArea_;
	}

	float GetEntryDistance() const;

	Vector3 CalculateSlotOffsetForTest(EnemyFormationPattern pattern, uint32_t slotIndex, uint32_t memberCount, float elapsedTime) const;
	bool ShouldSlotAttackForTest(uint32_t slotIndex, uint32_t memberCount, float phaseTime) const;
	Vector3 CalculateSmoothedPositionForTest(const Vector3 &current, const Vector3 &target, float sharpness, float deltaTime) const;
	Vector3 ClampToCombatAreaForTest(const Vector3 &position, const Vector3 &playerPosition) const;

	/// \brief グループIDを設定
	void SetGroupId(int id) {
		groupId_ = id;
	}

	/// \brief グループIDを取得
	int GetGroupId() const {
		return groupId_;
	}

	///--------------------------------------------------------------
	///							フォーメーション関連
private:
	/// \brief フォーメーション設定の生成
	FormationConfig CreateFormationConfig(FormationType type);

	/// \brief V字フォーメーション設定
	FormationConfig CreateVFormation();

	/// \brief 直線フォーメーション設定
	FormationConfig CreateLineFormation();

	/// \brief 円形フォーメーション設定
	FormationConfig CreateCircleFormation();

	/// \brief 菱形フォーメーション設定
	FormationConfig CreateDiamondFormation();

	/// \brief 動的フォーメーション計算
	FormationConfig CalculateDynamicFormation(const Vector3 &playerPosition);

	/// \brief メンバの目標位置計算
	void CalculateMemberTargetPositions(const Vector3 &leaderPos, const Vector3 &playerPos);

	/// \brief メンバの相対位置追尾更新
	void UpdateMemberPositions(float deltaTime);

	EnemyFormationPattern ConvertFormationType(FormationType type) const;
	void InitializeMotionFromLeader(const Vector3 &playerPosition);
	void UpdateGroupState(float deltaTime, const Vector3 &playerPosition);
	Vector3 CalculateGroupCenter() const;
	Vector3 CalculateCombatAnchor(const Vector3 &playerPosition) const;
	Vector3 ClampToCombatArea(const Vector3 &position, const Vector3 &playerPosition) const;
	Vector3 SmoothPosition(const Vector3 &current, const Vector3 &target, float sharpness, float deltaTime) const;
	Vector3 CalculateSlotOffset(uint32_t slotIndex, uint32_t memberCount) const;
	bool ShouldSlotAttack(uint32_t slotIndex, uint32_t memberCount) const;
	float CalculateSlotDelay(uint32_t slotIndex, uint32_t memberCount) const;

	///--------------------------------------------------------------
	///							群動作ロジック
private:
	/// \brief Boid的な群制御（分離・結合・整列）
	Vector3 CalculateBoidForce(EnemyBase *member, const Vector3 &targetPos);

	/// \brief 分離処理（敵同士が近づきすぎないようにする）
	Vector3 CalculateSeparation(EnemyBase *member);

	/// \brief 結合処理（敵が集団中心に寄る）
	Vector3 CalculateCohesion(EnemyBase *member, const Vector3 &targetPos);

	/// \brief 方向整列処理（敵の向きを揃える）
	Vector3 CalculateAlignment(EnemyBase *member);

	///--------------------------------------------------------------
	///							メンバ変数
private:
	int groupId_;                           // グループID
	EnemyManager *enemyManager_ = nullptr; // Group より長く生存する非所有の解決元
	EnemyHandle leaderEnemyHandle_{};
	std::vector<EnemyHandle> memberEnemyHandles_;

	FormationConfig currentFormation_;       // 現在のフォーメーション
	std::vector<Vector3> memberTargetPositions_; // 各メンバの目標位置

	//========================================
	// グループ状態管理
	EnemyGroupState groupState_;
	EnemyFormationPattern formationPattern_;
	EnemyGroupAttackPattern attackPattern_;
	EnemyGroupMotion motion_;
	EnemyCombatArea combatArea_;
	EnemyFormationSpawnBounds spawnBounds_;
	EnemyGroupFinishReason finishReason_;
	uint32_t initialMemberCount_;
	uint32_t destroyedMemberCount_;
	uint32_t exitedMemberCount_;
	float stateTimer_;
	Vector3 groupCenter_;

	//========================================
	// パラメータ
	float formationUpdateTimer_;  // フォーメーション更新タイマー
	float minFormationUpdateInterval_; // フォーメーション更新最小間隔（毎フレーム更新を避ける）
	float attackInterval_ = EnemyFormationConstants::kAttackInterval;
	float attackSlotDelay_ = EnemyFormationConstants::kAttackSlotDelay;
};

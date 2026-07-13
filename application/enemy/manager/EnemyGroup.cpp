/*********************************************************************
 * \file   EnemyGroup.cpp
 * \brief  敵のグループ管理クラス実装
 *********************************************************************/
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "EnemyGroup.h"
#include "EnemyBase.h"
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kPi = 3.14159265358979323846f;

	float Clamp01(float value) {
		return std::max(0.0f, std::min(value, 1.0f));
	}

	Vector3 LerpVector(const Vector3 &a, const Vector3 &b, float t) {
		const float ratio = Clamp01(t);
		return {
			a.x + (b.x - a.x) * ratio,
			a.y + (b.y - a.y) * ratio,
			a.z + (b.z - a.z) * ratio,
		};
	}

	float LengthVector(const Vector3 &value) {
		return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
	}
}

///=============================================================================
/// コンストラクタ
EnemyGroup::EnemyGroup()
	: groupId_(-1),
	  leaderEnemy_(nullptr),
	  groupState_(EnemyGroupState::Finished),
	  formationPattern_(EnemyFormationPattern::VShape),
	  attackPattern_(EnemyGroupAttackPattern::Staggered),
	  finishReason_(EnemyGroupFinishReason::None),
	  initialMemberCount_(0),
	  destroyedMemberCount_(0),
	  exitedMemberCount_(0),
	  stateTimer_(0.0f),
	  groupCenter_({0.0f, 0.0f, 0.0f}),
	  formationUpdateTimer_(0.0f),
	  minFormationUpdateInterval_(0.0f) {
}

///=============================================================================
/// グループ初期化
void EnemyGroup::Initialize(EnemyBase *leaderEnemy, FormationType formationType) {
	leaderEnemy_ = leaderEnemy;
	memberEnemies_.clear();
	memberTargetPositions_.clear();

	if (leaderEnemy_) {
		memberEnemies_.push_back(leaderEnemy_);
		leaderEnemy_->SetFormationSlotIndex(0);
		leaderEnemy_->SetFormationFollowSpeed(EnemyFormationConstants::kFormationFollowSpeed);
		leaderEnemy_->SetFormationFollowSharpness(EnemyFormationConstants::kFormationFollowSharpness);
		leaderEnemy_->SetFormationFollowEnabled(true);
	}

	currentFormation_ = CreateFormationConfig(formationType);
	memberTargetPositions_.resize(currentFormation_.maxMemberCount);
	formationPattern_ = ConvertFormationType(formationType);
	attackPattern_ = EnemyGroupAttackPattern::Staggered;
	groupState_ = leaderEnemy_ ? EnemyGroupState::Enter : EnemyGroupState::Finished;
	finishReason_ = leaderEnemy_ ? EnemyGroupFinishReason::None : EnemyGroupFinishReason::Cancelled;
	initialMemberCount_ = leaderEnemy_ ? 1u : 0u;
	destroyedMemberCount_ = 0;
	exitedMemberCount_ = 0;
	stateTimer_ = 0.0f;
	motion_ = EnemyGroupMotion{};
	if (leaderEnemy_) {
		motion_.entryPosition = leaderEnemy_->GetPosition();
		motion_.combatCenter = leaderEnemy_->GetPosition();
		motion_.exitPosition = leaderEnemy_->GetPosition();
		motion_.smoothedCombatAnchor = leaderEnemy_->GetPosition();
		motion_.groupCenterVelocity = {0.0f, 0.0f, 0.0f};
		groupCenter_ = leaderEnemy_->GetPosition();
	}
}

void EnemyGroup::Initialize(EnemyBase *leaderEnemy, EnemyFormationPattern pattern, EnemyGroupAttackPattern attackPattern, const EnemyGroupMotion &motion, const EnemyCombatArea &combatArea, const EnemyFormationSpawnBounds &spawnBounds) {
	leaderEnemy_ = leaderEnemy;
	memberEnemies_.clear();
	memberTargetPositions_.clear();
	formationPattern_ = pattern;
	attackPattern_ = attackPattern;
	currentFormation_ = CreateVFormation();
	motion_ = motion;
	combatArea_ = combatArea;
	spawnBounds_ = spawnBounds;
	groupCenter_ = motion_.entryPosition;
	groupState_ = leaderEnemy_ ? EnemyGroupState::Enter : EnemyGroupState::Finished;
	finishReason_ = leaderEnemy_ ? EnemyGroupFinishReason::None : EnemyGroupFinishReason::Cancelled;
	stateTimer_ = 0.0f;
	initialMemberCount_ = 0;
	destroyedMemberCount_ = 0;
	exitedMemberCount_ = 0;

	if (leaderEnemy_) {
		AddMember(leaderEnemy_, 0);
		leaderEnemy_->SetFormationTarget(motion_.entryPosition);
	}
}

///=============================================================================
/// グループにメンバを追加
void EnemyGroup::AddMember(EnemyBase *member, int positionIndex) {
	if (!member || positionIndex < 0 || positionIndex >= currentFormation_.maxMemberCount) {
		return;
	}

	member->SetFormationSlotIndex(static_cast<uint32_t>(positionIndex));
	member->SetFormationFollowSpeed(EnemyFormationConstants::kFormationFollowSpeed);
	member->SetFormationFollowSharpness(EnemyFormationConstants::kFormationFollowSharpness);
	member->SetFormationFollowEnabled(true);
	memberEnemies_.push_back(member);
	initialMemberCount_ = std::max(initialMemberCount_, static_cast<uint32_t>(memberEnemies_.size()));
}

///=============================================================================
/// グループ更新（編隊制御ロジック）
void EnemyGroup::Update(const Vector3 &playerPosition) {
	Update(1.0f / 60.0f, playerPosition);
}

void EnemyGroup::Update(float deltaTime, const Vector3 &playerPosition) {
	const float safeDeltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
	RemoveDeadMembers();
	if (memberEnemies_.empty()) {
		groupState_ = EnemyGroupState::Finished;
		if (finishReason_ == EnemyGroupFinishReason::None) {
			finishReason_ = exitedMemberCount_ > 0 ? EnemyGroupFinishReason::MixedDestroyedAndExited : EnemyGroupFinishReason::AllMembersDestroyed;
		}
		return;
	}

	if (!leaderEnemy_ || !leaderEnemy_->IsAlive()) {
		leaderEnemy_ = memberEnemies_.front();
	}

	if (groupState_ == EnemyGroupState::Finished) {
		for (EnemyBase *member : memberEnemies_) {
			if (member) {
				member->ClearFormationTarget();
			}
		}
		memberEnemies_.clear();
		leaderEnemy_ = nullptr;
		return;
	}

	UpdateGroupState(safeDeltaTime, playerPosition);
	groupCenter_ = SmoothPosition(groupCenter_, CalculateGroupCenter(), EnemyFormationConstants::kGroupCenterSharpness, safeDeltaTime);
	UpdateMemberPositions(safeDeltaTime);
}

///=============================================================================
/// グループ内の敵削除処理
void EnemyGroup::RemoveDeadMembers() {
	memberEnemies_.erase(
		std::remove_if(memberEnemies_.begin(), memberEnemies_.end(),
					   [this](EnemyBase *enemy) {
						   const bool shouldRemove = !enemy || !enemy->IsAlive();
						   if (shouldRemove) {
							   ++destroyedMemberCount_;
						   }
						   return shouldRemove;
					   }),
		memberEnemies_.end());

	for (uint32_t i = 0; i < static_cast<uint32_t>(memberEnemies_.size()); ++i) {
		memberEnemies_[i]->SetFormationSlotIndex(i);
	}
}

///=============================================================================
/// グループの活性状態確認
bool EnemyGroup::IsActive() const {
	return !IsFinished() && !memberEnemies_.empty();
}

///=============================================================================
/// グループ内の生存敵数
size_t EnemyGroup::GetAliveCount() const {
	size_t count = 0;
	for (auto *member : memberEnemies_) {
		if (member && member->IsAlive()) {
			count++;
		}
	}
	return count;
}

FormationConfig EnemyGroup::CreateFormationConfig(FormationType type) {
	switch (type) {
	case FormationType::VFormation:
		return CreateVFormation();
	case FormationType::LineFormation:
		return CreateLineFormation();
	case FormationType::CircleFormation:
		return CreateCircleFormation();
	case FormationType::DiamondFormation:
		return CreateDiamondFormation();
	case FormationType::DynamicFormation:
	default:
		return CreateVFormation();
	}
}

FormationConfig EnemyGroup::CreateVFormation() {
	FormationConfig config{};
	config.type = FormationType::VFormation;
	config.spacing = EnemyFormationConstants::kFormationSpacing;
	config.cohesionStrength = 0.8f;
	config.separationStrength = 0.5f;
	config.alignmentStrength = 0.3f;
	config.maxMemberCount = 5;
	for (int i = 0; i < 5; ++i) {
		config.offsets[i] = CalculateSlotOffsetForTest(EnemyFormationPattern::VShape, static_cast<uint32_t>(i), 5, 0.0f);
	}
	return config;
}

FormationConfig EnemyGroup::CreateLineFormation() {
	FormationConfig config{};
	config.type = FormationType::LineFormation;
	config.spacing = EnemyFormationConstants::kFormationSpacing;
	config.cohesionStrength = 0.7f;
	config.separationStrength = 0.4f;
	config.alignmentStrength = 0.4f;
	config.maxMemberCount = 8;
	for (int i = 0; i < 8; ++i) {
		config.offsets[i] = CalculateSlotOffsetForTest(EnemyFormationPattern::HorizontalLine, static_cast<uint32_t>(i), 8, 0.0f);
	}
	return config;
}

FormationConfig EnemyGroup::CreateCircleFormation() {
	FormationConfig config{};
	config.type = FormationType::CircleFormation;
	config.spacing = EnemyFormationConstants::kFormationSpacing;
	config.cohesionStrength = 0.6f;
	config.separationStrength = 0.6f;
	config.alignmentStrength = 0.2f;
	config.maxMemberCount = 6;
	for (int i = 0; i < 6; ++i) {
		config.offsets[i] = CalculateSlotOffsetForTest(EnemyFormationPattern::Circle, static_cast<uint32_t>(i), 6, 0.0f);
	}
	return config;
}

FormationConfig EnemyGroup::CreateDiamondFormation() {
	FormationConfig config{};
	config.type = FormationType::DiamondFormation;
	config.spacing = EnemyFormationConstants::kFormationSpacing;
	config.cohesionStrength = 0.75f;
	config.separationStrength = 0.55f;
	config.alignmentStrength = 0.35f;
	config.maxMemberCount = 5;
	for (int i = 0; i < 5; ++i) {
		config.offsets[i] = CalculateSlotOffsetForTest(EnemyFormationPattern::FigureEight, static_cast<uint32_t>(i), 5, 0.0f);
	}
	return config;
}

FormationConfig EnemyGroup::CalculateDynamicFormation(const Vector3 &playerPosition) {
	playerPosition;
	return CreateCircleFormation();
}

void EnemyGroup::CalculateMemberTargetPositions(const Vector3 &leaderPos, const Vector3 &playerPos) {
	leaderPos;
	playerPos;
	memberTargetPositions_.resize(memberEnemies_.size());
	for (uint32_t i = 0; i < static_cast<uint32_t>(memberEnemies_.size()); ++i) {
		memberTargetPositions_[i] = groupCenter_ + CalculateSlotOffset(i, static_cast<uint32_t>(memberEnemies_.size()));
	}
}

void EnemyGroup::UpdateMemberPositions(float deltaTime) {
	deltaTime;
	const uint32_t memberCount = static_cast<uint32_t>(memberEnemies_.size());
	if (memberCount == 0) {
		return;
	}

	memberTargetPositions_.resize(memberCount);
	for (uint32_t i = 0; i < memberCount; ++i) {
		EnemyBase *member = memberEnemies_[i];
		if (!member || !member->IsAlive()) {
			continue;
		}

		const Vector3 targetPosition = groupCenter_ + CalculateSlotOffset(i, memberCount);
		memberTargetPositions_[i] = targetPosition;
		member->SetFormationSlotIndex(i);
		member->SetFormationTarget(targetPosition);
		member->SetFormationFollowSpeed(EnemyFormationConstants::kFormationFollowSpeed);
		member->SetFormationFollowSharpness(EnemyFormationConstants::kFormationFollowSharpness);
		member->SetFormationFollowEnabled(groupState_ != EnemyGroupState::Finished);
		member->SetFormationAttackEnabled(groupState_ == EnemyGroupState::Combat && ShouldSlotAttack(i, memberCount));
	}
}

Vector3 EnemyGroup::CalculateBoidForce(EnemyBase *member, const Vector3 &targetPos) {
	member;
	targetPos;
	return {0.0f, 0.0f, 0.0f};
}

Vector3 EnemyGroup::CalculateSeparation(EnemyBase *member) {
	member;
	return {0.0f, 0.0f, 0.0f};
}

Vector3 EnemyGroup::CalculateCohesion(EnemyBase *member, const Vector3 &targetPos) {
	member;
	targetPos;
	return {0.0f, 0.0f, 0.0f};
}

Vector3 EnemyGroup::CalculateAlignment(EnemyBase *member) {
	member;
	return {0.0f, 0.0f, 0.0f};
}

EnemyFormationPattern EnemyGroup::ConvertFormationType(FormationType type) const {
	switch (type) {
	case FormationType::LineFormation:
		return EnemyFormationPattern::HorizontalLine;
	case FormationType::VFormation:
		return EnemyFormationPattern::VShape;
	case FormationType::CircleFormation:
		return EnemyFormationPattern::Circle;
	case FormationType::DiamondFormation:
		return EnemyFormationPattern::FigureEight;
	case FormationType::DynamicFormation:
		return EnemyFormationPattern::Column;
	default:
		return EnemyFormationPattern::VShape;
	}
}

void EnemyGroup::InitializeMotionFromLeader(const Vector3 &playerPosition) {
	if (!leaderEnemy_) {
		return;
	}

	const Vector3 leaderPosition = leaderEnemy_->GetPosition();
	motion_.entryPosition = leaderPosition;
	motion_.combatCenter = CalculateCombatAnchor(playerPosition);
	motion_.smoothedCombatAnchor = motion_.combatCenter;

	const float exitX = (leaderPosition.x >= playerPosition.x) ? playerPosition.x + 80.0f : playerPosition.x - 80.0f;
	motion_.exitPosition = {
		exitX,
		playerPosition.y + 12.0f,
		playerPosition.z + EnemyFormationConstants::kCombatForwardDistance + 45.0f,
	};
	groupCenter_ = motion_.entryPosition;
}

void EnemyGroup::UpdateGroupState(float deltaTime, const Vector3 &playerPosition) {
	if (stateTimer_ <= 0.0f && motion_.elapsedTime <= 0.0f) {
		InitializeMotionFromLeader(playerPosition);
	}

	motion_.elapsedTime += deltaTime;
	motion_.phaseTime += deltaTime;
	stateTimer_ += deltaTime;

	const Vector3 combatAnchor = CalculateCombatAnchor(playerPosition);
	motion_.smoothedCombatAnchor = SmoothPosition(motion_.smoothedCombatAnchor, combatAnchor, EnemyFormationConstants::kCombatAnchorSharpness, deltaTime);
	motion_.combatCenter = ClampToCombatArea(motion_.smoothedCombatAnchor, playerPosition);

	switch (groupState_) {
	case EnemyGroupState::Enter:
		if (stateTimer_ >= std::max(0.01f, motion_.entryDuration)) {
			groupState_ = EnemyGroupState::Combat;
			stateTimer_ = 0.0f;
		}
		break;
	case EnemyGroupState::Combat:
		if (stateTimer_ >= std::max(0.01f, motion_.combatDuration)) {
			groupState_ = EnemyGroupState::Exit;
			stateTimer_ = 0.0f;
			const float exitX = groupCenter_.x >= playerPosition.x ? playerPosition.x + spawnBounds_.offscreenMarginX : playerPosition.x - spawnBounds_.offscreenMarginX;
			motion_.exitPosition = {exitX, playerPosition.y + spawnBounds_.offscreenMarginY * 0.5f, playerPosition.z + combatArea_.combatDistance + 50.0f};
		}
		break;
	case EnemyGroupState::Exit:
		if (stateTimer_ >= std::max(0.01f, motion_.exitDuration)) {
			groupState_ = EnemyGroupState::Finished;
			for (EnemyBase *member : memberEnemies_) {
				if (member) {
					member->MarkExited();
					member->ClearFormationTarget();
					++exitedMemberCount_;
				}
			}
			finishReason_ = destroyedMemberCount_ > 0 ? EnemyGroupFinishReason::MixedDestroyedAndExited : EnemyGroupFinishReason::AllMembersExited;
			memberEnemies_.clear();
			leaderEnemy_ = nullptr;
		}
		break;
	case EnemyGroupState::Finished:
	default:
		break;
	}
}

Vector3 EnemyGroup::CalculateGroupCenter() const {
	switch (groupState_) {
	case EnemyGroupState::Enter:
		return LerpVector(motion_.entryPosition, motion_.combatCenter, stateTimer_ / std::max(0.01f, motion_.entryDuration));
	case EnemyGroupState::Combat:
		return motion_.combatCenter;
	case EnemyGroupState::Exit:
		return LerpVector(motion_.combatCenter, motion_.exitPosition, stateTimer_ / std::max(0.01f, motion_.exitDuration));
	case EnemyGroupState::Finished:
	default:
		return groupCenter_;
	}
}

Vector3 EnemyGroup::CalculateCombatAnchor(const Vector3 &playerPosition) const {
	const float patternBiasX = (static_cast<int>(formationPattern_) % 2 == 0) ? -4.0f : 4.0f;
	const float patternBiasY = (formationPattern_ == EnemyFormationPattern::Circle || formationPattern_ == EnemyFormationPattern::FigureEight) ? 2.0f : 0.0f;
	const Vector3 anchor = {
		playerPosition.x + patternBiasX,
		playerPosition.y + patternBiasY,
		playerPosition.z + combatArea_.combatDistance,
	};
	return ClampToCombatArea(anchor, playerPosition);
}

Vector3 EnemyGroup::ClampToCombatArea(const Vector3 &position, const Vector3 &playerPosition) const {
	const float safeHalfWidth = std::max(4.0f, combatArea_.halfWidth - combatArea_.edgePadding);
	const float safeHalfHeight = std::max(3.0f, combatArea_.halfHeight - combatArea_.edgePadding);
	return {
		std::max(playerPosition.x - safeHalfWidth, std::min(position.x, playerPosition.x + safeHalfWidth)),
		std::max(playerPosition.y - safeHalfHeight, std::min(position.y, playerPosition.y + safeHalfHeight)),
		playerPosition.z + combatArea_.combatDistance,
	};
}

Vector3 EnemyGroup::SmoothPosition(const Vector3 &current, const Vector3 &target, float sharpness, float deltaTime) const {
	if (deltaTime <= 0.0f) {
		return current;
	}

	const float rate = 1.0f - std::exp(-std::max(0.1f, sharpness) * std::min(deltaTime, 0.1f));
	return {
		current.x + (target.x - current.x) * rate,
		current.y + (target.y - current.y) * rate,
		current.z + (target.z - current.z) * rate,
	};
}

Vector3 EnemyGroup::CalculateSlotOffset(uint32_t slotIndex, uint32_t memberCount) const {
	return CalculateSlotOffsetForTest(formationPattern_, slotIndex, memberCount, motion_.elapsedTime);
}

Vector3 EnemyGroup::CalculateSlotOffsetForTest(EnemyFormationPattern pattern, uint32_t slotIndex, uint32_t memberCount, float elapsedTime) const {
	const uint32_t safeCount = std::max(1u, memberCount);
	const float usableWidth = std::max(8.0f, (combatArea_.halfWidth - combatArea_.edgePadding) * 2.0f);
	const float spacing = std::min(std::max(1.0f, motion_.formationSpacing), usableWidth / std::max(1.0f, static_cast<float>(safeCount)));
	const float centeredIndex = static_cast<float>(slotIndex) - (static_cast<float>(safeCount) - 1.0f) * 0.5f;

	switch (pattern) {
	case EnemyFormationPattern::HorizontalLine:
		return {centeredIndex * spacing, 0.0f, 0.0f};
	case EnemyFormationPattern::VShape: {
		if (slotIndex == 0) {
			return {0.0f, 0.0f, 0.0f};
		}
		const uint32_t pairIndex = (slotIndex + 1u) / 2u;
		const float side = (slotIndex % 2u == 1u) ? -1.0f : 1.0f;
		return {side * spacing * static_cast<float>(pairIndex), -spacing * 0.35f * static_cast<float>(pairIndex), -spacing * 0.8f * static_cast<float>(pairIndex)};
	}
	case EnemyFormationPattern::Circle: {
		const float angle = elapsedTime * motion_.orbitAngularSpeed + 2.0f * kPi * static_cast<float>(slotIndex) / static_cast<float>(safeCount);
		const float radiusX = std::min(motion_.orbitRadiusX, std::max(1.0f, combatArea_.halfWidth - combatArea_.edgePadding));
		const float radiusY = std::min(motion_.orbitRadiusY, std::max(1.0f, combatArea_.halfHeight - combatArea_.edgePadding));
		return {std::cos(angle) * radiusX, std::sin(angle) * radiusY, 0.0f};
	}
	case EnemyFormationPattern::FigureEight: {
		const float angle = elapsedTime * motion_.orbitAngularSpeed + 2.0f * kPi * static_cast<float>(slotIndex) / static_cast<float>(safeCount);
		const float radiusX = std::min(motion_.orbitRadiusX, std::max(1.0f, combatArea_.halfWidth - combatArea_.edgePadding));
		const float radiusY = std::min(motion_.orbitRadiusY, std::max(1.0f, combatArea_.halfHeight - combatArea_.edgePadding));
		return {std::sin(angle) * radiusX, std::sin(angle * 2.0f) * radiusY, 0.0f};
	}
	case EnemyFormationPattern::Column:
		return {0.0f, centeredIndex * spacing * 0.4f, -static_cast<float>(slotIndex) * spacing};
	case EnemyFormationPattern::Count:
	default:
		return {centeredIndex * spacing, 0.0f, 0.0f};
	}
}

float EnemyGroup::GetEntryDistance() const {
	return LengthVector({
		motion_.combatCenter.x - motion_.entryPosition.x,
		motion_.combatCenter.y - motion_.entryPosition.y,
		motion_.combatCenter.z - motion_.entryPosition.z,
	});
}

Vector3 EnemyGroup::CalculateSmoothedPositionForTest(const Vector3 &current, const Vector3 &target, float sharpness, float deltaTime) const {
	return SmoothPosition(current, target, sharpness, deltaTime);
}

Vector3 EnemyGroup::ClampToCombatAreaForTest(const Vector3 &position, const Vector3 &playerPosition) const {
	return ClampToCombatArea(position, playerPosition);
}

bool EnemyGroup::ShouldSlotAttack(uint32_t slotIndex, uint32_t memberCount) const {
	return ShouldSlotAttackForTest(slotIndex, memberCount, motion_.phaseTime);
}

bool EnemyGroup::ShouldSlotAttackForTest(uint32_t slotIndex, uint32_t memberCount, float phaseTime) const {
	memberCount = std::max(1u, memberCount);
	if (attackPattern_ == EnemyGroupAttackPattern::None) {
		return false;
	}

	const float cycleTime = std::fmod(std::max(0.0f, phaseTime), EnemyFormationConstants::kAttackInterval);
	const float slotDelay = CalculateSlotDelay(slotIndex, memberCount);
	return cycleTime >= slotDelay && cycleTime < slotDelay + EnemyFormationConstants::kAttackWindow;
}

float EnemyGroup::CalculateSlotDelay(uint32_t slotIndex, uint32_t memberCount) const {
	switch (attackPattern_) {
	case EnemyGroupAttackPattern::Staggered:
		return static_cast<float>(slotIndex) * EnemyFormationConstants::kAttackSlotDelay;
	case EnemyGroupAttackPattern::LeaderThenWing:
		return slotIndex == 0 ? 0.0f : EnemyFormationConstants::kAttackSlotDelay * static_cast<float>((slotIndex + 1u) / 2u);
	case EnemyGroupAttackPattern::Alternating:
		return (slotIndex % 2u == 0u) ? 0.0f : EnemyFormationConstants::kAttackInterval * 0.5f;
	case EnemyGroupAttackPattern::None:
	default:
		memberCount;
		return EnemyFormationConstants::kAttackInterval;
	}
}

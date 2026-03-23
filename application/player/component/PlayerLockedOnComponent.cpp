/*********************************************************************
 * \file   PlayerLockedOnComponent.cpp
 * \brief  プレイヤーロックオン機能コンポーネント実装
 *
 * \author Harukichimaru
 * \date   February 2026
 *********************************************************************/
#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "PlayerLockedOnComponent.h"
#include "EnemyBase.h"
#include "EnemyManager.h"
#include "MagMath.h"
#include <algorithm>
#include <cmath>

//=============================================================================
// 初期化
void PlayerLockedOnComponent::Initialize(EnemyManager *enemyManager) {
	enemyManager_ = enemyManager;
	lockOnTargets_.clear();
	primaryLockOnTarget_ = nullptr;
	aimingTarget_ = nullptr;

	// デフォルト設定
	lockOnRange_ = 50.0f;   // 50メートル範囲
	lockOnFOV_ = 180.0f;    // 前方180度（後ろ以外ほぼ全周）でロック可能に拡大
	maxLockOnTargets_ = 3;  // 最大3敵同時ロック
	lockOnMode_ = false;
	lockOnAcquireTimer_ = 0.0f;
	lockOnAcquireInterval_ = 0.35f;
}

//=============================================================================
// ロックオン開始
void PlayerLockedOnComponent::BeginLockOn() {
	lockOnMode_ = true;
	lockOnAcquireTimer_ = 0.0f;
	aimingTarget_ = nullptr;
	lockOnTargets_.clear();
	primaryLockOnTarget_ = nullptr;
}

//=============================================================================
// ロックオン更新（長押し中の逐次追加）
void PlayerLockedOnComponent::UpdateLockOn(const Vector3 &playerPos, const Vector3 &playerForward, float deltaTime) {
	if (!lockOnMode_ || !enemyManager_) {
		return;
	}

	// 無効になったターゲットを除去
	lockOnTargets_.erase(
		std::remove_if(lockOnTargets_.begin(), lockOnTargets_.end(),
					   [&](EnemyBase *target) {
						   if (!target || !target->IsAlive() || !target->IsCollisionEnabled()) {
							   return true;
						   }
						   const Vector3 targetPos = target->GetPosition();
						   const Vector3 toEnemy = {
							   targetPos.x - playerPos.x,
							   targetPos.y - playerPos.y,
							   targetPos.z - playerPos.z};
						   const float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
						   return (distance > lockOnRange_) || !IsEnemyInFOV(playerPos, playerForward, targetPos);
					   }),
		lockOnTargets_.end());

	primaryLockOnTarget_ = lockOnTargets_.empty() ? nullptr : lockOnTargets_.front();

	// 照準中心にある候補を毎フレーム更新
	aimingTarget_ = FindBestTargetInReticle(playerPos, playerForward);

	if (static_cast<int>(lockOnTargets_.size()) >= maxLockOnTargets_) {
		return;
	}

	if (!aimingTarget_) {
		return;
	}

	lockOnAcquireTimer_ += deltaTime;
	if (lockOnAcquireTimer_ < lockOnAcquireInterval_) {
		return;
	}

	if (!IsAlreadyLocked(aimingTarget_)) {
		lockOnTargets_.push_back(aimingTarget_);
		if (!primaryLockOnTarget_) {
			primaryLockOnTarget_ = aimingTarget_;
		}
	}
	lockOnAcquireTimer_ = 0.0f;
}

//=============================================================================
// ロックオン終了
void PlayerLockedOnComponent::EndLockOn() {
	lockOnMode_ = false;
	aimingTarget_ = nullptr;
}

//=============================================================================
// 範囲内の敵を取得
std::vector<EnemyBase *> PlayerLockedOnComponent::GetEnemiesInRange(
	const Vector3 &playerPos,
	const Vector3 &playerForward) {

	std::vector<EnemyBase *> enemiesInRange;

	const auto &enemies = enemyManager_->GetEnemies();
	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive() || !enemy->IsCollisionEnabled()) {
			continue;
		}

		const Vector3 enemyPos = enemy->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z};

		// 距離計算
		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// ロックオン範囲外はスキップ
		if (distance > lockOnRange_) {
			continue;
		}

		// 視野角チェック
		if (!IsEnemyInFOV(playerPos, playerForward, enemyPos)) {
			continue;
		}

		// 重複チェック
		bool alreadyInList = false;
		for (const auto *target : enemiesInRange) {
			if (target == enemy.get()) {
				alreadyInList = true;
				break;
			}
		}
		if (!alreadyInList) {
			enemiesInRange.push_back(enemy.get());
		}
	}

	return enemiesInRange;
}

//=============================================================================
// 視野角チェック
bool PlayerLockedOnComponent::IsEnemyInFOV(
	const Vector3 &playerPos,
	const Vector3 &playerForward,
	const Vector3 &enemyPos) {

	const Vector3 toEnemy = {
		enemyPos.x - playerPos.x,
		enemyPos.y - playerPos.y,
		enemyPos.z - playerPos.z};

	float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

	// 距離0チェック（ゼロ除算防止）
	if (distance < 0.001f) {
		return true;
	}

	// 正規化
	float normalizedX = toEnemy.x / distance;
	float normalizedY = toEnemy.y / distance;
	float normalizedZ = toEnemy.z / distance;

	// 内積計算
	float dotProduct = normalizedX * playerForward.x +
					   normalizedY * playerForward.y +
					   normalizedZ * playerForward.z;

	// 視野角チェック
	float fovRadians = lockOnFOV_ * 0.5f * MagMath::PI / 180.0f;
	float angleRadians = std::acos(std::max(-1.0f, std::min(1.0f, dotProduct)));

	return angleRadians <= fovRadians;
}

//=============================================================================
// 最寄りの敵を取得
EnemyBase *PlayerLockedOnComponent::GetNearestEnemy(
	const Vector3 &playerPos,
	const Vector3 &playerForward) {

	EnemyBase *nearestEnemy = nullptr;
	float bestScore = -1.0f;

	const auto &enemies = enemyManager_->GetEnemies();
	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive() || !enemy->IsCollisionEnabled()) {
			continue;
		}

		const Vector3 enemyPos = enemy->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z};

		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// ロックオン範囲外はスキップ
		if (distance > lockOnRange_) {
			continue;
		}

		// 視野角チェック
		if (!IsEnemyInFOV(playerPos, playerForward, enemyPos)) {
			continue;
		}

		// スコア = 前方への角度（高いほど前方）- 距離ペナルティ
		float normalizedX = toEnemy.x / (distance + 0.001f);
		float normalizedY = toEnemy.y / (distance + 0.001f);
		float normalizedZ = toEnemy.z / (distance + 0.001f);

		float dotProduct = normalizedX * playerForward.x +
						   normalizedY * playerForward.y +
						   normalizedZ * playerForward.z;

		float score = dotProduct - (distance / lockOnRange_) * 0.3f;

		if (score > bestScore) {
			bestScore = score;
			nearestEnemy = enemy.get();
		}
	}

	return nearestEnemy;
}

//=============================================================================
// 照準中心に最も近い候補を取得（未ロック対象のみ）
EnemyBase *PlayerLockedOnComponent::FindBestTargetInReticle(
	const Vector3 &playerPos,
	const Vector3 &playerForward) {
	EnemyBase *bestTarget = nullptr;
	float bestScore = -10000.0f;

	const auto &enemies = enemyManager_->GetEnemies();
	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive() || !enemy->IsCollisionEnabled()) {
			continue;
		}

		EnemyBase *candidate = enemy.get();
		if (IsAlreadyLocked(candidate)) {
			continue;
		}

		const Vector3 enemyPos = candidate->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z};

		const float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
		if (distance > lockOnRange_ || distance < 0.001f) {
			continue;
		}

		if (!IsEnemyInFOV(playerPos, playerForward, enemyPos)) {
			continue;
		}

		const float invDist = 1.0f / distance;
		const float dirX = toEnemy.x * invDist;
		const float dirY = toEnemy.y * invDist;
		const float dirZ = toEnemy.z * invDist;
		const float dot = dirX * playerForward.x + dirY * playerForward.y + dirZ * playerForward.z;

		// 中心に近いほど高評価、距離が近いほどわずかに優遇
		const float score = dot * 10.0f - distance * 0.02f;
		if (score > bestScore) {
			bestScore = score;
			bestTarget = candidate;
		}
	}

	return bestTarget;
}

//=============================================================================
// 既にロック済みか判定
bool PlayerLockedOnComponent::IsAlreadyLocked(EnemyBase *target) const {
	if (!target) {
		return false;
	}
	return std::find(lockOnTargets_.begin(), lockOnTargets_.end(), target) != lockOnTargets_.end();
}

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

	// デフォルト設定
	lockOnRange_ = 50.0f;  // 50メートル範囲
	lockOnFOV_ = 90.0f;	   // 視野角90度（左右45度ずつ）
	maxLockOnTargets_ = 8; // 最大8敵同時ロック
	lockOnMode_ = false;
}

//=============================================================================
// ロックオン更新
void PlayerLockedOnComponent::Update(const Vector3 &playerPos, const Vector3 &playerForward) {
	if(!enemyManager_) {
		lockOnTargets_.clear();
		primaryLockOnTarget_ = nullptr;
		lockOnMode_ = false;
		return;
	}

	// 範囲内の敵を取得
	std::vector<EnemyBase *> enemiesInRange = GetEnemiesInRange(playerPos, playerForward);

	// ロック対象を範囲内の敵で更新（最大数まで）
	lockOnTargets_.clear();
	int lockCount = std::min((int)enemiesInRange.size(), maxLockOnTargets_);
	for(int i = 0; i < lockCount; ++i) {
		lockOnTargets_.push_back(enemiesInRange[i]);
	}

	// メインターゲットの更新
	if(!lockOnTargets_.empty()) {
		primaryLockOnTarget_ = lockOnTargets_[0];
		lockOnMode_ = true;
	} else {
		primaryLockOnTarget_ = nullptr;
		lockOnMode_ = false;
	}
}

//=============================================================================
// 範囲内の敵を取得
std::vector<EnemyBase *> PlayerLockedOnComponent::GetEnemiesInRange(
	const Vector3 &playerPos,
	const Vector3 &playerForward) {

	std::vector<EnemyBase *> enemiesInRange;

	const auto &enemies = enemyManager_->GetEnemies();
	for(const auto &enemy : enemies) {
		if(!enemy || !enemy->IsAlive()) {
			continue;
		}

		const Vector3 enemyPos = enemy->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z };

		// 距離計算
		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// ロックオン範囲外はスキップ
		if(distance > lockOnRange_) {
			continue;
		}

		// 視野角チェック
		if(!IsEnemyInFOV(playerPos, playerForward, enemyPos)) {
			continue;
		}

		// 重複チェック
		bool alreadyInList = false;
		for(const auto *target : enemiesInRange) {
			if(target == enemy.get()) {
				alreadyInList = true;
				break;
			}
		}
		if(!alreadyInList) {
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
		enemyPos.z - playerPos.z };

	float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

	// 距離0チェック（ゼロ除算防止）
	if(distance < 0.001f) {
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
	for(const auto &enemy : enemies) {
		if(!enemy || !enemy->IsAlive()) {
			continue;
		}

		const Vector3 enemyPos = enemy->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z };

		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// ロックオン範囲外はスキップ
		if(distance > lockOnRange_) {
			continue;
		}

		// 視野角チェック
		if(!IsEnemyInFOV(playerPos, playerForward, enemyPos)) {
			continue;
		}

		// スコア = 前方への角度（高いほど前方）- 距離ペナルティ
		float normalizedX = toEnemy.x / ( distance + 0.001f );
		float normalizedY = toEnemy.y / ( distance + 0.001f );
		float normalizedZ = toEnemy.z / ( distance + 0.001f );

		float dotProduct = normalizedX * playerForward.x +
			normalizedY * playerForward.y +
			normalizedZ * playerForward.z;

		float score = dotProduct - ( distance / lockOnRange_ ) * 0.3f;

		if(score > bestScore) {
			bestScore = score;
			nearestEnemy = enemy.get();
		}
	}

	return nearestEnemy;
}

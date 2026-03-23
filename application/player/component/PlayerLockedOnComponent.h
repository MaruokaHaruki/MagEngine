/*********************************************************************
 * \file   PlayerLockedOnComponent.h
 * \brief  プレイヤーロックオン機能コンポーネント
 *
 * \author Harukichimaru
 * \date   February 2026
 * \note   敵への自動ロックオン・追尾ターゲット管理
 *
 * 単一責任：敵のロックオン判定と複数ロック対象の管理
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Vector3.h"
#include <memory>
#include <vector>

// 前方宣言
class EnemyBase;
class EnemyManager;

///=============================================================================
///                    プレイヤーロックオン管理コンポーネント
class PlayerLockedOnComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 初期化
	/// @param enemyManager 敵管理マネージャー
	void Initialize(EnemyManager *enemyManager);

	/// @brief ロックオン開始（ミサイルボタン押下時）
	void BeginLockOn();

	/// @brief ロックオン更新（長押し中に呼び出し）
	/// @param playerPos プレイヤーの位置
	/// @param playerForward プレイヤーの前方ベクトル
	/// @param deltaTime フレーム時間
	void UpdateLockOn(const Vector3 &playerPos, const Vector3 &playerForward, float deltaTime);

	/// @brief ロックオン終了（ミサイルボタン解放時）
	void EndLockOn();

	///--------------------------------------------------------------
	///                        ロックオン状態取得
	/// @brief ロックオン状態確認
	bool HasLockOnTarget() const {
		return !lockOnTargets_.empty() && primaryLockOnTarget_ != nullptr;
	}

	/// @brief メインロックオン対象を取得
	EnemyBase *GetPrimaryTarget() const {
		return primaryLockOnTarget_;
	}

	/// @brief 全ロックオン対象を取得
	const std::vector<EnemyBase *> &GetAllTargets() const {
		return lockOnTargets_;
	}

	/// @brief 現在照準内にいる候補ターゲットを取得
	EnemyBase *GetAimingTarget() const {
		return aimingTarget_;
	}

	/// @brief ロックオン対象数を取得
	size_t GetTargetCount() const {
		return lockOnTargets_.size();
	}

	///--------------------------------------------------------------
	///                        ロックオン設定
	/// @brief ロックオン範囲を設定
	void SetLockOnRange(float range) {
		lockOnRange_ = range;
	}

	/// @brief ロックオン範囲を取得
	float GetLockOnRange() const {
		return lockOnRange_;
	}

	/// @brief ロックオン視野角を設定（度数法）
	void SetLockOnFOV(float fov) {
		lockOnFOV_ = fov;
	}

	/// @brief ロックオン視野角を取得
	float GetLockOnFOV() const {
		return lockOnFOV_;
	}

	/// @brief 最大ロック数を設定
	void SetMaxLockOnTargets(int maxTargets) {
		maxLockOnTargets_ = maxTargets;
	}

	/// @brief 最大ロック数を取得
	int GetMaxLockOnTargets() const {
		return maxLockOnTargets_;
	}

	/// @brief ロックオン状態をリセット
	void ClearLockOn() {
		lockOnTargets_.clear();
		primaryLockOnTarget_ = nullptr;
		aimingTarget_ = nullptr;
		lockOnAcquireTimer_ = 0.0f;
	}

	/// @brief ロックオンモード中か
	bool IsLockOnMode() const {
		return lockOnMode_;
	}

	/// @brief 敵マネージャーを設定
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	/// @brief 範囲内の敵を取得
	std::vector<EnemyBase *> GetEnemiesInRange(const Vector3 &playerPos, const Vector3 &playerForward);

	/// @brief 視野角チェック（敵が視野内か判定）
	bool IsEnemyInFOV(const Vector3 &playerPos, const Vector3 &playerForward, const Vector3 &enemyPos);

	/// @brief 最寄りの敵を取得
	EnemyBase *GetNearestEnemy(const Vector3 &playerPos, const Vector3 &playerForward);

	/// @brief 照準中心に最も近い候補を取得（未ロック対象のみ）
	EnemyBase *FindBestTargetInReticle(const Vector3 &playerPos, const Vector3 &playerForward);

	/// @brief 既にロック済みか判定
	bool IsAlreadyLocked(EnemyBase *target) const;

	///--------------------------------------------------------------
	///                        メンバ変数
	EnemyManager *enemyManager_;			 // 敵管理マネージャーへの参照
	std::vector<EnemyBase *> lockOnTargets_; // ロックオン対象敵のリスト
	EnemyBase *primaryLockOnTarget_;		 // メインのロックオン対象

	float lockOnRange_;			  // ロックオン探索範囲（メートル）
	float lockOnFOV_;			  // ロックオン視野角（度数法）
	int maxLockOnTargets_;		  // 同時ロック可能な最大敵数
	bool lockOnMode_;			  // ロックオン中フラグ
	float lockOnAcquireTimer_;	  // 次のロック取得までの経過時間
	float lockOnAcquireInterval_; // 1体ずつロックする間隔（秒）
	EnemyBase *aimingTarget_;	  // 現在照準中心にいる候補ターゲット
};

/*********************************************************************
 * \file   IAIBehavior.h
 * \brief  AI戦略基底インターフェース
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include <string>

// 前方宣言
class Enemy;

/**
 * @brief AI戦略の基底インターフェース
 *
 * すべてのAI行動パターンがこのインターフェースを実装します。
 */
class IAIBehavior {
public:
	virtual ~IAIBehavior() = default;

	/**
	 * @brief 毎フレーム実行される戦略処理
	 * @param deltaTime フレーム経過時間（秒）
	 * @param enemy このAI の対象敵
	 * @param playerPos プレイヤーの現在位置
	 */
	virtual void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) = 0;

	/**
	 * @brief 戦略名を取得
	 */
	virtual std::string GetBehaviorName() const = 0;

	/**
	 * @brief このAIを初期化
	 */
	virtual void Initialize() {}

	/**
	 * @brief このAIをリセット
	 */
	virtual void Reset() {}

protected:
	/// 戦略の実行時間カウンター（継承先で使用可能）
	float elapsedTime_ = 0.0f;
};

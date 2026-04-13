/*********************************************************************
 * \file   HealthComponent.h
 * \brief  HP管理・ダメージ処理コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include <functional>
#include <algorithm>

/**
 * @brief HP管理とダメージ処理を担当するコンポーネント
 *
 * 責務：
 * - HP の初期化と現在値管理
 * - ダメージ受付と HP 減少処理
 * - ダメージクールダウン管理（連続ダメージ防止）
 * - 死亡判定とコールバック実行
 */
class HealthComponent : public IEnemyComponent {
public:
	virtual ~HealthComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - maxHP (int): 最大HP
	 *   - damageCooldown (float): ダメージを受けた後のクールダウン時間（秒）
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief 毎フレーム更新（ダメージクールダウン減少）
	 */
	void Update(float deltaTime) override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "HealthComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief ダメージを受ける
	 * @param damage ダメージ量
	 * @return クールダウン中でない場合true、クールダウン中はfalse
	 */
	bool TakeDamage(int damage);

	/**
	 * @brief 現在のHPを取得
	 */
	int GetCurrentHP() const { return currentHP_; }

	/**
	 * @brief 最大HPを取得
	 */
	int GetMaxHP() const { return maxHP_; }

	/**
	 * @brief HP を満タンに回復
	 */
	void RestoreHealth(int amount);

	/**
	 * @brief 生存判定
	 */
	bool IsAlive() const { return currentHP_ > 0; }

	/**
	 * @brief ダメージクールダウン中かどうか
	 */
	bool IsInDamageCooldown() const { return damageCooldownTimer_ > 0.0f; }

	/**
	 * @brief 死亡コールバックを設定
	 */
	void SetOnDeadCallback(std::function<void()> callback) {
		onDeadCallback_ = callback;
	}

private:
	//========================================
	// メンバ変数
	//========================================
	int maxHP_;                 ///< 最大HP
	int currentHP_;             ///< 現在のHP
	float damageCooldown_;      ///< ダメージクールダウン時間
	float damageCooldownTimer_; ///< ダメージクールダウン タイマー

	std::function<void()> onDeadCallback_; ///< 死亡時コールバック
};

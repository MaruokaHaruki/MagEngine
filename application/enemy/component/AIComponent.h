/*********************************************************************
 * \file   AIComponent.h
 * \brief  AI戦略実行管理コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include "../../../engine/math/MagMath.h"
#include <memory>

using namespace MagMath;

// 前方宣言
class IAIBehavior;

/**
 * @brief AI戦略の実行を管理するコンポーネント
 *
 * 責務：
 * - 複数のAI戦略（Behavior）の保有と切り替え
 * - 各フレームでアクティブな戦略の Update を呼び出し
 * - 戦略の動的変更
 */
class AIComponent : public IEnemyComponent {
public:
	virtual ~AIComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - strategy (string): 初期戦略名
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief 毎フレーム更新（現在の戦略を実行）
	 */
	void Update(float deltaTime) override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "AIComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief 戦略を設定
	 */
	void SetBehavior(std::unique_ptr<IAIBehavior> behavior);

	/**
	 * @brief 現在の戦略を取得
	 */
	IAIBehavior* GetBehavior() const { return behavior_.get(); }

	/**
	 * @brief プレイヤー位置を設定（戦略の実行に使用）
	 */
	void SetPlayerPosition(const Vector3& playerPos);

	/**
	 * @brief プレイヤー位置を取得
	 */
	const Vector3& GetPlayerPosition() const { return playerPosition_; }

private:
	//========================================
	// メンバ変数
	//========================================
	std::unique_ptr<IAIBehavior> behavior_; ///< アクティブなAI戦略
	Vector3 playerPosition_;                 ///< プレイヤー位置（戦略用）
};

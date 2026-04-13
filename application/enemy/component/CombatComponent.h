/*********************************************************************
 * \file   CombatComponent.h
 * \brief  射撃・攻撃制御コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include <string>

// forward decl
namespace MagMath {
	struct Vector3;
}

/**
 * @brief 敵の射撃・攻撃を管理するコンポーネント
 *
 * 責務：
 * - 射撃間隔の管理
 * - 射撃可能判定
 * - 敵弾の生成・発射
 */
class CombatComponent : public IEnemyComponent {
public:
	virtual ~CombatComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - shootInterval (float): 射撃間隔（秒）
	 *   - bulletType (string): 敵弾タイプ
	 *   - shootDistance (float): 射撃可能距離
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief 毎フレーム更新（クールダウン減少）
	 */
	void Update(float deltaTime) override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "CombatComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief 指定方向に射撃
	 * @param direction 射撃方向
	 * @return 射撃成功したかどうか
	 */
	bool Fire(const MagMath::Vector3& direction);

	/**
	 * @brief 射撃可能かどうか
	 */
	bool CanFire() const { return shootCooldownTimer_ <= 0.0f; }

	/**
	 * @brief 射撃距離内かどうかを判定
	 */
	bool IsInShootDistance(float distance) const;

	/**
	 * @brief 射撃間隔を設定
	 */
	void SetShootInterval(float interval) { shootInterval_ = interval; }

private:
	//========================================
	// メンバ変数
	//========================================
	float shootInterval_;       ///< 射撃間隔（秒）
	float shootCooldownTimer_;  ///< 射撃クールダウン タイマー
	std::string bulletType_;    ///< 敵弾タイプ
	float shootDistance_;       ///< 射撃可能距離
};

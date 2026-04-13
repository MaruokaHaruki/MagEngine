/*********************************************************************
 * \file   VisualComponent.h
 * \brief  描画・アニメーション管理コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include <string>
#include <memory>

// 前方宣言
namespace MagEngine {
	class Object3d;
	class Object3dSetup;
}

/**
 * @brief 敵の描画・アニメーションを管理するコンポーネント
 *
 * 責務：
 * - Object3dの所有と管理
 * - 描画処理（Draw()）
 * - アニメーション状態の管理
 * - モデルの初期化
 */
class VisualComponent : public IEnemyComponent {
public:
	virtual ~VisualComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - modelPath (string): モデルファイルパス
	 *   - scale (float): スケール
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief 描画処理
	 */
	void Draw() override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "VisualComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief Object3dを取得
	 */
	MagEngine::Object3d* GetObject3d() const { return object3d_.get(); }

	/**
	 * @brief Object3dSetupを設定（初期化時に使用）
	 */
	void SetObject3dSetup(MagEngine::Object3dSetup* setup) { object3dSetup_ = setup; }

	/**
	 * @brief アニメーションを再生
	 */
	void PlayAnimation(const std::string& name);

	/**
	 * @brief 現在のアニメーション名を取得
	 */
	const std::string& GetCurrentAnimation() const { return currentAnimation_; }

private:
	//========================================
	// メンバ変数
	//========================================
	std::unique_ptr<MagEngine::Object3d> object3d_; ///< 3Dオブジェクト
	MagEngine::Object3dSetup* object3dSetup_;       ///< Object3dSetup（所有しない）
	std::string modelPath_;                         ///< モデルファイルパス
	float scale_;                                   ///< スケール
	std::string currentAnimation_;                  ///< 現在再生中のアニメーション
};

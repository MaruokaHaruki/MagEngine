/*********************************************************************
 * \file   ClearScene.h
 * \brief  クリアシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "BaseScene.h"

// Forward declaration
class SceneContext;
namespace MagEngine {
	struct EngineContext;
}

///=============================================================================
///                         クリアシーンクラス
class ClearScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) override;

	/// \brief 終了処理
	void Finalize() override;

	/// \brief 更新
	void Update(float deltaTime) override;

	/// \brief 描画対象登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld) override;

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// EngineContext
	const MagEngine::EngineContext *engineContext_ = nullptr;
	SceneContext *sceneContext_ = nullptr;
};

/*********************************************************************
 * \file   EngineApp.cpp
 * \brief  エンジンの接続点から現行ゲームを実行する
 *********************************************************************/
#include "EngineApp.h"

namespace MagEngine {
	///=============================================================================
	/// 作品の初期化
	void EngineApp::InitializeApplication() {
		if (launchOptions_.editor || !launchOptions_.scene.empty()) {
			editorHost_ = std::make_unique<EditorHost>();
			editorHost_->Initialize(engineContext_, launchOptions_);
			win_->SetCloseRequestHandler([this] { return editorHost_->CanClose(); });
			return;
		}
		//========================================
		// 作品用の文字と演出
		textRenderer_->Initialize(*spriteSetup_, "font_atlas.png", "resources/config/ui/font_atlas.json");
		trailEffectManager_->LoadAllPresetsFromJson("resources/trail/test_preset.json");
		trailEffectManager_->CreateFromPreset("test_trail", "test_trail");

		//========================================
		// Sceneの構築
		sceneFactory_ = std::make_unique<SceneFactory>();
		sceneManager_ = std::make_unique<SceneManager>();
		sceneManager_->SetSceneFactory(sceneFactory_.get());
		sceneManager_->Initialize(engineContext_);
	}

	///=============================================================================
	/// ゲームの更新
	void EngineApp::UpdateApplication(const FrameTime &frameTime) {
		if (editorHost_) { editorHost_->Update(frameTime); return; }
		sceneManager_->Update(frameTime);
	}

	///=============================================================================
	/// 描画対象の登録
	void EngineApp::RegisterApplicationRenderables(RenderWorld &renderWorld) {
		if (editorHost_) { editorHost_->RegisterRenderables(renderWorld); return; }
		sceneManager_->RegisterRenderables(renderWorld);
	}

	///=============================================================================
	/// ゲームの終了
	void EngineApp::FinalizeApplication() {
		if (editorHost_) { win_->SetCloseRequestHandler({}); editorHost_->Finalize(); editorHost_.reset(); }
		// NOTE: 部分初期化時も呼ばれるため、生成済みの所有物だけを解除する。
		if (sceneManager_) {
			sceneManager_->Finalize();
			sceneManager_.reset();
		}
		sceneFactory_.reset();
	}
}

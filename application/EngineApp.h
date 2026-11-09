/*********************************************************************
 * \file   EngineApp.h
 * \brief  現行ゲームの起動とSceneの所有を担当する
 *********************************************************************/
#pragma once
#include "MagFramework.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "application/editor/EditorHost.h"

namespace MagEngine {
	///=============================================================================
	/// 現行ゲームアプリケーション
	class EngineApp : public MagFramework {
	public:
		/// @brief 起動時の編集指定。未指定なら従来のゲームを起動する
		explicit EngineApp(EditorLaunchOptions options = {}) : launchOptions_(std::move(options)) {}
	protected:
		///--------------------------------------------------------------
		/// 初期化・更新・描画登録・終了
		/// @brief 作品用の設定と初期Sceneを構築する
		void InitializeApplication() override;
		/// @brief 検証済みの時間（秒）を現行Sceneへ渡す
		void UpdateApplication(const FrameTime &frameTime) override;
		/// @brief 現行Sceneの非所有描画参照を登録する
		void RegisterApplicationRenderables(RenderWorld &renderWorld) override;
		/// @brief Sceneと作品用UIをEngineより先に解除する
		void FinalizeApplication() override;

	private:
		///--------------------------------------------------------------
		/// ゲームが所有するScene管理
		std::unique_ptr<SceneFactory> sceneFactory_;
		std::unique_ptr<SceneManager> sceneManager_;
		EditorLaunchOptions launchOptions_;
		std::unique_ptr<EditorHost> editorHost_;
	};
}

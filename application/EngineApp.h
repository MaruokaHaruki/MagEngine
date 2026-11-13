/*********************************************************************
 * \file   EngineApp.h
 * \brief  制作画面と最小実行アプリの起動を担当する
 *********************************************************************/
#pragma once
#include "MagFramework.h"


#include "application/editor/EditorHost.h"

namespace MagEngine {
	///=============================================================================
	/// エンジン起動アプリケーション
	class EngineApp : public MagFramework {
	public:
		/// @brief 起動時の編集指定。既定値はエントリーポイントで解決する
		explicit EngineApp(EditorLaunchOptions options = {}) : launchOptions_(std::move(options)) {}
	protected:
		///--------------------------------------------------------------
		/// 初期化・更新・描画登録・終了
		/// @brief 編集ホストを構築する
		void InitializeApplication() override;
		/// @brief 検証済みの時間（秒）を編集ホストへ渡す
		void UpdateApplication(const FrameTime &frameTime) override;
		/// @brief 編集文書の描画参照を登録する
		void RegisterApplicationRenderables(RenderWorld &renderWorld) override;
		/// @brief 編集ホストをEngineより先に解除する
		void FinalizeApplication() override;

	private:
		///--------------------------------------------------------------
		/// 起動指定と編集ホスト
		EditorLaunchOptions launchOptions_;
		std::unique_ptr<EditorHost> editorHost_;
	};
}

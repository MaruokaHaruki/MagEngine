/*********************************************************************
 * \file   EngineApp.cpp
 * \brief  エンジンアプリケーションの具体実装クラスの実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   ゲーム固有の初期化・更新・描画処理を実装
 *********************************************************************/
#include "EngineApp.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void EngineApp::Initialize() {
		///--------------------------------------------------------------
		///						 フレームワーク初期化
		MagFramework::Initialize();
	}

	///=============================================================================
	///						終了処理
	void EngineApp::Finalize() {
		// フレームワークの終了処理
		MagFramework::Finalize();
	}

	///=============================================================================
	///						更新
	void EngineApp::Update() {
		//========================================
		// 更新処理
		MagFramework::Update();
	}

	///=============================================================================
	///						描画
	void EngineApp::Draw() {
		//========================================
		// レンダーテクスチャ前処理
		MagFramework::RenderPreDraw();

		//========================================
		// Sceneフェーズ描画（Skybox/Opaque/Cloud/Trail）
		MagFramework::OpaqueRender();

		//========================================
		// Overlayフェーズ描画（Sprite）
		MagFramework::ExecuteRenderPhase(RenderPhase::Overlay);

		//========================================
		// Overlay後フェーズ描画（Particleなど）
		MagFramework::ExecuteRenderPhase(RenderPhase::PostOverlay);

		//========================================
		// レンダーテクスチャ後処理
		MagFramework::RenderPostDraw();

		//========================================
		// PostProcessフェーズ描画（SceneColorからPresentColorへ合成）
		MagFramework::ExecuteRenderPhase(RenderPhase::PostProcess);

		MagFramework::ImGuiPreDraw();
		// ↓この間に書け!!!
		// ↑
		MagFramework::ImGuiPostDraw();

		//========================================
		// 描画後処理
		MagFramework::PostDraw();
	}
}

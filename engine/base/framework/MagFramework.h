/*********************************************************************
 * \file   MagFramework.h
 * \brief  エンジンフレームワークの基本クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   ゲームループと初期化・終了処理を統合管理
 *********************************************************************/
#pragma once
#include <chrono>
//========================================
// COMMENT: リリースビルド時に ImGui を完全に除外する
#ifdef _DEBUG
#define ENABLE_IMGUI 1
#else
#define ENABLE_IMGUI 0
#endif

//========================================
// Framework
#include "Camera.h"
#include "DirectXCore.h"
#include "EngineContext.h"
#if ENABLE_IMGUI
#include "EditorUiSystem.h"
#include "ImguiSetup.h"
#endif
#include "Input.h"
#include "SrvSetup.h"
#include "WinApp.h"
// Manager
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "engine/graphics/text/TextRenderer.h"
#include "LightManager.h"
#include "LineManager.h"
#include "MAudioG.h"
#include "ModelManager.h"
#include "PostEffectManager.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "TextureManager.h"
// Setup
#include "CloudSetup.h"
#include "Object3dSetup.h"
#include "ParticleSetup.h"
#include "SkyboxSetup.h"
#include "SpriteSetup.h"
#include "TrailEffectManager.h"
#include "TrailEffectSetup.h"
#include "engine/render/Renderer.h"
#include "engine/render/pass/RenderWorld.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						FrameWorkクラス
	class MagFramework {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		/// \brief 仮想デストラクタ
		virtual ~MagFramework() = default;
		/// \brief メインループ
		void Run();
		/// \brief 初期化
		virtual void Initialize();
		/// \brief 更新
		virtual void Update();
		/// \brief 描画
		virtual void Draw() = 0;
		/// \brief 終了処理
		virtual void Finalize();
		/// @brief ポストエフェクトのImGui描画
		void DrawPostEffectImGui();

	private:
		/// @brief EngineContextへ既存サービスの非所有参照を設定
		void InitializeEngineContext();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	public:
		/// @brief レンダーテクスチャ前処理
		void RenderPreDraw();
		/// @biref レンダーテクスチャ後処理
		void RenderPostDraw();
		/// @brief フレームワーク共通後処理
		void PostDraw();
		/// @brief ImGuiの更新前処理
		void ImGuiPreDraw();
		/// @brief ImGuiの更新後処理
		void ImGuiPostDraw();
		/// @brief Sceneフェーズ描画
		void OpaqueRender();
		/// @brief 指定フェーズのRenderPass描画
		void ExecuteRenderPhase(RenderPhase phase);

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/// \brief 終了リクエストの取得
		virtual bool IsEndRequest() const {
			return isEndRequest_;
		}

		/// \brief トレイルエフェクトマネージャーの取得
		TrailEffectManager *GetTrailEffectManager() const {
			return trailEffectManager_.get();
		}

		///--------------------------------------------------------------
		///							メンバ変数
	protected:
		//========================================
		// ゲーム終了フラグ
		bool isEndRequest_ = false;
		//========================================
		// ウィンドウクラス
		std::unique_ptr<WinApp> win_;
		//========================================
		// ダイレクトX
		std::unique_ptr<DirectXCore> dxCore_;
		//========================================
		// ImGui
		/// COMMENT: リリースビルド（NDEBUG）では ImGui を除外してメモリ節約
#if ENABLE_IMGUI
		std::unique_ptr<ImguiSetup> imguiSetup_;
		std::unique_ptr<EditorUiSystem> editorUiSystem_;
#endif
		//========================================
		// SrvSetup
		std::unique_ptr<SrvSetup> srvSetup_;
		std::unique_ptr<TextRenderer> textRenderer_;
		//========================================
		// 共通部
		// スプライトセットアップ
		std::unique_ptr<SpriteSetup> spriteSetup_;
		// パーティクルセットアップ
		std::unique_ptr<ParticleSetup> particleSetup_;
		// 3Dオブジェクセットアップ
		std::unique_ptr<Object3dSetup> object3dSetup_;
		// モデルセットアップ
		std::unique_ptr<ModelSetup> modelSetup_;
		// Skyboxセットアップ
		std::unique_ptr<SkyboxSetup> skyboxSetup_;
		// Cloudセットアップ
		std::unique_ptr<CloudSetup> cloudSetup_;
		// TrailEffectセットアップ
		std::unique_ptr<TrailEffectSetup> trailEffectSetup_;
		//========================================
		// マネージャ
		// EngineContext - NOTE: 実体はFrameworkが保持し、Sceneへは非所有参照として渡す
		EngineContext engineContext_;
		//========================================
		// マネージャ
		// カメラマネージャ
		std::unique_ptr<CameraManager> cameraManager_;
		// ラインマネージャ
		std::unique_ptr<LineManager> lineManager_;
		// テクスチャマネージャ
		std::unique_ptr<TextureManager> textureManager_;
		//========================================
		// マネージャ
		// ポストエフェクトマネージャ
		std::unique_ptr<PostEffectManager> postEffectManager_;
		// シーンマネージャ
		std::unique_ptr<SceneManager> sceneManager_;
		// シーンファクトリー
		std::unique_ptr<SceneFactory> sceneFactory_;
		// ライトマネージャ
		std::unique_ptr<LightManager> lightManager_;
		// トレイルエフェクトマネージャ
		std::unique_ptr<TrailEffectManager> trailEffectManager_;
		//========================================
		// レンダラー
		Renderer renderer_;
		RenderWorld renderWorld_;
		std::chrono::steady_clock::time_point previousFrameTime_{};
		bool hasPreviousFrameTime_ = false;
	};

}

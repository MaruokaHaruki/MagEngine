/*********************************************************************
 * \file   MagFramework.cpp
 * \brief  エンジンフレームワークの基本クラスの実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   ゲームループと初期化・終了処理の実装
 *********************************************************************/
#include "MagFramework.h"
#include "WinApp.h"
#include "engine/render/RenderContext.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						実行
	void MagFramework::Run() {
		//========================================
		// 初期化
		Initialize();
		//========================================
		// メインループ
		MSG msg{};
		// メッセージがなくなるまでループ
		while (msg.message != WM_QUIT) {
			// メッセージがあれば処理
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				// メッセージ処理
				TranslateMessage(&msg);
				// メッセージ処理
				DispatchMessage(&msg);
			} else {
				//---------------------------------------
				// 更新
				Update();
				//---------------------------------------
				// 終了リクエストがあれば終了
				if (IsEndRequest()) {
					break;
				}
				//---------------------------------------
				// 描画
				Draw();
			}
		}
		//========================================
		// 終了処理
		Finalize();
	}

	///=============================================================================
	///						初期化
	void MagFramework::Initialize() {
		///--------------------------------------------------------------
		///						 ウィンドウ生成
		win_ = std::make_unique<WinApp>();
		// ウィンドウの生成
		win_->CreateGameWindow(L"MagEngine_Ver1.4.6");

		///--------------------------------------------------------------
		///						 ダイレクトX生成
		dxCore_ = std::make_unique<DirectXCore>();
		// ダイレクトXの初期化
		dxCore_->InitializeDirectX(win_.get());

		///--------------------------------------------------------------
		///						 ポストエフェクトマネージャ
		postEffectManager_ = std::make_unique<PostEffectManager>();
		postEffectManager_->Initialize(dxCore_.get());

		///--------------------------------------------------------------
		///						 ImGuiのセットアップ
		/// COMMENT: デバッグビルドのみ ImGui を初期化してメモリ削減
#if ENABLE_IMGUI
		imguiSetup_ = std::make_unique<ImguiSetup>();
		// ImGuiの初期化
		imguiSetup_->Initialize(win_.get(), dxCore_.get(), Style::EDITOR);
#endif

		///--------------------------------------------------------------
		/// 					 カメラの初期化
		cameraManager_ = std::make_unique<CameraManager>();
		cameraManager_->Initialize();

		///--------------------------------------------------------------
		///                        デバックテキストマネージャ
		DebugTextManager::GetInstance()->Initialize(win_.get());
		// デバッグテキストマネージャの初期化（カメラ初期化後に設定）
		DebugTextManager::GetInstance()->SetCamera(cameraManager_->GetCurrentCamera());
		// デバッグテキストの表示を有効にする
		DebugTextManager::GetInstance()->SetDebugTextEnabled(true);

		// 初期の永続的なデバッグテキストを設定
		DebugTextManager::GetInstance()->AddAxisLabels(); // 座標軸ラベル

		///--------------------------------------------------------------
		///						 SrvSetupクラス
		srvSetup_ = std::make_unique<SrvSetup>();
		// SrvSetupの初期化
		srvSetup_->Initialize(dxCore_.get());

		///--------------------------------------------------------------
		///						 入力クラス
		// 入力の初期化
		Input::GetInstance()->Initialize(win_->GetWindowClass().hInstance, win_->GetWindowHandle());

		///--------------------------------------------------------------
		// 						 テクスチャマネージャ
		textureManager_ = std::make_unique<TextureManager>();
		textureManager_->Initialize(dxCore_.get(), "resources/texture/", srvSetup_.get());

		///--------------------------------------------------------------
		///						 ライトマネージャ
		lightManager_ = std::make_unique<LightManager>();
		// ライトマネージャの初期化
		lightManager_->Initialize();

		///--------------------------------------------------------------
		///						 スプライトクラス
		//========================================
		// スプライト共通部
		spriteSetup_ = std::make_unique<SpriteSetup>();
		// スプライト共通部の初期化
		spriteSetup_->Initialize(dxCore_.get(), *textureManager_);

		///--------------------------------------------------------------
		///						 Object3D共通部
		//========================================
		// モデルマネージャの初期化
		ModelManager::GetInstance()->Initialize(dxCore_.get(), *textureManager_);
		//========================================
		// 3Dオブジェクト共通部
		object3dSetup_ = std::make_unique<Object3dSetup>();
		// 3Dオブジェクト共通部の初期化
		object3dSetup_->Initialize(dxCore_.get());
		// Object3Dのカメラ設定
		object3dSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// Object3Dのライトマネージャ設定
		object3dSetup_->SetLightManager(lightManager_.get());
		// NOTE: RenderPassはObject3dSetupの描画状態を再利用し、旧描画順だけを置き換える
		renderer_.Initialize(*object3dSetup_);

		///--------------------------------------------------------------
		///						 Skybox共通部
		skyboxSetup_ = std::make_unique<SkyboxSetup>();
		//  Skyboxの初期化
		skyboxSetup_->Initialize(dxCore_.get(), *textureManager_);
		//  Skyboxのカメラ設定
		skyboxSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// Skyboxのライトマネージャ設定
		skyboxSetup_->SetLightManager(lightManager_.get());

		///--------------------------------------------------------------
		///						 パーティクル共通部
		particleSetup_ = std::make_unique<ParticleSetup>();
		// パーティクルセットアップの初期化
		particleSetup_->Initialize(dxCore_.get(), srvSetup_.get(), *textureManager_);
		// パーティクルのカメラ設定
		particleSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());

		///--------------------------------------------------------------
		///						 クラウド共通部
		cloudSetup_ = std::make_unique<CloudSetup>();
		// クラウドセットアップの初期化
		cloudSetup_->Initialize(dxCore_.get());
		// CloudのライトマネージャSetup
		cloudSetup_->SetLightManager(lightManager_.get());

		///--------------------------------------------------------------
		///						 トレイルエフェクト共通部
		trailEffectSetup_ = std::make_unique<TrailEffectSetup>();
		// トレイルエフェクトセットアップの初期化
		trailEffectSetup_->Initialize(dxCore_.get());
		// トレイルエフェクトセットアップにSrvSetupを設定
		trailEffectSetup_->SetSrvSetup(srvSetup_.get());

		///--------------------------------------------------------------
		///						 トレイルエフェクトマネージャ
		trailEffectManager_ = std::make_unique<TrailEffectManager>();
		// トレイルエフェクトマネージャの初期化
		trailEffectManager_->Initialize(trailEffectSetup_.get());
		// トレイルプリセットJSONを一度だけ読み込む（シーン内では読み込まない）
		trailEffectManager_->LoadAllPresetsFromJson("resources/trail/test_preset.json");
		// テスト用インスタンスを作成
		trailEffectManager_->CreateFromPreset("test_trail", "test_trail");

		///--------------------------------------------------------------
		///						 ラインマネージャ
		lineManager_ = std::make_unique<LineManager>();
		lineManager_->Initialize(dxCore_.get(), srvSetup_.get());
		// Lineのカメラ設定
		lineManager_->SetDefaultCamera(cameraManager_->GetCurrentCamera());

		///--------------------------------------------------------------
		///						 オーディオの初期化
		MAudioG::GetInstance()->Initialize("resources/sound/");

		///--------------------------------------------------------------
		///						 EngineContextの構築
		InitializeEngineContext();

		///--------------------------------------------------------------
		///						 シーンマネージャ
		sceneManager_ = std::make_unique<SceneManager>();
		// シーンファクトリーのセット
		sceneFactory_ = std::make_unique<SceneFactory>();
		sceneManager_->SetSceneFactory(sceneFactory_.get());
		// シーンマネージャの初期化
		sceneManager_->Initialize(engineContext_);

		///--------------------------------------------------------------
		///						 各種設定
		// ライトマネージャへラインマネージャポインタの受け渡し
		lightManager_->SetLineManager(lineManager_.get());

		///--------------------------------------------------------------
		///						 エディターレイアウトの初期化
		editorLayout_ = std::make_unique<EditorLayout>();
		/// COMMENT: ImGui 除外時はエディターレイアウト初期化をスキップ
#if ENABLE_IMGUI
		editorLayout_->Initialize(dxCore_.get(), postEffectManager_.get(), imguiSetup_.get());

		///--------------------------------------------------------------
		///					 Game Viewport にレンダーテクスチャを設定
		// レンダーテクスチャリソースを取得
		auto renderTextureResource = dxCore_->GetRenderTextureResource(dxCore_->GetRenderResourceIndex());
		if (renderTextureResource.Get()) {
			// ImGui用のテクスチャハンドルを取得
			ImTextureID textureHandle = imguiSetup_->RegisterTextureForImGui(renderTextureResource.Get());
			// GameViewportPanelに設定
			if (editorLayout_->GetViewportPanel()) {
				editorLayout_->GetViewportPanel()->SetRenderTextureHandle((void *)textureHandle);
			}
		}
#endif // ENABLE_IMGUI
	}

	///=============================================================================
	///						EngineContextの構築
	void MagFramework::InitializeEngineContext() {
		// 処理内容：既存SingletonとFramework所有オブジェクトをContextへ集約する
		// 理由：所有権を変更せず、Scene側のSingleton直接参照を段階的に減らすため
		engineContext_.input = Input::GetInstance();
		engineContext_.cameraManager = cameraManager_.get();
		engineContext_.textureManager = textureManager_.get();
		engineContext_.modelManager = ModelManager::GetInstance();
		engineContext_.audio = MAudioG::GetInstance();
		engineContext_.graphics = dxCore_.get();
		engineContext_.spriteSetup = spriteSetup_.get();
		engineContext_.object3dSetup = object3dSetup_.get();
		engineContext_.particleSetup = particleSetup_.get();
		engineContext_.skyboxSetup = skyboxSetup_.get();
		engineContext_.cloudSetup = cloudSetup_.get();
		engineContext_.trailEffectSetup = trailEffectSetup_.get();
		engineContext_.trailEffectManager = trailEffectManager_.get();
		engineContext_.debugTextManager = DebugTextManager::GetInstance();
		engineContext_.lineManager = lineManager_.get();
		engineContext_.Validate();
	}

	///=============================================================================
	///						更新
	void MagFramework::Update() {
		//========================================
		// デバックカメラの呼び出し1,2
		if (Input::GetInstance()->PushKey(DIK_1)) {
			cameraManager_->SetCurrentCamera("DebugCamera");
		}
		if (Input::GetInstance()->PushKey(DIK_2)) {
			cameraManager_->SetCurrentCamera("DefaultCamera");
		}

		//========================================
		// カメラの更新
		cameraManager_->UpdateAll(*lineManager_);

		//========================================
		// デバックテキストの更新（カメラ更新後に実行）
		DebugTextManager::GetInstance()->SetCamera(cameraManager_->GetCurrentCamera());
		DebugTextManager::GetInstance()->Update();

		//========================================
		// ラインの更新
		// カメラの更新
		lineManager_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// ラインの更新
		lineManager_->Update();

		//=========================================
		// ライトの可視化
		lightManager_->Update();

		//========================================
		// Object3Dのカメラ設定の更新
		object3dSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// particleのカメラ設定の更新
		particleSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// skyboxのカメラ設定の更新
		skyboxSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());
		// TrailEffectのカメラ設定の更新
		trailEffectSetup_->SetDefaultCamera(cameraManager_->GetCurrentCamera());

		//========================================
		// トレイルエフェクトマネージャの更新
		trailEffectManager_->Update(1.0f / 60.0f); // 固定フレームレート

		//========================================
		// インプットの更新
		Input::GetInstance()->Update();

		//========================================
		// シーンマネージャの更新
		sceneManager_->Update();
	}

	///=============================================================================
	///						終了処理
	void MagFramework::Finalize() {
		// GPUが参照中のリソースを各マネージャが解放しないよう、破棄前に同期する
		if (dxCore_) {
			dxCore_->WaitForGpuIdle();
		}
		//========================================
		// シーンの終了処理
		if (sceneManager_) {
			sceneManager_->Finalize();
			sceneManager_.reset();
		}
		sceneFactory_.reset();
		//========================================
		// エディターレイアウトの終了処理
		if (editorLayout_) {
			editorLayout_->Finalize();
			editorLayout_.reset();
		}
		//========================================
		// COMMENT: ImGui 条件付き終了処理
#if ENABLE_IMGUI
		// ImGuiの終了処理
		if (imguiSetup_) {
			imguiSetup_->Finalize();
			imguiSetup_.reset();
		}
#endif // ENABLE_IMGUI
		//========================================
		// トレイルエフェクトの終了処理
		trailEffectManager_.reset();
		trailEffectSetup_.reset();
		//========================================
		// ライトマネージャの終了処理
		if (lightManager_) {
			lightManager_->Finalize();
			lightManager_.reset();
		}
		//========================================
		// デバッグテキストの終了処理
		DebugTextManager::GetInstance()->Finalize();
		//========================================
		// audioの終了処理
		MAudioG::GetInstance()->Finalize();
		//========================================
		// テクスチャマネージャの終了処理
		if (textureManager_) {
			textureManager_->Finalize();
			textureManager_.reset();
		}
		//========================================
		// モデルマネージャの終了処理
		ModelManager::GetInstance()->Finalize();
		//========================================
		// ラインマネージャの終了処理
		if (lineManager_) {
			lineManager_->Finalize();
			lineManager_.reset();
		}
		//========================================
		// 共通描画セットアップの終了処理
		cloudSetup_.reset();
		skyboxSetup_.reset();
		object3dSetup_.reset();
		particleSetup_.reset();
		spriteSetup_.reset();
		postEffectManager_.reset();
		srvSetup_.reset();
		//========================================
		// カメラマネージャの終了処理
		if (cameraManager_) {
			cameraManager_->Finalize();
			cameraManager_.reset();
		}
		//========================================
		// ダイレクトX
		if (dxCore_) {
			dxCore_->ReleaseDirectX();
			dxCore_.reset();
		}
		//========================================
		// ウィンドウの終了
		if (win_) {
			win_->CloseWindow();
			win_.reset();
		}
	}

	///=============================================================================
	///                        レンダーテクスチャ前処理
	void MagFramework::RenderPreDraw() {
		dxCore_->RenderTexturePreDraw();
		srvSetup_->PreDraw();
		//========================================
		//  Lineの描画
		lineManager_->Draw();
	}

	///=============================================================================
	///                        レンダーテクスチャ後処理
	void MagFramework::RenderPostDraw() {
		dxCore_->RenderTexturePostDraw();
	}

	///=============================================================================
	///						フレームワーク共通前処理
	void MagFramework::PreDraw() {
		//========================================
		// ループ前処理(ポストエフェクト適用)
		dxCore_->PreDraw(postEffectManager_.get(), *textureManager_);
	}

	///=============================================================================
	///						フレームワーク共通後処理
	void MagFramework::PostDraw() {
		//========================================
		// COMMENT: ImGui 条件付き描画
#if ENABLE_IMGUI
		// ImGui描画
		imguiSetup_->Draw();
#endif // ENABLE_IMGUI
		//========================================
		// ループ後処理
		dxCore_->PostDraw();
	}

	///=============================================================================
	///						ImGuiの更新前処理
	void MagFramework::ImGuiPreDraw() {
		//========================================
		// COMMENT: デバッグビルドのみ ImGui フレーム開始
#if ENABLE_IMGUI
		imguiSetup_->Begin();
#ifdef _DEBUG
		//========================================
		// エディターレイアウトの更新と描画
		if (editorLayout_) {
			editorLayout_->Update();
			editorLayout_->Draw();
		}

		// シーンのImgui描画
		sceneManager_->ImGuiDraw();
		// InPutのImGui描画
		Input::GetInstance()->ImGuiDraw();
		// CameraのImGui描画
		cameraManager_->DrawImGui();
		// LightのImGui描画
		lightManager_->DrawImGui();
		// LineのImGui描画
		lineManager_->DrawImGui();
		// TrailEffectManagerのImGui描画
		trailEffectManager_->DrawImGui();
		// ImGuiでデバッグテキストを描画
		DebugTextManager::GetInstance()->DrawImGui();
		// ポストエフェクトのImGui描画
		DrawPostEffectImGui();
#endif // DEBUG
#endif // ENABLE_IMGUI
	}

	///=============================================================================
	///						ImGuiの更新後処理
	void MagFramework::ImGuiPostDraw() {
		//========================================
		// COMMENT: デバッグビルドのみ ImGui フレーム終了
#if ENABLE_IMGUI
		imguiSetup_->End();
#endif // ENABLE_IMGUI
	}

	///=============================================================================
	///						Object2D共通描画設定
	void MagFramework::Object2DCommonDraw() {
		//========================================
		// スプライト共通描画設定
		spriteSetup_->CommonDrawSetup();
		// 2D描画
		sceneManager_->Object2DDraw();
	}

	///=============================================================================
	///						particle共通描画設定
	void MagFramework::ParticleCommonDraw() {
		//========================================
		// パーティクル共通描画設定
		particleSetup_->CommonDrawSetup();
		// パーティクル描画
		sceneManager_->ParticleDraw();
	}

	///=============================================================================
	///						3D不透明描画
	void MagFramework::OpaqueRender() {
		renderWorld_.Clear();
		sceneManager_->RegisterRenderables(renderWorld_);

		auto commandList = dxCore_->GetCommandList();
		assert(commandList);
		RenderContext renderContext{*commandList.Get()};
		renderer_.Render(renderContext, renderWorld_);
	}

	///=============================================================================
	///						Skybox共通描画設定
	void MagFramework::SkyboxCommonDraw() {
		//========================================
		// Skybox共通描画設定
		skyboxSetup_->CommonDrawSetup();
		// Skybox描画（最初に描画して背景として扱う）
		sceneManager_->SkyboxDraw();
	}

	///=============================================================================
	///						Cloud共通描画設定
	void MagFramework::CloudCommonDraw() {
		//========================================
		// Cloud共通描画設定
		cloudSetup_->CommonDrawSetup();
		// Cloud描画
		sceneManager_->CloudDraw();
	}

	///=============================================================================
	///						TrailEffect共通描画設定
	void MagFramework::TrailEffectCommonDraw() {
		//========================================
		// TrailEffect共通描画設定
		trailEffectSetup_->CommonDrawSetup();
		// TrailEffect描画
		sceneManager_->TrailEffectDraw();
	}

	///=============================================================================
	///						ポストエフェクトのImGui描画
	void MagFramework::DrawPostEffectImGui() {
		ImGui::Begin("Post Effects");

		ImGui::Text("Multiple effects can be applied in order");
		ImGui::Separator();

		bool grayscaleEnabled = postEffectManager_->IsEffectEnabled(PostEffectManager::EffectType::Grayscale);
		if (ImGui::Checkbox("Grayscale", &grayscaleEnabled)) {
			postEffectManager_->SetEffectEnabled(PostEffectManager::EffectType::Grayscale, grayscaleEnabled);
		}

		bool vignetteEnabled = postEffectManager_->IsEffectEnabled(PostEffectManager::EffectType::Vignette);
		if (ImGui::Checkbox("Vignette", &vignetteEnabled)) {
			postEffectManager_->SetEffectEnabled(PostEffectManager::EffectType::Vignette, vignetteEnabled);
		}

		ImGui::End();
	}
}

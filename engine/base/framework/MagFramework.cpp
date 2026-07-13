/*********************************************************************
 * \file   MagFramework.cpp
 * \brief  エンジンフレームワークの基本クラスの実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   ゲームループと初期化・終了処理の実装
 *********************************************************************/
#include "MagFramework.h"
#include "Logger.h"
#include "WinApp.h"
#include "engine/render/pass/RenderContext.h"

#include <format>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	namespace {
		const char *ToString(PostEffectManager::PostEffectResourceSlot slot) {
			switch(slot) {
			case PostEffectManager::PostEffectResourceSlot::Ping:
				return "Ping";
			case PostEffectManager::PostEffectResourceSlot::Pong:
				return "Pong";
			}
			return "Unknown";
		}

		const char *ToString(PostEffectManager::PostEffectStage stage) {
			switch(stage) {
			case PostEffectManager::PostEffectStage::BeforeEffect:
				return "BeforeEffect";
			case PostEffectManager::PostEffectStage::AfterEffect:
				return "AfterEffect";
			}
			return "Unknown";
		}

		const char *ToString(PostEffectManager::PostEffectTransitionMismatchReason reason) {
			switch(reason) {
			case PostEffectManager::PostEffectTransitionMismatchReason::MissingRecordedTransition:
				return "MissingRecordedTransition";
			case PostEffectManager::PostEffectTransitionMismatchReason::UnexpectedRecordedTransition:
				return "UnexpectedRecordedTransition";
			case PostEffectManager::PostEffectTransitionMismatchReason::SlotMismatch:
				return "SlotMismatch";
			case PostEffectManager::PostEffectTransitionMismatchReason::BeforeStateMismatch:
				return "BeforeStateMismatch";
			case PostEffectManager::PostEffectTransitionMismatchReason::AfterStateMismatch:
				return "AfterStateMismatch";
			case PostEffectManager::PostEffectTransitionMismatchReason::StageMismatch:
				return "StageMismatch";
			case PostEffectManager::PostEffectTransitionMismatchReason::SequenceMismatch:
				return "SequenceMismatch";
			}
			return "Unknown";
		}

		void ReportPostEffectInternalDiagnostics(const PostEffectManager &postEffectManager) {
			const auto &plan = postEffectManager.GetResourceTransitionPlan();
			const auto &executed = postEffectManager.GetResourceTransitions();
			const PostEffectManager::PostEffectTransitionComparisonResult comparison =
				postEffectManager.CompareResourceTransitionPlanWithRecordedTransitions();

			Logger::Log(std::format("PostEffect Internal Plan: Count={}", plan.size()), Logger::LogLevel::Info);
			for(const PostEffectManager::PostEffectResourceTransition &transition : plan) {
				Logger::Log(std::format("PostEffect Internal Plan Seq={} Slot={} ResourceIndex={} Stage={} 0x{:X} -> 0x{:X}",
										transition.sequence,
										ToString(transition.slot),
										transition.resourceIndex,
										ToString(transition.stage),
										static_cast<uint32_t>(transition.beforeState),
										static_cast<uint32_t>(transition.afterState)),
							Logger::LogLevel::Info);
			}

			Logger::Log(std::format("PostEffect Internal Executed: Count={}", executed.size()), Logger::LogLevel::Info);
			for(const PostEffectManager::PostEffectResourceTransition &transition : executed) {
				Logger::Log(std::format("PostEffect Internal Executed Seq={} Slot={} ResourceIndex={} Stage={} 0x{:X} -> 0x{:X}",
										transition.sequence,
										ToString(transition.slot),
										transition.resourceIndex,
										ToString(transition.stage),
										static_cast<uint32_t>(transition.beforeState),
										static_cast<uint32_t>(transition.afterState)),
							Logger::LogLevel::Info);
			}

			Logger::Log(std::format("PostEffect Internal Mismatch: Count={} Match={}",
									comparison.mismatches.size(),
									comparison.isMatch),
						comparison.isMatch ? Logger::LogLevel::Success : Logger::LogLevel::Warning);
			for(const PostEffectManager::PostEffectTransitionMismatch &mismatch : comparison.mismatches) {
				Logger::Log(std::format("PostEffect Internal Mismatch Reason={} ExpectedSeq={} ActualSeq={}",
										ToString(mismatch.reason),
										mismatch.expected.sequence,
										mismatch.actual.sequence),
							Logger::LogLevel::Warning);
			}
		}
	}

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
		editorUiSystem_ = std::make_unique<EditorUiSystem>();
		editorUiSystem_->Initialize();
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
		///						 ラインマネージャ
		lineManager_ = std::make_unique<LineManager>();
		lineManager_->Initialize(dxCore_.get(), srvSetup_.get());
		// NOTE: RendererはLineRenderPass内でLineManager参照を保持するため、Pass生成前に実体を用意する。
		lineManager_->SetDefaultCamera(cameraManager_->GetCurrentCamera());

		///--------------------------------------------------------------
		///						 トレイルエフェクト共通部
		trailEffectSetup_ = std::make_unique<TrailEffectSetup>();
		// トレイルエフェクトセットアップの初期化
		trailEffectSetup_->Initialize(dxCore_.get());
		// トレイルエフェクトセットアップにSrvSetupを設定
		trailEffectSetup_->SetSrvSetup(srvSetup_.get());
		// NOTE: Scene/Overlay/PostOverlay/PostProcessの順に実行し、Present前の合成までPassで管理する。
		renderer_.Initialize(*skyboxSetup_, *object3dSetup_, *cloudSetup_, *trailEffectSetup_, *spriteSetup_, *particleSetup_, *lineManager_, *dxCore_, *postEffectManager_, *textureManager_);

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
		///						 オーディオの初期化
		MAudioG::GetInstance()->Initialize("resources/sound/");

		///--------------------------------------------------------------
		///						 EngineContextの構築
		InitializeEngineContext();

#if ENABLE_IMGUI
		///--------------------------------------------------------------
		///						 常駐Debug UI登録
		RegisterEngineEditorPanels();
#endif

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
#if ENABLE_IMGUI
		engineContext_.editorUiSystem = editorUiSystem_.get();
#endif
		engineContext_.Validate();
	}

#if ENABLE_IMGUI
	///=============================================================================
	///						常駐Debug UI登録
	void MagFramework::RegisterEngineEditorPanels() {
		// NOTE: Framework は登録だけを担当し、毎フレームの個別呼び出しは EditorUiSystem に集約する。
		editorUiSystem_->RegisterPanel("Input", EditorUiCategory::Engine, false, []() {
			Input::GetInstance()->ImGuiDraw();
		});
		editorUiSystem_->RegisterPanel("Camera Manager", EditorUiCategory::Engine, true, [this]() {
			cameraManager_->DrawImGui();
		});
		editorUiSystem_->RegisterPanel("Light Manager", EditorUiCategory::Engine, true, [this]() {
			lightManager_->DrawImGui();
		});
		editorUiSystem_->RegisterPanel("Line Manager", EditorUiCategory::Engine, false, [this]() {
			lineManager_->DrawImGui();
		});
		editorUiSystem_->RegisterPanel("Trail Effect Manager", EditorUiCategory::Engine, false, [this]() {
			trailEffectManager_->DrawImGui();
		});
		editorUiSystem_->RegisterPanel("Debug Text Manager", EditorUiCategory::Engine, false, []() {
			DebugTextManager::GetInstance()->DrawImGui();
		});
		editorUiSystem_->RegisterPanel("Post Effects", EditorUiCategory::Rendering, true, [this]() {
			DrawPostEffectImGui();
		});
		editorUiSystem_->RegisterPanel("Performance", EditorUiCategory::Application, false, [this]() {
			imguiSetup_->ShowPerformanceMonitor();
		});
	}
#endif

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
		// COMMENT: ImGui 条件付き終了処理
#if ENABLE_IMGUI
		if (editorUiSystem_) {
			editorUiSystem_->Finalize();
			editorUiSystem_.reset();
		}
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
#ifdef _DEBUG
			// NOTE: D3D12の簡易終了レポートでは所有元を追いづらいため、標準Live Objectレポートを明示する。
			dxCore_->CheckResourceLeaks();
#endif
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
		renderer_.BeginFrameBarrierRecording();
		dxCore_->RenderTexturePreDraw();
		srvSetup_->PreDraw();
	}

	///=============================================================================
	///                        レンダーテクスチャ後処理
	void MagFramework::RenderPostDraw() {
		dxCore_->RenderTexturePostDraw();
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
		renderer_.ValidateFrameBarriers();
	}

	///=============================================================================
	///						ImGuiの更新前処理
	void MagFramework::ImGuiPreDraw() {
		//========================================
		// COMMENT: デバッグビルドのみ ImGui フレーム開始
#if ENABLE_IMGUI
		imguiSetup_->Begin();
#ifdef _DEBUG
		// NOTE: 個別Debug UIはEditorUiSystemへ登録し、Frameworkは実行入口だけを持つ。
		if (editorUiSystem_) {
			editorUiSystem_->Draw();
		}
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
	///						Sceneフェーズ描画
	void MagFramework::OpaqueRender() {
		renderWorld_.Clear();
		sceneManager_->RegisterRenderables(renderWorld_);
		// NOTE: World/HUDで描画順と深度方針を分けるため、同じLineManager参照をモード付きで登録する。
		renderWorld_.AddLine(LineRenderItem{lineManager_.get(), 0, true, LineRenderMode::World});
		renderWorld_.AddLine(LineRenderItem{lineManager_.get(), 1, true, LineRenderMode::Hud});
		lineManager_->SetRenderWorldLineItemCount(static_cast<uint32_t>(renderWorld_.GetLineItems().size()));

		auto commandList = dxCore_->GetCommandList();
		assert(commandList);
		RenderContext renderContext{*commandList.Get()};
		renderer_.ExecutePhase(RenderPhase::Scene, renderContext, renderWorld_);
	}

	///=============================================================================
	///						指定フェーズのRenderPass描画
	void MagFramework::ExecuteRenderPhase(RenderPhase phase) {
		auto commandList = dxCore_->GetCommandList();
		assert(commandList);
		RenderContext renderContext{*commandList.Get()};
		renderer_.ExecutePhase(phase, renderContext, renderWorld_);
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

#ifdef _DEBUG
		ImGui::Separator();
		ImGui::Text("RenderGraph barriers: %zu", renderer_.GetRenderGraph().GetManualBarriers().size());
		const std::vector<PostEffectManager::PostEffectResourceTransition> &plan = postEffectManager_->GetResourceTransitionPlan();
		const std::vector<PostEffectManager::PostEffectResourceTransition> &transitions = postEffectManager_->GetResourceTransitions();
		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			postEffectManager_->CompareResourceTransitionPlanWithRecordedTransitions();
		ImGui::Text("PostEffect internal plan: %zu", plan.size());
		ImGui::Text("PostEffect internal executed: %zu", transitions.size());
		ImGui::Text("PostEffect internal mismatch: %zu", comparison.mismatches.size());
		for(size_t i = 0; i < transitions.size(); ++i) {
			const PostEffectManager::PostEffectResourceTransition &transition = transitions[i];
			// NOTE: PostEffect内部遷移は簡易RenderGraph外なので、Smoke Test用に直近フレームの実行結果だけ見せる。
			ImGui::Text("Transition %zu: Seq=%u %s[%u] %s 0x%X -> 0x%X",
						i,
						transition.sequence,
						ToString(transition.slot),
						transition.resourceIndex,
						ToString(transition.stage),
						static_cast<uint32_t>(transition.beforeState),
						static_cast<uint32_t>(transition.afterState));
		}
		if(ImGui::Button("Report Render Diagnostics")) {
			renderer_.ReportSmokeTestDiagnostics();
			ReportPostEffectInternalDiagnostics(*postEffectManager_);
			if(lineManager_) {
				lineManager_->ReportDiagnostics();
			}
			if(dxCore_) {
				dxCore_->ReportDebugMessages();
			}
		}
#endif

		ImGui::End();
	}
}

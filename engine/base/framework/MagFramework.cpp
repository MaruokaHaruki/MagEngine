/*********************************************************************
 * \file   MagFramework .cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "MagFramework.h"
#include "WinApp.h"

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
	win_->CreateGameWindow(L"MagEngine_Ver1.0.2");

	///--------------------------------------------------------------
	///						 ダイレクトX生成
	dxCore_ = std::make_unique<DirectXCore>();
	// ダイレクトXの初期化
	dxCore_->InitializeDirectX(win_.get());
	dxCore_->CreateRenderTextureRTV();

	///--------------------------------------------------------------
	///						 ImGuiのセットアップ
	imguiSetup_ = std::make_unique<ImguiSetup>();
	// ImGuiの初期化
	imguiSetup_->Initialize(win_.get(), dxCore_.get(), Style::CLASSIC);

	///--------------------------------------------------------------
	/// 					 カメラの初期化
	CameraManager::GetInstance()->Initialize();

	///--------------------------------------------------------------
	///                        デバックテキストマネージャ
	DebugTextManager::GetInstance()->Initialize(win_.get());
	// デバッグテキストマネージャの初期化（カメラ初期化後に設定）
	DebugTextManager::GetInstance()->SetCamera(CameraManager::GetInstance()->GetCurrentCamera());
	// デバッグテキストの表示を有効にする
	DebugTextManager::GetInstance()->SetDebugTextEnabled(true);

	// 初期の永続的なデバッグテキストを設定
	DebugTextManager::GetInstance()->AddAxisLabels(); // 座標軸ラベル
	// DebugTextManager::GetInstance()->AddGridLabels(5.0f, 2); // グリッドラベル

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
	TextureManager::GetInstance()->Initialize(dxCore_.get(), "resources/texture/", srvSetup_.get());

	///--------------------------------------------------------------
	///						 ライトマネージャ
	lightManager_ = std::make_unique<LightManager>();
	// ライトマネージャの初期化
	lightManager_->Initialize();

	///--------------------------------------------------------------
	/// 					 カメラの初期化
	CameraManager::GetInstance()->Initialize();
	// TODO: カメラの改善 現在、カメラの設定はここで行っているが、自由に変更がしにくいという問題が発生している。

	///--------------------------------------------------------------
	///						 スプライトクラス
	//========================================
	// スプライト共通部
	spriteSetup_ = std::make_unique<SpriteSetup>();
	// スプライト共通部の初期化
	spriteSetup_->Initialize(dxCore_.get());

	///--------------------------------------------------------------
	///						 Object3D共通部
	//========================================
	// モデルマネージャの初期化
	ModelManager::GetInstance()->Initialize(dxCore_.get());
	//========================================
	// 3Dオブジェクト共通部
	object3dSetup_ = std::make_unique<Object3dSetup>();
	// 3Dオブジェクト共通部の初期化
	object3dSetup_->Initialize(dxCore_.get());
	// Object3Dのカメラ設定
	object3dSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
	// Object3Dのライトマネージャ設定
	object3dSetup_->SetLightManager(lightManager_.get());

	///--------------------------------------------------------------
	///						 Skybox共通部
	skyboxSetup_ = std::make_unique<SkyboxSetup>();
	//  Skyboxの初期化
	skyboxSetup_->Initialize(dxCore_.get());
	//  Skyboxのカメラ設定
	skyboxSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());

	///--------------------------------------------------------------
	///						 パーティクル共通部
	particleSetup_ = std::make_unique<ParticleSetup>();
	// パーティクルセットアップの初期化
	particleSetup_->Initialize(dxCore_.get(), srvSetup_.get());
	// パーティクルのカメラ設定
	particleSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());

	///--------------------------------------------------------------
	///						 クラウド共通部
	cloudSetup_ = std::make_unique<CloudSetup>();
	// クラウドセットアップの初期化
	cloudSetup_->Initialize(dxCore_.get());

	///--------------------------------------------------------------
	///						 ラインマネージャ
	LineManager::GetInstance()->Initialize(dxCore_.get(), srvSetup_.get());
	// Lineのカメラ設定
	LineManager::GetInstance()->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());

	///--------------------------------------------------------------
	///						 オーディオの初期化
	MAudioG::GetInstance()->Initialize("resources/sound/");

	///--------------------------------------------------------------
	///						 シーンマネージャ
	sceneManager_ = std::make_unique<SceneManager>();
	// シーンマネージャの初期化
	sceneManager_->Initialize(spriteSetup_.get(), object3dSetup_.get(), particleSetup_.get(),
		skyboxSetup_.get(), cloudSetup_.get());
	// シーンファクトリーのセット		
	sceneFactory_ = std::make_unique<SceneFactory>();
	sceneManager_->SetSceneFactory(sceneFactory_.get());

	///--------------------------------------------------------------
	///						 各種設定
	// ライトマネージャへラインマネージャポインタの受け渡し
	lightManager_->SetLineManager(LineManager::GetInstance());
}

///=============================================================================
///						更新
void MagFramework::Update() {
	//========================================
	// デバックカメラの呼び出し1,2
	if (Input::GetInstance()->PushKey(DIK_1)) {
		CameraManager::GetInstance()->SetCurrentCamera("DebugCamera");
	}
	if (Input::GetInstance()->PushKey(DIK_2)) {
		CameraManager::GetInstance()->SetCurrentCamera("DefaultCamera");
	}

	//========================================
	// カメラの更新
	CameraManager::GetInstance()->UpdateAll();

	//========================================
	// デバックテキストの更新（カメラ更新後に実行）
	DebugTextManager::GetInstance()->SetCamera(CameraManager::GetInstance()->GetCurrentCamera());
	DebugTextManager::GetInstance()->Update();

	//========================================
	// ラインの更新
	// カメラの更新
	LineManager::GetInstance()->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
	// ラインの更新
	LineManager::GetInstance()->Update();

	//=========================================
	// ライトの可視化
	lightManager_->Update();

	//========================================
	// Object3Dのカメラ設定の更新
	object3dSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
	// particleのカメラ設定の更新
	particleSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
	// skyboxのカメラ設定の更新
	skyboxSetup_->SetDefaultCamera(CameraManager::GetInstance()->GetCurrentCamera());
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
	//========================================
	// ImGuiの終了処理
	imguiSetup_->Finalize();
	//========================================
	// audioの終了処理
	MAudioG::GetInstance()->Finalize();
	//========================================
	// テクスチャマネージャの終了処理
	TextureManager::GetInstance()->Finalize();
	//========================================
	// モデルマネージャの終了処理
	ModelManager::GetInstance()->Finalize();
	//========================================
	// ラインマネージャの終了処理
	LineManager::GetInstance()->Finalize();
	//========================================
	// ダイレクトX
	dxCore_->ReleaseDirectX();
	//========================================
	// ウィンドウの終了
	win_->CloseWindow();
}

///=============================================================================
///                        レンダーテクスチャ前処理
void MagFramework::RenderPreDraw() {
	dxCore_->RenderTexturePreDraw();
	srvSetup_->PreDraw();
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
	// ループ前処理
	dxCore_->PreDraw();
	//========================================
	//  Lineの描画
	LineManager::GetInstance()->Draw();
}

///=============================================================================
///						フレームワーク共通後処理
void MagFramework::PostDraw() {
	//========================================
	// ImGui描画
	imguiSetup_->Draw();
	//========================================
	// ループ後処理
	dxCore_->PostDraw();
}

///=============================================================================
///						ImGuiの更新前処理
void MagFramework::ImGuiPreDraw() {
	//========================================
	// imguiの初期化
	imguiSetup_->Begin();
#ifdef _DEBUG
	// シーンのImgui描画
	sceneManager_->ImGuiDraw();
	// InPutのImGui描画
	Input::GetInstance()->ImGuiDraw();
	// CameraのImGui描画
	CameraManager::GetInstance()->DrawImGui();
	// LightのImGui描画
	lightManager_->DrawImGui();
	// LineのImGui描画
	LineManager::GetInstance()->DrawImGui();
	// ImGuiでデバッグテキストを描画
	DebugTextManager::GetInstance()->DrawImGui();
#endif // DEBUG
}

///=============================================================================
///						ImGuiの更新後処理
void MagFramework::ImGuiPostDraw() {
	//========================================
	// imguiの終了処理
	imguiSetup_->End();
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
///						Object3D共通描画設定
void MagFramework::Object3DCommonDraw() {
	//========================================s
	// 3D共通描画設定
	object3dSetup_->CommonDrawSetup();
	// 3D描画
	sceneManager_->Object3DDraw();
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

/*********************************************************************
 * \file   TitleScene.cpp
 * \brief  タイトルシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#include "TitleScene.h"
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "EngineContext.h"
#include "Input.h"
#include "ModelManager.h"
#include "SceneContext.h"
#include "TextureManager.h"
#include "TitleCamera.h"
#include <cassert>
using namespace MagEngine;

///=============================================================================
/// 初期化
/// NOTE: contextからセットアップを取得
void TitleScene::Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) {
	engineContext.Validate();
	engineContext_ = &engineContext;
	sceneContext_ = &sceneContext;
	CameraManager *cameraManager = engineContext_->cameraManager;
	TextureManager *textureManager = engineContext_->textureManager;
	ModelManager *modelManager = engineContext_->modelManager;

	// NOTE: EngineContextからセットアップを取得し、SceneContextはScene管理情報に限定する
	MagEngine::SpriteSetup *spriteSetup = engineContext_->spriteSetup;
	MagEngine::Object3dSetup *object3dSetup = engineContext_->object3dSetup;
	MagEngine::SkyboxSetup *skyboxSetup = engineContext_->skyboxSetup;
	MagEngine::CloudSetup *cloudSetup = engineContext_->cloudSetup;

	//========================================
	// 読み込み関係
	// textureManager->LoadTexture(".dds");
	// スプライト
	textureManager->LoadTexture("uvChecker.dds");
	// 演出系
	textureManager->LoadTexture("WolfOne_Title.dds");
	textureManager->LoadTexture("WolfOne_Triangle.dds");
	textureManager->LoadTexture("WolfOne_PressEnter.dds");
	textureManager->LoadTexture("WolfOne_PressA.dds");
	textureManager->LoadTexture("WolfOne_Engage.dds");
	textureManager->LoadTexture("WolfOne_GameOver.dds");
	textureManager->LoadTexture("WolfOne_Comprete.dds");
	// 操作ガイドUI
	textureManager->LoadTexture("xbox_button_color_a.dds");
	textureManager->LoadTexture("xbox_button_color_b.dds");
	textureManager->LoadTexture("xbox_button_color_x.dds");
	textureManager->LoadTexture("xbox_button_color_y.dds");
	textureManager->LoadTexture("xbox_rt.dds");
	textureManager->LoadTexture("xbox_ls.dds");
	textureManager->LoadTexture("white1x1.dds"); // トランジション用
	// 操作テキスト
	textureManager->LoadTexture("WolfOne_Dodge.dds");
	textureManager->LoadTexture("WolfOne_MachineGun.dds");
	textureManager->LoadTexture("WolfOne_ControlStick.dds");
	textureManager->LoadTexture("WolfOne_Missile.dds");
	textureManager->LoadTexture("WolfOne_Test.dds");
	// メニューテキスト
	textureManager->LoadTexture("WolfOne_Resume.dds");
	textureManager->LoadTexture("WolfOne_Controls.dds");
	textureManager->LoadTexture("WolfOne_ReturntoTitle.dds");
	textureManager->LoadTexture("WolfOne_Pause.dds");
	// モデル
	modelManager->LoadModel("jet.obj"); // モデルは事前にロードしておく
	modelManager->LoadModel("Missile.obj");
	modelManager->LoadModel("Bullet.obj");  // 弾のモデル
	modelManager->LoadModel("ground.obj");  // 地形のモデル
	modelManager->LoadModel("skydome.obj"); // 地面のモデルもロード
	// スカイボックス
	textureManager->LoadTexture("rostock_laage_airport_4k.dds");
	textureManager->LoadTexture("qwantani_dusk_2_puresky_4k.dds");
	textureManager->LoadTexture("overcast_soil_puresky_4k.dds");
	textureManager->LoadTexture("moonless_golf_4k.dds");
	textureManager->LoadTexture("kloppenheim_02_puresky_4k.dds");

	//========================================
	// カメラ設定
	cameraManager->AddCamera("TitleCamera");

	// TitleCameraの初期化
	titleCamera_ = std::make_unique<TitleCamera>();
	titleCamera_->Initialize("TitleCamera", *engineContext_->cameraManager);
	// FollowCameraをメインカメラに設定
	cameraManager->SetCurrentCamera("TitleCamera");

	//========================================
	// スプライト
	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Initialize(spriteSetup, "WolfOne_Title.dds");
	titleSprite_->SetPosition({640.0f, 130.0f}); // 画面上部中央
	titleSprite_->SetAnchorPoint({0.5f, 0.5f});	 // 中心を基準点に

	pressEnterSprite_ = std::make_unique<Sprite>();
	pressEnterSprite_->Initialize(spriteSetup, "WolfOne_PressA.dds");
	pressEnterSprite_->SetPosition({640.0f, 550.0f});	// 画面下部中央
	pressEnterSprite_->SetAnchorPoint({0.5f, 0.5f});	// 中心を基準点に
	pressEnterBaseSize_ = pressEnterSprite_->GetSize(); // Press Enterの基本サイズを保存

	//========================================
	// プレイヤーの初期化（演出用）
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dSetup, "jet.obj", *engineContext_->input, *engineContext_->lineManager);
	// プレイヤーの初期位置（カメラから見える位置）
	Vector3 initialPos = {0.0f, 5.0f, 10.0f}; // カメラの前方
	player_->GetObject3d()->GetTransform()->translate = initialPos;
	player_->GetObject3d()->GetTransform()->scale = {0.5f, 0.5f, 0.5f}; // サイズ調整
	// カメラにプレイヤーを設定
	titleCamera_->SetPlayer(player_.get());

	//========================================
	// スカイボックス	
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(skyboxSetup);
	// Skyboxのモデルを設定
	skybox_->SetTexture("overcast_soil_puresky_4k.dds");
	// SkyboxのTransformを設定
	skybox_->SetTransform({{1000.0f, 1000.0f, 1000.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});
	//========================================
	// 雲
	cloud_ = std::make_unique<Cloud>();
	cloud_->Initialize(cloudSetup);

	// 雲のサイズ設定（広い範囲に配置）
	cloud_->SetSize({500.0f, 100.0f, 500.0f});
	cloud_->SetEnabled(true);

	// 雲のTransform設定
	cloud_->GetTransform().translate = {0.0f, -50.0f, 0.0f};

	// 雲の密度と速度を調整（美しい表現）
	auto &cloudParams = cloud_->GetMutableParams();
	// 密度：雲の濃さ（自然な透け感）
	cloudParams.density = 1.5f;
	// カバレッジ：雲の分布（豊かな分布）
	cloudParams.coverage = 0.35f;
	// ノイズ速度：雲の流れる速さ（自然な流れ）
	cloudParams.noiseSpeed = 1.5f;
	// 環境光：雲の明るさ（明るく映える）
	cloudParams.ambient = 0.75f;
	// 太陽光強度：太陽光による照明の強さ（影がはっきり）
	cloudParams.sunIntensity = 1.6f;
	// ベースノイズスケール：大きな雲の形状（自然なサイズ）
	cloudParams.baseNoiseScale = 0.0085f;
	// ディテールウェイト：細かいディテールの影響度（より詳細に）
	cloudParams.detailWeight = 0.35f;
	//========================================
	// トランジション
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize(spriteSetup);
	sceneTransition_->SetColor({0.0f, 0.0f, 0.0f, 1.0f}); // 黒

	// 完全にリセットしてからトランジション開始
	sceneTransition_->Reset();

	// シーン開始時にオープニングトランジション
	sceneTransition_->StartOpening(TransitionType::ZoomIn, 1.5f);
}

///=============================================================================
///						終了処理
void TitleScene::Finalize() {
}

///=============================================================================
/// NOTE: GamePlay候補の初期化失敗後もTitleを継続するため、完了済みの閉じ演出を解除する。
void TitleScene::OnSceneChangeFailed(int requestedSceneNo, std::string_view errorMessage) {
	(void)requestedSceneNo;
	(void)errorMessage;
	SetSceneNo(-1);

	if (sceneTransition_) {
		sceneTransition_->SetOnCompleteCallback({});
		sceneTransition_->Cancel();
	}

	if (engineContext_ && engineContext_->cameraManager) {
		CameraManager *cameraManager = engineContext_->cameraManager;
		cameraManager->SetCurrentCamera("TitleCamera");
		if (engineContext_->debugTextManager) {
			engineContext_->debugTextManager->SetCamera(cameraManager->GetCamera("TitleCamera"));
		}
	}
}

///=============================================================================
///						更新
void TitleScene::Update(const FrameTime &frameTime) {
	const float unscaledDeltaTime = frameTime.unscaledDeltaTime;
	assert(engineContext_);
	Input *input = engineContext_->input;
	CameraManager *cameraManager = engineContext_->cameraManager;

	//========================================
	// 経過時間の更新
	totalElapsedTime_ += unscaledDeltaTime;

	//========================================
	// Object3D

	//========================================
	// Sprite
	if (titleSprite_) {
		titleSprite_->Update();
	}

	// Press Enterの点滅処理
	blinkTimer_ += unscaledDeltaTime;

	// フェード速度（1秒でフェードイン/アウト）
	float fadeSpeed = 1.0f;

	if (isFadingOut_) {
		pressEnterAlpha_ -= fadeSpeed / 60.0f;
		if (pressEnterAlpha_ <= 0.0f) {
			pressEnterAlpha_ = 0.0f;
			isFadingOut_ = false;
		}
	} else {
		pressEnterAlpha_ += fadeSpeed / 60.0f;
		if (pressEnterAlpha_ >= 1.0f) {
			pressEnterAlpha_ = 1.0f;
			isFadingOut_ = true;
		}
	}

	// Press Enterスプライトの透過度とスケールを設定
	if (pressEnterSprite_) {
		pressEnterSprite_->SetColor({1.0f, 1.0f, 1.0f, pressEnterAlpha_});

		// スケール変動（点滅に合わせて）
		float pulseScale = 0.95f + pressEnterAlpha_ * 0.1f;
		pressEnterSprite_->SetSize({pressEnterBaseSize_.x * pulseScale,
									pressEnterBaseSize_.y * pulseScale});
		pressEnterSprite_->Update();
	}

	//=========================================
	// Skybox
	if (skybox_) {
		skybox_->Update();

		// スカイボックスの露出値を時間で変化させ、雰囲気を演出
		// 明るい時間帯から始まり、ゆっくり暗くなる
		float exposureVariation = std::sin(totalElapsedTime_ * 0.3f) * 0.2f;
		// TODO: Skyboxに露出度設定メソッドがあれば使用
	}

	//=========================================
	// 雲の更新
	if (cloud_) {
		cloud_->Update(*cameraManager->GetCurrentCamera(), unscaledDeltaTime);
	}

	//========================================
	// タイトルカメラ更新
	if (titleCamera_) {
		titleCamera_->Update();
	}

	//========================================
	// プレイヤー更新（演出用の動き）
	if (player_) {
		Transform *playerTransform = player_->GetObject3d()->GetTransform();
		player_->GetObject3d()->Update();
	}

	//========================================
	// トランジション更新
	if (sceneTransition_) {
		sceneTransition_->Update(unscaledDeltaTime);
	}

	//========================================
	// シーン遷移
	if (input->TriggerKey(DIK_RETURN)) {
		// トランジション開始
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 1.0f);
			// トランジション完了時にシーン遷移
			sceneTransition_->SetOnCompleteCallback([this]() {
				SetSceneNo(SCENE::GAMEPLAY);
			});
		}
	}
	// コントローラ
	if (input->TriggerButton(XINPUT_GAMEPAD_A)) {
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 0.8f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				SetSceneNo(SCENE::GAMEPLAY);
			});
		}
	}
	// シーンリセット
	if (input->TriggerKey(DIK_R)) {
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 0.5f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				SetSceneNo(SCENE::TITLE);
			});
		}
	}
}

///=============================================================================
///						描画対象の登録
void TitleScene::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	if(skybox_) {
		skybox_->RegisterRenderables(renderWorld);
	}
	if(player_) {
		player_->RegisterRenderables(renderWorld);
	}
	if (cloud_) {
		cloud_->RegisterRenderables(renderWorld);
	}
	if (titleSprite_) {
		titleSprite_->RegisterRenderables(renderWorld);
	}
	if (pressEnterSprite_) {
		pressEnterSprite_->RegisterRenderables(renderWorld);
	}
	if (sceneTransition_) {
		sceneTransition_->RegisterRenderables(renderWorld);
	}
}


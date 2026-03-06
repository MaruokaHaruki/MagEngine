/*********************************************************************
 * \file   TitleScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "TitleScene.h"
#include "TitleCamera.h"
using namespace MagEngine;

///=============================================================================
///						初期化
void TitleScene::Initialize(MagEngine::SpriteSetup *spriteSetup,
							MagEngine::Object3dSetup *object3dSetup,
							MagEngine::ParticleSetup *particleSetup,
							MagEngine::SkyboxSetup *skyboxSetup,
							MagEngine::CloudSetup *cloudSetup,
							MagEngine::TrailEffectSetup *trailEffectSetup) {

	//========================================
	// 読み込み関係
	// TextureManager::GetInstance()->LoadTexture(".dds");
	// スプライト
	TextureManager::GetInstance()->LoadTexture("uvChecker.dds");
	// 演出系
	TextureManager::GetInstance()->LoadTexture("WolfOne_Title.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Triangle.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_PressEnter.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_PressA.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Engage.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_GameOver.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Comprete.dds");
	// 操作ガイドUI
	TextureManager::GetInstance()->LoadTexture("xbox_button_color_a.dds");
	TextureManager::GetInstance()->LoadTexture("xbox_button_color_b.dds");
	TextureManager::GetInstance()->LoadTexture("xbox_button_color_x.dds");
	TextureManager::GetInstance()->LoadTexture("xbox_button_color_y.dds");
	TextureManager::GetInstance()->LoadTexture("xbox_rt.dds");
	TextureManager::GetInstance()->LoadTexture("xbox_ls.dds");
	TextureManager::GetInstance()->LoadTexture("white1x1.dds"); // トランジション用
	// 操作テキスト
	TextureManager::GetInstance()->LoadTexture("WolfOne_Dodge.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_MachineGun.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_ControlStick.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Missile.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Test.dds");
	// メニューテキスト
	TextureManager::GetInstance()->LoadTexture("WolfOne_Resume.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Controls.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_ReturntoTitle.dds");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Pause.dds");
	// モデル
	ModelManager::GetInstance()->LoadModel("jet.obj"); // モデルは事前にロードしておく
	ModelManager::GetInstance()->LoadModel("Missile.obj");
	ModelManager::GetInstance()->LoadModel("Bullet.obj");  // 弾のモデル
	ModelManager::GetInstance()->LoadModel("ground.obj");  // 地形のモデル
	ModelManager::GetInstance()->LoadModel("skydome.obj"); // 地面のモデルもロード
	// スカイボックス
	TextureManager::GetInstance()->LoadTexture("rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("qwantani_dusk_2_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("overcast_soil_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("moonless_golf_4k.dds");
	TextureManager::GetInstance()->LoadTexture("kloppenheim_02_puresky_4k.dds");

	//========================================
	// カメラ設定
	CameraManager::GetInstance()->AddCamera("TitleCamera");

	// TitleCameraの初期化
	titleCamera_ = std::make_unique<TitleCamera>();
	titleCamera_->Initialize("TitleCamera");
	// FollowCameraをメインカメラに設定
	CameraManager::GetInstance()->SetCurrentCamera("TitleCamera");

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
	player_->Initialize(object3dSetup, "jet.obj");
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
///						更新
void TitleScene::Update() {
	//========================================
	// 経過時間の更新
	totalElapsedTime_ += 1.0f / 60.0f;

	//========================================
	// Object3D

	//========================================
	// Sprite
	if (titleSprite_) {
		titleSprite_->Update();
	}

	// Press Enterの点滅処理
	blinkTimer_ += 1.0f / 60.0f; // 1フレーム進める

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
		cloud_->Update(*CameraManager::GetInstance()->GetCurrentCamera(), 1.0f / 60.0f);
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
		sceneTransition_->Update();
	}

	//========================================
	// シーン遷移
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		// トランジション開始
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 1.0f);
			// トランジション完了時にシーン遷移
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::GAMEPLAY;
			});
		}
	}
	// コントローラ
	if (Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 0.8f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::GAMEPLAY;
			});
		}
	}
	// シーンリセット
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::ZoomIn, 0.5f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::TITLE;
			});
		}
	}
}

///=============================================================================
///						2D描画
void TitleScene::Object2DDraw() {
	if (titleSprite_) {
		titleSprite_->Draw();
	}

	// Press Enterを常に描画（透過度で制御）
	if (pressEnterSprite_) {
		pressEnterSprite_->Draw();
	}

	// トランジション描画（最前面）
	if (sceneTransition_) {
		sceneTransition_->Draw();
	}
}

///=============================================================================
///						3D描画
void TitleScene::Object3DDraw() {
	if (player_) {
		player_->Draw();
	}
}

///=============================================================================
///						Particle描画
void TitleScene::ParticleDraw() {
}

///=============================================================================
///						Skybox描画
void TitleScene::SkyboxDraw() {
	if (skybox_) {
		skybox_->Draw();
	}
}

///=============================================================================
///						Cloud描画
void TitleScene::CloudDraw() {
	//========================================
	// 雲の描画
	if (cloud_) {
		cloud_->Draw();
	}
}

///=============================================================================
///						TrailEffect描画
void TitleScene::TrailEffectDraw() {
	// タイトルシーンではTrailEffectは不要
}

///=============================================================================
///						ImGui描画
void TitleScene::ImGuiDraw() {
#ifdef _DEBUG
	// TitleSceneのImGui描画
	ImGui::Begin("TitleScene");
	ImGui::Text("Hello, TitleScene!");
	ImGui::End();

	if (titleCamera_) {
		titleCamera_->DrawImGui();
	}

	//========================================
	// シーン遷移
	if (sceneTransition_) {
		sceneTransition_->DrawImGui();
	}
#endif
}
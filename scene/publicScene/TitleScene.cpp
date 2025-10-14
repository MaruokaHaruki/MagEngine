/*********************************************************************
 * \file   TitleScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "TitleScene.h"
#include "CameraManager.h"
#include "Input.h"
#include "MAudioG.h"
#include "SceneManager.h"
#include "TitleCamera.h"

///=============================================================================
///						初期化
void TitleScene::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup, SkyboxSetup *skyboxSetup) {
	// 適当に引数を使用
	// 引数を使用しない場合は警告を出さないようにする
	spriteSetup;
	object3dSetup;
	particleSetup;
	skyboxSetup;

	//========================================
	// 読み込み関係
	// スプライト
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Title.png");
	TextureManager::GetInstance()->LoadTexture("WolfOne_Triangle.png");

	// モデル
	ModelManager::GetInstance()->LoadModel("jet.obj");		// モデルは事前にロードしておく
	ModelManager::GetInstance()->LoadModel("axisPlus.obj"); // 弾のモデル
	ModelManager::GetInstance()->LoadModel("ground.obj");	// 地形のモデル
	ModelManager::GetInstance()->LoadModel("skydome.obj");	// 地面のモデルもロード
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
	titleSprite_->Initialize(spriteSetup, "WolfOne_Triangle.png");
	titleSprite_->SetPosition({100.0f, 100.0f});
	titleSprite_->SetSize({200.0f, 200.0f});

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
}

///=============================================================================
///						終了処理
void TitleScene::Finalize() {
}

///=============================================================================
///						更新
void TitleScene::Update() {
	//========================================
	// 曲を再生

	//========================================
	// Object3D

	//========================================
	// Sprite
	if (titleSprite_) {
		titleSprite_->Update();
	}

	//=========================================
	// Skybox
	if (skybox_) {
		skybox_->Update();
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

		//========================================
		// 【1】オープニング：ゆっくり上昇
		if (titleCamera_ && titleCamera_->GetCurrentPhase() == TitleCameraPhase::Opening) {
			// 緩やかに上昇＋前進
			playerTransform->translate.y += 0.1f;
			playerTransform->translate.z += 0.15f;

			// 機体をやや上向きに
			playerTransform->rotate.x = -0.15f;
		}

		//========================================
		// 【2】ヒーローショット：上昇を継続
		if (titleCamera_ && titleCamera_->GetCurrentPhase() == TitleCameraPhase::HeroShot) {
			// 上昇速度を少し上げる
			playerTransform->translate.y += 0.2f;
			playerTransform->translate.z += 0.25f;

			// 機体を少し上向きに
			playerTransform->rotate.x = -0.2f;
		}

		//========================================
		// 【3】タイトル表示：安定した上昇
		if (titleCamera_ && titleCamera_->GetCurrentPhase() == TitleCameraPhase::TitleDisplay) {
			// 安定した上昇
			playerTransform->translate.y += 0.15f;
			playerTransform->translate.z += 0.2f;

			// 機体を水平に近づける
			playerTransform->rotate.x += (-0.1f - playerTransform->rotate.x) * 0.1f;
		}

		//========================================
		// 【4】ループ：ゆっくり旋回
		if (titleCamera_ && titleCamera_->GetCurrentPhase() == TitleCameraPhase::Loop) {
			static float loopTimer = 0.0f;
			loopTimer += 1.0f / 60.0f;

			// ゆっくり前進
			playerTransform->translate.z += 0.1f;

			// 時折ゆっくりロール
			static float rollTimer = 0.0f;
			rollTimer += 1.0f / 60.0f;

			if (rollTimer > 5.0f) {
				float rollAmount = std::sin((rollTimer - 5.0f) * 0.5f) * 0.2f;
				playerTransform->rotate.z += (rollAmount - playerTransform->rotate.z) * 0.05f;
			}

			// 機体は水平を維持
			playerTransform->rotate.x += (-0.05f - playerTransform->rotate.x) * 0.1f;
		}

		player_->GetObject3d()->Update();
	}

	//========================================
	// シーン遷移
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneNo = SCENE::GAMEPLAY;
	}
	// コントローラ
	if (Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
		sceneNo = SCENE::GAMEPLAY;
	}
	// シーンリセット
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		sceneNo = SCENE::GAMEPLAY;
		sceneNo = SCENE::TITLE;
	}
}

///=============================================================================
///						2D描画
void TitleScene::Object2DDraw() {
	if (titleSprite_) {
		titleSprite_->Draw();
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
///						パーティクル描画
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

	if (player_) {
		player_->DrawImGui();
	}
#endif // DEBUG
}
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
	// カメラ設定
	CameraManager::GetInstance()->GetCamera("DefaultCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.2f, 0.0f, 0.0f}, {0.0f, 4.0f, -16.0f}});

	//========================================
	// 音の読み込み

	//========================================
	// モデルの読み込み

	//========================================
	// タイトルオブジェクト

	//========================================
	// スプライトクラス

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

	//=========================================
	// Skybox
	if (skybox_) {
		skybox_->Update();
	}

	//========================================
	// シーン遷移
	if (Input::GetInstance()->TriggerKey(VK_SPACE)) {
		sceneNo = SCENE::GAMEPLAY;
	}
	// コントローラ
	if (Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
		sceneNo = SCENE::GAMEPLAY;
	}
}

///=============================================================================
///						2D描画
void TitleScene::Object2DDraw() {
}

///=============================================================================
///						3D描画
void TitleScene::Object3DDraw() {
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
#endif // DEBUG
}

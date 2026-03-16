/*********************************************************************
 * \file   ClearScene.cpp
 * \brief  クリアシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#include "ClearScene.h"
#include "Input.h"

///=============================================================================
/// 初期化
void ClearScene::Initialize(SceneContext *context) {
	// NOTE: 引数が1つに削減された
	// NOTE: contextから必要なセットアップにアクセスする場合は
	//       context->GetXxxSetup()で取得できる

	// 使用しない場合は何もしなくてもよい
	context;
}

///=============================================================================
///						終了処理
void ClearScene::Finalize() {
}

///=============================================================================
///						更新
void ClearScene::Update() {
	//========================================
	// シーン遷移
	if (MagEngine::Input::GetInstance()->PushKey(VK_SPACE)) {
		SetSceneNo(TITLE);
	}
	// コントローラ
	if (MagEngine::Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
		SetSceneNo(TITLE);
	}
}

///=============================================================================
///						2D描画
void ClearScene::Object2DDraw() {
}

///=============================================================================
///						3D描画
void ClearScene::Object3DDraw() {
}

///=============================================================================
///						パーティクル描画
void ClearScene::ParticleDraw() {
}

///=============================================================================
///						Skybox描画
void ClearScene::SkyboxDraw() {
}

///=============================================================================
///						Cloud描画
void ClearScene::CloudDraw() {
}

///=============================================================================
///						TrailEffect描画
void ClearScene::TrailEffectDraw() {
}

///=============================================================================
///						ImGui描画
void ClearScene::ImGuiDraw() {
#ifdef DEBUG
	// ClearSceneのImGui描画
	ImGui::Begin("ClearScene");
	ImGui::Text("Hello, ClearScene!");
	ImGui::End();
#endif // DEBUG
}

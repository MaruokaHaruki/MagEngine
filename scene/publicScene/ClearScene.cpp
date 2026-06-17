/*********************************************************************
 * \file   ClearScene.cpp
 * \brief  クリアシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#include "ClearScene.h"
#include "EngineContext.h"
#include "Input.h"
#include "SceneContext.h"
#include <cassert>

///=============================================================================
/// 初期化
void ClearScene::Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) {
	engineContext.Validate();
	engineContext_ = &engineContext;
	sceneContext_ = &sceneContext;
}

///=============================================================================
///						終了処理
void ClearScene::Finalize() {
}

///=============================================================================
///						更新
void ClearScene::Update() {
	assert(engineContext_);
	MagEngine::Input *input = engineContext_->input;

	//========================================
	// シーン遷移
	if (input->PushKey(VK_SPACE)) {
		SetSceneNo(TITLE);
	}
	// コントローラ
	if (input->TriggerButton(XINPUT_GAMEPAD_A)) {
		SetSceneNo(TITLE);
	}
}

///=============================================================================
///						2D描画
void ClearScene::Object2DDraw() {
}

///=============================================================================
///						3D不透明描画対象の登録
void ClearScene::RegisterRenderables(MagEngine::RenderWorld &) {
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

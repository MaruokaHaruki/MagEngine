#include "SceneManager.h"
#include "ImguiSetup.h"
// public:
#include "ClearScene.h"
#include "GamePlayScene.h"
#include "TitleScene.h"
// private:
#include "DebugScene.h"

///=============================================================================
///						初期化
void SceneManager::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup,
							  SkyboxSetup *skyboxSetup) {
	//========================================
	// 2D共通部
	spriteSetup_ = spriteSetup;
	// 3D共通部
	object3dSetup_ = object3dSetup;
	// パーティクル共通部
	particleSetup_ = particleSetup;
	// Skybox共通部
	skyboxSetup_ = skyboxSetup;
	// 初期シーンを設定（例としてDebugSceneを設定）
	nowScene_ = std::make_unique<TitleScene>();
	nowScene_->Initialize(spriteSetup_, object3dSetup_, particleSetup_, skyboxSetup_);

	// シーンの初期設定
	currentSceneNo_ = 0;
	prevSceneNo_ = -1;
}

///=============================================================================
///						終了処理
void SceneManager::Finalize() {
	if (nowScene_) {
		nowScene_->Finalize();
	}
}

///=============================================================================
///						更新
void SceneManager::Update() {
	//========================================
	// シーンの切り替え
	prevSceneNo_ = currentSceneNo_;
	currentSceneNo_ = nowScene_->GetSceneNo();

	//========================================
	// シーンが切り替わった場合
	if (prevSceneNo_ != currentSceneNo_) {
		if (nowScene_) {
			// 現在のシーンの終了処理
			nowScene_->Finalize();
		}
		// シーンの生成
		nowScene_ = sceneFactory_->CreateScene(currentSceneNo_);
		// シーンの初期化
		nowScene_->Initialize(spriteSetup_, object3dSetup_, particleSetup_, skyboxSetup_);
	}

	//========================================
	// シーンの更新
	if (nowScene_) {
		nowScene_->Update();
	}
}

///=============================================================================
///						2D描画
void SceneManager::Object2DDraw() {
	if (nowScene_) {
		nowScene_->Object2DDraw();
	}
}

///=============================================================================
///						3D描画
void SceneManager::Object3DDraw() {
	if (nowScene_) {
		nowScene_->Object3DDraw();
	}
}

///=============================================================================
///						パーティクル描画
void SceneManager::ParticleDraw() {
	if (nowScene_) {
		nowScene_->ParticleDraw();
	}
}

///=============================================================================
///						Skybox描画
void SceneManager::SkyboxDraw() {
	if (nowScene_) {
		nowScene_->SkyboxDraw();
	}
}

///=============================================================================
///						Cloud描画
void SceneManager::CloudDraw() {
	if (nowScene_) {
		nowScene_->CloudDraw();
	}
}

///=============================================================================
///						ImGui描画
void SceneManager::ImGuiDraw() {
	//========================================

	if (nowScene_) {
		nowScene_->ImGuiDraw();
	}
	//========================================
	// シーンを切り替えるボタン
	// publicScene
	ImGui::Begin("SceneChange");
	ImGui::Text("publicScene");
	if (ImGui::Button("TitleScene")) {
		nowScene_->SetSceneNo(TITLE);
	}
	if (ImGui::Button("GamePlayScene")) {
		nowScene_->SetSceneNo(GAMEPLAY);
	}
	if (ImGui::Button("ClearScene")) {
		nowScene_->SetSceneNo(CLEAR);
	}
	//========================================
	// privateScene
	ImGui::Separator();
	ImGui::Text("privateScene");
	if (ImGui::Button("DebugScene")) {
		nowScene_->SetSceneNo(DEBUG);
	}

	ImGui::End();
}

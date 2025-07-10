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
void SceneManager::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup) {
	//========================================
	// 2D共通部
	spriteSetup_ = spriteSetup;
	// 3D共通部
	object3dSetup_ = object3dSetup;
	// パーティクル共通部
	particleSetup_ = particleSetup;

	// 初期シーン番号をBaseScene::GetCurrentSceneNo()で取得
	currentSceneNo_ = BaseScene::GetCurrentSceneNo(); // これはDEBUG（0）
	prevSceneNo_ = -1;								  // 異なる値にして最初のUpdateでシーン作成をトリガーする

	// nowScene_はnullptrのままにして、Updateで作成させる
	nowScene_ = nullptr;
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
	// 初回実行時またはシーンの切り替え
	if (nowScene_) {
		prevSceneNo_ = currentSceneNo_;
		currentSceneNo_ = nowScene_->GetSceneNo();
	} else {
		// 初回実行時：nowScene_がnullptrの場合
		currentSceneNo_ = BaseScene::GetCurrentSceneNo();
	}

	//========================================
	// シーンが切り替わった場合または初回実行時
	if (prevSceneNo_ != currentSceneNo_) {
		if (nowScene_) {
			// 現在のシーンの終了処理
			nowScene_->Finalize();
		}
		// シーンの生成
		nowScene_ = sceneFactory_->CreateScene(currentSceneNo_);
		// シーンの初期化
		nowScene_->Initialize(spriteSetup_, object3dSetup_, particleSetup_);
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

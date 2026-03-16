/*********************************************************************
 * \file   SceneManager.cpp
 * \brief  シーン管理実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップを統一管理
 *         NOTE: シーン遷移は各シーンが設定したnextSceneNo_から判定
 *********************************************************************/
#include "SceneManager.h"
#include "ImguiSetup.h"
#include "SceneFactory.h"
// public:
#include "ClearScene.h"
#include "GamePlayScene.h"
#include "TitleScene.h"
// private:
#include "DebugScene.h"

///=============================================================================
/// NOTE: SceneContextにセットアップを設定して一元管理
/// NOTE: ファクトリーパターンで初期シーンも生成
void SceneManager::Initialize(MagEngine::SpriteSetup *spriteSetup,
							  MagEngine::Object3dSetup *object3dSetup,
							  MagEngine::ParticleSetup *particleSetup,
							  MagEngine::SkyboxSetup *skyboxSetup,
							  MagEngine::CloudSetup *cloudSetup,
							  MagEngine::TrailEffectSetup *trailEffectSetup,
							  MagEngine::TrailEffectManager *trailEffectManager) {
	//========================================
	// NOTE: SceneContextにすべてのセットアップを設定
	sceneContext_.SetSpriteSetup(spriteSetup);
	sceneContext_.SetObject3dSetup(object3dSetup);
	sceneContext_.SetParticleSetup(particleSetup);
	sceneContext_.SetSkyboxSetup(skyboxSetup);
	sceneContext_.SetCloudSetup(cloudSetup);
	sceneContext_.SetTrailEffectSetup(trailEffectSetup);
	sceneContext_.SetTrailEffectManager(trailEffectManager);

	//========================================
	// NOTE: 互換性のため、ローカル変数にも保存
	// NOTE: 将来的にはこれらは削除可能
	spriteSetup_ = spriteSetup;
	object3dSetup_ = object3dSetup;
	particleSetup_ = particleSetup;
	skyboxSetup_ = skyboxSetup;
	cloudSetup_ = cloudSetup;
	trailEffectSetup_ = trailEffectSetup;
	trailEffectManager_ = trailEffectManager;

	//========================================
	// NOTE: ファクトリーが設定されていない場合は生成
	if (!sceneFactory_) {
		// NOTE: デフォルトファクトリーを生成（この方法は改良の余地あり）
		static SceneFactory defaultFactory;
		sceneFactory_ = &defaultFactory;
	}

	// NOTE: 初期シーンをFactoryで生成
	nowScene_ = sceneFactory_->CreateScene(SCENE::TITLE, &sceneContext_);

	// シーンの初期設定
	currentSceneNo_ = SCENE::TITLE;
	prevSceneNo_ = -1;
}

///=============================================================================
/// 終了処理
void SceneManager::Finalize() {
	if (nowScene_) {
		nowScene_->Finalize();
	}
}

///=============================================================================
/// NOTE: シーン番号はnowScene_->nextSceneNo_から取得
/// NOTE: nextSceneNo_ == -1の場合はシーン遷移なし
void SceneManager::Update() {
	//========================================
	// NOTE: 前のシーン番号を保存
	prevSceneNo_ = currentSceneNo_;
	// NOTE: 次のシーン番号をシーンから取得
	currentSceneNo_ = nowScene_->GetSceneNo();

	//========================================
	// NOTE: シーン遷移判定（-1の場合は遷移しない）
	if (prevSceneNo_ != currentSceneNo_ && currentSceneNo_ != -1) {
		if (nowScene_) {
			// 現在のシーンの終了処理
			nowScene_->Finalize();
		}
		// NOTE: ファクトリーでシーンをSceneContextと共に生成
		nowScene_ = sceneFactory_->CreateScene(currentSceneNo_, &sceneContext_);
	}

	//========================================
	// シーンの更新
	if (nowScene_) {
		nowScene_->Update();
	}
}

///=============================================================================
/// 2D描画
void SceneManager::Object2DDraw() {
	if (nowScene_) {
		nowScene_->Object2DDraw();
	}
}

///=============================================================================
/// 3D描画
void SceneManager::Object3DDraw() {
	if (nowScene_) {
		nowScene_->Object3DDraw();
	}
}

///=============================================================================
/// パーティクル描画
void SceneManager::ParticleDraw() {
	if (nowScene_) {
		nowScene_->ParticleDraw();
	}
}

///=============================================================================
/// Skybox描画
void SceneManager::SkyboxDraw() {
	if (nowScene_) {
		nowScene_->SkyboxDraw();
	}
}

///=============================================================================
/// Cloud描画
void SceneManager::CloudDraw() {
	if (nowScene_) {
		nowScene_->CloudDraw();
	}
}

///=============================================================================
/// TrailEffect描画
void SceneManager::TrailEffectDraw() {
	if (nowScene_) {
		nowScene_->TrailEffectDraw();
	}
}

///=============================================================================
/// ImGui描画
void SceneManager::ImGuiDraw() {
	//========================================

	if (nowScene_) {
		nowScene_->ImGuiDraw();
	}
	//========================================
	// NOTE: シーンを切り替えるボタン
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

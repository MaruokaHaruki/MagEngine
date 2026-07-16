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
#include "DirectXCore.h"
#include "EditorUiSystem.h"
#include "EngineContext.h"
#include "SceneFactory.h"
#include "SpriteSetup.h"
#include "Logger.h"
// public:
#include "ClearScene.h"
#include "GamePlayScene.h"
#include "TitleScene.h"
// private:
#include "DebugScene.h"
#include <cassert>
#include <exception>
#include <stdexcept>

///=============================================================================
/// NOTE: SceneContextにセットアップを設定して一元管理
/// NOTE: ファクトリーパターンで初期シーンも生成
void SceneManager::Initialize(const MagEngine::EngineContext &engineContext) {
	//========================================
	// NOTE: Sceneの旧Singletonフォールバックを禁止するため、初期化時点で必須依存を検証
	engineContext.Validate();
	engineContext_ = &engineContext;

	//========================================
	// NOTE: SceneContextはEngineサービスを持たず、Scene管理情報だけを扱う
	sceneContext_.SetSceneManager(this);

	//========================================
	// NOTE: ファクトリーが設定されていない場合は生成
	if (!sceneFactory_) {
		// NOTE: デフォルトファクトリーを生成（この方法は改良の余地あり）
		static SceneFactory defaultFactory;
		sceneFactory_ = &defaultFactory;
	}

	// NOTE: 初期Sceneには遷移元が存在しないため、失敗理由を記録して初期化失敗を上位へ通知する。
	std::string initialSceneError;
	if (!CreateAndInitializeCandidateScene(SCENE::TITLE, nowScene_, initialSceneError)) {
		DiscardCandidateScene(nowScene_);
		throw std::runtime_error(initialSceneError);
	}

	engineContext_->editorUiSystem->RegisterPanel("Scene Switcher", MagEngine::EditorUiCategory::Tools, true, [this]() {
		if (!nowScene_) {
			return;
		}
		ImGui::Text("Public Scene");
		if (ImGui::Button("TitleScene")) {
			nowScene_->SetSceneNo(TITLE);
		}
		if (ImGui::Button("GamePlayScene")) {
			nowScene_->SetSceneNo(GAMEPLAY);
		}
		if (ImGui::Button("ClearScene")) {
			nowScene_->SetSceneNo(CLEAR);
		}
		ImGui::Separator();
		ImGui::Text("Private Scene");
		if (ImGui::Button("DebugScene")) {
			nowScene_->SetSceneNo(DEBUG);
		}
		if (!lastSceneInitializationError_.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("Last scene initialization error: %s", lastSceneInitializationError_.c_str());
		}
	});

	// シーンの初期設定
	currentSceneNo_ = SCENE::TITLE;
	prevSceneNo_ = -1;
}

///=============================================================================
/// 終了処理
void SceneManager::Finalize() {
	if (engineContext_ && engineContext_->graphics) {
		engineContext_->graphics->WaitForGpuIdle();
	}

	if (nowScene_) {
		nowScene_->Finalize();
	}
}

///=============================================================================
/// NOTE: シーン番号はnowScene_->nextSceneNo_から取得
/// NOTE: nextSceneNo_ == -1の場合はシーン遷移なし
void SceneManager::Update(const FrameTime &frameTime) {
	if (!nowScene_) {
		return;
	}

	const int requestedSceneNo = nowScene_->GetSceneNo();
	if (requestedSceneNo != -1 && requestedSceneNo != currentSceneNo_) {
		if (engineContext_ && engineContext_->graphics) {
			engineContext_->graphics->WaitForGpuIdle();
		}

		std::unique_ptr<BaseScene> candidateScene;
		std::string errorMessage;
		if (!CreateAndInitializeCandidateScene(requestedSceneNo, candidateScene, errorMessage)) {
			DiscardCandidateScene(candidateScene);
			HandleSceneChangeFailure(requestedSceneNo, errorMessage);
		} else {
#ifdef _DEBUG
			engineContext_->editorUiSystem->ClearScenePanels();
#endif
			nowScene_->Finalize();
			nowScene_ = std::move(candidateScene);
			currentSceneNo_ = requestedSceneNo;
			prevSceneNo_ = currentSceneNo_;
			lastSceneInitializationError_.clear();
#ifdef _DEBUG
			nowScene_->RegisterEditorPanels();
#endif
		}
	}

	//========================================
	// シーンの更新
	if (nowScene_) {
		nowScene_->Update(frameTime);
	}
}

///=============================================================================
/// 3D不透明描画対象の登録
void SceneManager::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	if (nowScene_) {
		nowScene_->RegisterRenderables(renderWorld);
	}
}

///=============================================================================
bool SceneManager::CreateAndInitializeCandidateScene(int sceneNo, std::unique_ptr<BaseScene> &candidateScene, std::string &errorMessage) {
	try {
		if (sceneNo == SCENE::GAMEPLAY) {
			const StageLoadResult stageLoadResult = GamePlayScene::LoadDefaultStageDefinition();
			if (!stageLoadResult.IsSuccess()) {
				errorMessage = "Stage preflight failed: " + stageLoadResult.errorMessage;
				return false;
			}
		}

		candidateScene = sceneFactory_->CreateScene(sceneNo);
		if (!candidateScene) {
			errorMessage = "SceneFactory returned null for " + std::string(GetSceneName(sceneNo)) + ".";
			return false;
		}

		candidateScene->Initialize(*engineContext_, sceneContext_);
		return true;
	} catch (const std::exception &exception) {
		errorMessage = exception.what();
	} catch (...) {
		errorMessage = "Unknown scene initialization failure.";
	}
	return false;
}

///=============================================================================
void SceneManager::DiscardCandidateScene(std::unique_ptr<BaseScene> &candidateScene) {
	if (!candidateScene) {
		return;
	}

#ifdef _DEBUG
	engineContext_->editorUiSystem->ClearScenePanels();
#endif
	try {
		// 理由：部分初期化済みのSceneが登録したColliderやコールバックを、実体破棄前に解除するため。
		candidateScene->Finalize();
	} catch (const std::exception &exception) {
		Logger::Log("Candidate scene finalization failed: " + std::string(exception.what()), Logger::LogLevel::Error);
	} catch (...) {
		Logger::Log("Candidate scene finalization failed with an unknown exception.", Logger::LogLevel::Error);
	}
	candidateScene.reset();
}

///=============================================================================
void SceneManager::HandleSceneChangeFailure(int requestedSceneNo, const std::string &errorMessage) {
	lastSceneInitializationError_ = "Scene transition " + std::string(GetSceneName(currentSceneNo_)) + " -> " + GetSceneName(requestedSceneNo) + " failed: " + errorMessage;
	Logger::Log(lastSceneInitializationError_, Logger::LogLevel::Error);

	// 理由：失敗した要求を残すと毎フレーム同じ候補Sceneを生成してしまうため。
	nowScene_->SetSceneNo(-1);
	nowScene_->OnSceneChangeFailed(requestedSceneNo, lastSceneInitializationError_);
#ifdef _DEBUG
	nowScene_->RegisterEditorPanels();
#endif
}

///=============================================================================
const char *SceneManager::GetSceneName(int sceneNo) {
	switch (sceneNo) {
	case SCENE::DEBUG:
		return "Debug";
	case SCENE::TITLE:
		return "Title";
	case SCENE::GAMEPLAY:
		return "GamePlay";
	case SCENE::CLEAR:
		return "Clear";
	default:
		return "Unknown";
	}
}


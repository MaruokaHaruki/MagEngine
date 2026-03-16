/*********************************************************************
 * \file   SceneFactory.cpp
 * \brief  具体的なシーン工場実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: 各シーンの生成と初期化を行う
 *         NOTE: SceneContextでセットアップ類をまとめて渡している
 *********************************************************************/
#include "SceneFactory.h"
#include "Logger.h"
#include "SceneContext.h"
#include <memory>
using namespace Logger;
//========================================
// privateシーン
#include "DebugScene.h"
// publicシーン
#include "ClearScene.h"
#include "GameClearAnimation.h"
#include "GamePlayScene.h"
#include "TitleScene.h"

///=============================================================================
/// NOTE: SceneContextを使用してシーンを生成
///       これにより7個の引数を1つに削減
std::unique_ptr<BaseScene> SceneFactory::CreateScene(int sceneNo, SceneContext *context) {
	//========================================
	// 次のシーンの生成
	std::unique_ptr<BaseScene> nextScene = nullptr;

	//========================================
	// シーン名によって生成するシーンを変更
	if (sceneNo == SCENE::DEBUG) {
		nextScene = std::make_unique<DebugScene>();
	} else if (sceneNo == SCENE::GAMEPLAY) {
		nextScene = std::make_unique<GamePlayScene>();
	} else if (sceneNo == SCENE::TITLE) {
		nextScene = std::make_unique<TitleScene>();
	} else if (sceneNo == SCENE::CLEAR) {
		nextScene = std::make_unique<ClearScene>();
	} else {
		Log("シーン名が不正です", LogLevel::Error);
		return nextScene;
	}

	//========================================
	// NOTE: contextがnullptrでないかチェック
	if (!context) {
		Log("SceneContext is nullptr", LogLevel::Error);
		return nextScene;
	}

	//========================================
	// シーンの初期化 - NOTE: 引数がcontextの1つに統一
	nextScene->Initialize(context);

	//========================================
	// シーンを返す
	return nextScene;
}

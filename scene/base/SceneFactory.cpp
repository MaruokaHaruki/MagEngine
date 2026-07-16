/*********************************************************************
 * \file   SceneFactory.cpp
 * \brief  具体的なシーン工場実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: 各シーンの生成を行う
 *         NOTE: 初期化はSceneManagerが候補Sceneを保持したまま実行する
 *********************************************************************/
#include "SceneFactory.h"
#include "Logger.h"
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
/// NOTE: 生成と初期化を分離することで、初期化失敗時も遷移元Sceneを維持できる。
std::unique_ptr<BaseScene> SceneFactory::CreateScene(int sceneNo) {
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
	// シーンを返す
	return nextScene;
}

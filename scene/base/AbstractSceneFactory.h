/*********************************************************************
 * \file   AbstractSceneFactory.h
 * \brief  シーン生成用の抽象ファクトリー
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: シーンの生成を統一的に管理する
 *         NOTE: 引数をSceneContextに統一されている
 *********************************************************************/
#pragma once
#include "BaseScene.h"
#include <memory>

// Forward declaration
class SceneContext;
namespace MagEngine {
	struct EngineContext;
}

///=============================================================================
///                         抽象シーンファクトリ
/// NOTE: SceneContextを使用することで、生成時の引数を削減している
class AbstractSceneFactory {
public:
	virtual ~AbstractSceneFactory() = default;

	/// @brief シーンを作成する純粋仮想関数
	/// @param sceneNo シーン番号
	/// @param engineContext シーンが使用するEngineサービス
	/// @param sceneContext Scene管理用コンテキスト
	/// @return 作成されたシーン
	virtual std::unique_ptr<BaseScene> CreateScene(int sceneNo, const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) = 0;
};

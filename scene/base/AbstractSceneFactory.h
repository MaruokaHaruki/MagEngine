/*********************************************************************
 * \file   AbstractSceneFactory.h
 * \brief  シーン生成用の抽象ファクトリー
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: シーンの生成を統一的に管理する
 *         NOTE: 初期化はSceneManagerが候補Sceneの状態を管理して実行する
 *********************************************************************/
#pragma once
#include "BaseScene.h"
#include <memory>

///=============================================================================
///                         抽象シーンファクトリ
/// NOTE: Factoryは副作用を伴う初期化を担当しない
class AbstractSceneFactory {
public:
	virtual ~AbstractSceneFactory() = default;

	/// @brief シーンを作成する純粋仮想関数
	/// @param sceneNo シーン番号
	/// @return 作成されたシーン
	virtual std::unique_ptr<BaseScene> CreateScene(int sceneNo) = 0;
};

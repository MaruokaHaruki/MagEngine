/*********************************************************************
 * \file   SceneFactory.h
 * \brief  具体的なシーン工場実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: AbstractSceneFactoryの具体実装
 *         NOTE: 各シーンのインスタンス化を行う
 *********************************************************************/
#pragma once
#include "AbstractSceneFactory.h"

///=============================================================================
///                         シーン工場
/// NOTE: シーンの生成を一元管理する
class SceneFactory : public AbstractSceneFactory {
	///--------------------------------------------------------------
	///                            メンバ関数
	/**----------------------------------------------------------------------------
	 * \brief  CreateScene シーンの生成
	 * \param  sceneNo シーン番号
	 * \return シーン
	 */
	std::unique_ptr<BaseScene> CreateScene(int sceneNo) override;
};

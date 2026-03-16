/*********************************************************************
 * \file   BaseScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   NOTE: sceneNoの管理をインスタンスごとに変更
 *         NOTE: nextSceneNo_はデフォルト値-1で初期化される
 *********************************************************************/
#include "BaseScene.h"

// NOTE: 静的メンバー変数を削除
//       各シーンインスタンスがnextSceneNo_を持つようになったため不要

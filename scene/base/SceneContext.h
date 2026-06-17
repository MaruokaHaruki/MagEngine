/*********************************************************************
 * \file   SceneContext.h
 * \brief  Scene固有情報とScene管理情報を扱うコンテキスト
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: EngineサービスはEngineContextへ分離し、このContextには重複して持たせない
 *********************************************************************/
#pragma once

class SceneManager;

///=============================================================================
///                         シーンコンテキスト
/// NOTE: Scene遷移やScene間情報の置き場所として残し、Engineサービスは持たない。
class SceneContext {
public:
	SceneContext() = default;
	~SceneContext() = default;

	/// @brief SceneManagerを設定
	void SetSceneManager(SceneManager *sceneManager) {
		sceneManager_ = sceneManager;
	}

	/// @brief SceneManagerを取得
	SceneManager *GetSceneManager() const {
		return sceneManager_;
	}

private:
	// 処理内容：SceneManagerへの非所有参照を保持する
	// 理由：SceneContextの責務をScene管理系に限定し、EngineContextとの重複を避けるため
	SceneManager *sceneManager_ = nullptr;
};

#include "SceneTransition.h"

///=============================================================================
///                        初期化
void SceneTransition::Initialize() {
	currentSceneID_ = 0;
	nextSceneID_ = -1;
	isTransitioning_ = false;
}

///=============================================================================
///                        終了処理
void SceneTransition::Finalize() {
}

///=============================================================================
///                        更新
void SceneTransition::Update() {
	// シーン切り替え中の処理
}

///=============================================================================
///                        シーン切り替え要求
void SceneTransition::RequestSceneChange(int sceneID) {
}
/*********************************************************************
 * \file   TitleScene.h
 * \brief  タイトルシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "BaseScene.h"
#include "MagMath.h"
#include <memory>
#include <vector>
//========================================
// Application
#include "Cloud.h"
#include "CloudSetup.h"
#include "CollisionManager.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "HUD.h"
#include "Player.h"
#include "SceneTransition.h"
#include "Skydome.h"
#include "Skybox.h"
#include "Sprite.h"
#include "TitleCamera.h"

// Forward declaration
class SceneContext;
namespace MagEngine {
	struct EngineContext;
}

///=============================================================================
///                         タイトルシーンクラス
class TitleScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) override;
	void Finalize() override;

	void Update() override;

	/// \brief 描画対象登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld) override;

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// EngineContext
	const MagEngine::EngineContext *engineContext_ = nullptr;
	SceneContext *sceneContext_ = nullptr;

	//========================================
	// オブジェクト
	std::unique_ptr<Player> player_;

	//========================================
	// カメラ
	std::unique_ptr<TitleCamera> titleCamera_;

	//========================================
	// スプライト
	std::unique_ptr<MagEngine::Sprite> titleSprite_;
	std::unique_ptr<MagEngine::Sprite> pressEnterSprite_;
	MagMath::Vector2 titleSpriteBaseSize_; // タイトルスプライトの基本サイズ
	MagMath::Vector2 pressEnterBaseSize_;  // Press Enterスプライトの基本サイズ
	//========================================
	// 演出用変数
	float blinkTimer_ = 0.0f;
	float pressEnterAlpha_ = 1.0f;
	bool isFadingOut_ = true;
	float totalElapsedTime_ = 0.0f; // シーン全体の経過時間

	//========================================
	// スカイボックス
	std::unique_ptr<MagEngine::Skybox> skybox_;

	//========================================
	// 雲
	std::unique_ptr<MagEngine::Cloud> cloud_;

	//========================================
	// トランジション
	std::unique_ptr<SceneTransition> sceneTransition_;
};

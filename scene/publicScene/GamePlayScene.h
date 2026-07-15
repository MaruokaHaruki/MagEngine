/*********************************************************************
 * \file   GamePlayScene.h
 * \brief  ゲームプレイシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>
//========================================
// Engine
#include "Cloud.h"
#include "CloudSetup.h"
#include "Object3d.h"
#include "Object3dSetup.h"
#include "Particle.h"
#include "ParticleSetup.h"
#include "Skybox.h"
#include "SkyboxSetup.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include "stage/EnemyStageSystem.h"
#ifdef _DEBUG
#include "gameplay/GamePlayDebugController.h"
#endif

//========================================
// Game
#include "UIManager.h"

//========================================
// Forward declaration
class CollisionManager;
class FollowCamera;
class Skydome;
class Player;
class EnemyManager;
class EnemyBullet;
class SceneTransition;
class SceneContext;
namespace MagEngine {
	struct EngineContext;
}

///=============================================================================
///                         ゲームプレイシーンクラス
class GamePlayScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) override;
	void Finalize() override;

	void Update(float deltaTime) override;

	/// \brief 描画対象登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld) override;

	///--------------------------------------------------------------
	///							静的メンバ関数
	///--------------------------------------------------------------
	///							入出力関数
public:
	void RegisterEditorPanels();

private:
	void DrawDebugUi();
	void DrawPlayerDebugUi();
	void DrawEnemyDebugUi();
	void DrawCloudDebugUi();
	void DrawCollisionDebugUi();
	void DrawUiDebugUi();
	void DrawTransitionDebugUi();

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// EngineContext
	const MagEngine::EngineContext *engineContext_ = nullptr;
	SceneContext *sceneContext_ = nullptr;

	//========================================
	// 当たり判定
	std::unique_ptr<CollisionManager> collisionManager_;

	//========================================
	// カメラ
	std::unique_ptr<FollowCamera> followCamera_;

	//=========================================
	// スカイドーム
	std::unique_ptr<Skydome> skydome_;

	//========================================
	// プレイヤー
	std::unique_ptr<Player> player_;

	//========================================
	// 敵
	std::unique_ptr<EnemyManager> enemyManager_;
	std::vector<EnemyBullet *> enemyBulletBuffer_;
	GameFlowController gameFlowController_;

	//========================================
	// スプライト
	std::unique_ptr<MagEngine::Sprite> moveSprite_;

	//========================================
	// パーティクル
	std::unique_ptr<MagEngine::Particle> particle_;

	//========================================
	// 雲
	std::unique_ptr<MagEngine::Cloud> cloud_;

	//=========================================
	// Skybox
	std::unique_ptr<MagEngine::Skybox> skybox_;

	//========================================
	// トランジション
	std::unique_ptr<SceneTransition> sceneTransition_;

	//========================================
	// UI管理
	std::unique_ptr<UIManager> uiManager_;

#ifdef _DEBUG
	GamePlayDebugController debugController_;
#endif

	//========================================
	// ゲーム状態
	bool isGameOver_;
	bool isGameClear_;
	bool hasUIDeploymentStarted_ = false; // UI展開開始フラグ

	//========================================
	// タイムスケール（ジャスト回避スロー効果用）
	float gameTimeScale_ = 1.0f; // デフォルト: 1.0x（通常速度）

};

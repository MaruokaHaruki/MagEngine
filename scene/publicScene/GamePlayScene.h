/*********************************************************************
 * \file   GamePlayScene.h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
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

//========================================
// Game
#include "GameClearAnimation.h"
#include "GameOverUI.h"
#include "OperationGuideUI.h"
#include "StartAnimation.h"

//========================================
// 前方宣言
class CollisionManager;
class FollowCamera;
class Skydome;
class Player;
class EnemyManager;
class HUD;
class SceneTransition;

///=============================================================================
///						ゲームプレイシーンクラス
class GamePlayScene : public BaseScene {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::SpriteSetup *spriteSetup,
					MagEngine::Object3dSetup *object3dSetup,
					MagEngine::ParticleSetup *particleSetup,
					MagEngine::SkyboxSetup *skyboxSetup,
					MagEngine::CloudSetup *cloudSetup) override;

	/// \brief 終了処理
	void Finalize() override;

	/// \brief 更新
	void Update() override;

	/// @brie 2D描画
	void Object2DDraw() override;

	/// \brief 3D描画
	void Object3DDraw() override;

	/// \brief パーティクル描画
	void ParticleDraw() override;

	/// \brief Skybox描画
	void SkyboxDraw() override;

	/// \brief Cloud描画
	void CloudDraw() override;

	/// \brief ImGui描画
	void ImGuiDraw() override;

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
	// HUD
	std::unique_ptr<HUD> hud_;

	//========================================
	// ゲームオーバー
	std::unique_ptr<GameOverUI> gameOverUI_;
	bool isGameOver_;

	//========================================
	// トランジション
	std::unique_ptr<SceneTransition> sceneTransition_;

	//========================================
	// スタートアニメーション
	std::unique_ptr<StartAnimation> startAnimation_;

	//========================================
	// クリアアニメーション`
	std::unique_ptr<GameClearAnimation> gameClearAnimation_;
	bool isGameClear_;

	//========================================
	// 操作ガイドUI
	std::unique_ptr<OperationGuideUI> operationGuideUI_;
};

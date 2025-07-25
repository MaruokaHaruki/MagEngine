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
// Game
#include "Camera.h"
#include "Cloud.h"
#include "CollisionManager.h"
#include "DebugTextManager.h"
#include "Enemy.h"
#include "FollowCamera.h"
#include "LineManager.h"
#include "MAudioG.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Skydome.h"
#include "Sprite.h"
#include "Skybox.h"

///=============================================================================
///						ゲームプレイシーンクラス
class GamePlayScene : public BaseScene {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup, SkyboxSetup *skyboxSetup) override;

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
	std::unique_ptr<Player> player_; // Object3dからPlayerに変更

	//========================================
	// 敵
	std::unique_ptr<Enemy> enemy_;

	//========================================
	// スプライト
	std::unique_ptr<Sprite> moveSprite_;
	//========================================
	// パーティクル
	std::unique_ptr<Particle> particle_;

	//========================================
	// 雲システム
	std::unique_ptr<Cloud> cloudSystem_;

	//=========================================
	// Skybox
	std::unique_ptr<Skybox> skybox_;
};

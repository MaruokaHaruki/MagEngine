/*********************************************************************
 * \file   TitleScene.h
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
// Application
#include "Camera.h"
#include "Cloud.h"
#include "CollisionManager.h"
#include "DebugTextManager.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "HUD.h"
#include "LineManager.h"
#include "MAudioG.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "SceneTransition.h"
#include "Skybox.h"
#include "Skydome.h"
#include "Sprite.h"
#include "TitleCamera.h"

///=============================================================================
///						タイトルシーンクラス
class TitleScene : public BaseScene {
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
	// オブジェクト
	std::unique_ptr<Player> player_;

	//========================================
	// カメラ
	std::unique_ptr<TitleCamera> titleCamera_;

	//========================================
	// スプライト
	std::unique_ptr<Sprite> titleSprite_;
	std::unique_ptr<Sprite> pressEnterSprite_;

	//========================================
	// 演出用変数
	float blinkTimer_ = 0.0f;
	float pressEnterAlpha_ = 1.0f;
	bool isFadingOut_ = true;

	//========================================
	// スカイボックス
	std::unique_ptr<Skybox> skybox_;

	//========================================
	// トランジション
	std::unique_ptr<SceneTransition> sceneTransition_;
};
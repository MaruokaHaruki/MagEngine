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
#include "TitleCamera.h"

// Forward declaration
class SceneContext;

///=============================================================================
///                         タイトルシーンクラス
class TitleScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(SceneContext *context) override;
	void Finalize() override;

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

	/// \brief TrailEffect描画
	void TrailEffectDraw() override;

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
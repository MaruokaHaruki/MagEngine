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
//========================================
// Game
#include "Camera.h"
#include "CollisionManager.h"
#include "MAudioG.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Player.h"
#include "Sprite.h"

///=============================================================================
///						ゲームプレイシーンクラス
class GamePlayScene : public BaseScene {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup) override;

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
	// 地面

	//========================================
	// プレイヤー
	// 3dオブジェクト
	std::unique_ptr<Player> player_; // Object3dからPlayerに変更

	//========================================
	// 　敵

	//========================================
	// スプライト
	std::unique_ptr<Sprite> moveSprite_;
	//========================================
	// パーティクル
	std::unique_ptr<Particle> particle_;
	// エミッター
	std::unique_ptr<ParticleEmitter> particleEmitter_;
};

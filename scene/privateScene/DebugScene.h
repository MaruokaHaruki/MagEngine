/*********************************************************************
 * \file   DebugScene.h
 * 
 * \author Harukichimaru
 * \date   January 2025
 * \note   
 *********************************************************************/
#pragma once
#include "BaseScene.h"
 //========================================
 // Game
#include "Camera.h"
#include "Sprite.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Object3d.h"
#include "Model.h"
#include "MAudioG.h"

class DebugScene : public BaseScene {
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
	// オーディオ
	MAudioG *audio_;

	//========================================
	// スプライト
	
	//========================================
	// 3dオブジェクト
	// モンスターボール
	std::unique_ptr<Object3d> objMonsterBall_;
	// 地面
	std::unique_ptr<Object3d> objTerrain_;
	//========================================
	// パーティクル
	std::unique_ptr<Particle> particle_;
	// パーティクルエミッター
	std::unique_ptr<ParticleEmitter> particleEmitter_;


	///--------------------------------------------------------------
	///						 アプリケーション固有
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
};


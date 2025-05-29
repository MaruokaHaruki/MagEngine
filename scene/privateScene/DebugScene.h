/*********************************************************************
 * \file   DebugScene.h
 * \brief
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
#include "MAudioG.h"
#include "Model.h"
#include "Object3d.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Sprite.h"

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
	std::unique_ptr<Particle> particle2_;
	std::unique_ptr<Particle> particle3_;
	// パーティクルエミッター
	std::unique_ptr<ParticleEmitter> particleEmitter_;
	// パーティクルエミッター2
	std::unique_ptr<ParticleEmitter> particleEmitter2_;
	// パーティクルエミッター3
	std::unique_ptr<ParticleEmitter> particleEmitter3_;

	// ヒットエフェクト用パーティクル
	std::unique_ptr<Particle> hitEffect_core_;		// コア（中心の爆発）
	std::unique_ptr<Particle> hitEffect_sparks_;	// 火花
	std::unique_ptr<Particle> hitEffect_smoke_;		// 煙
	std::unique_ptr<Particle> hitEffect_shockwave_; // 衝撃波

	// ヒットエフェクト用エミッター
	std::unique_ptr<ParticleEmitter> hitEmitter_core_;
	std::unique_ptr<ParticleEmitter> hitEmitter_sparks_;
	std::unique_ptr<ParticleEmitter> hitEmitter_smoke_;
	std::unique_ptr<ParticleEmitter> hitEmitter_shockwave_;

	///--------------------------------------------------------------
	///						 アプリケーション固有
	Transform transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	// ヒットエフェクト制御用
	bool triggerHitEffect_ = false;
	Vector3 hitEffectPosition_ = {0.0f, 0.0f, 0.0f};
	float hitEffectScale_ = 1.0f;
	Vector4 coreColor_ = {1.0f, 0.5f, 0.1f, 1.0f};		// オレンジ
	Vector4 sparksColor_ = {1.0f, 1.0f, 0.3f, 1.0f};	// 黄色
	Vector4 smokeColor_ = {0.3f, 0.3f, 0.3f, 0.8f};		// 灰色
	Vector4 shockwaveColor_ = {0.8f, 0.9f, 1.0f, 0.5f}; // 青白い

	// ループ制御用
	bool enableHitEffectLoop_ = false;	 // ループ有効フラグ
	float hitEffectLoopInterval_ = 2.0f; // ループ間隔（秒）
	float hitEffectLoopTimer_ = 0.0f;	 // ループタイマー
};

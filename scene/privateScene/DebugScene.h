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

private:
	/// \brief ハンマーアニメーション更新
	void UpdateHammerAnimation();

	/// \brief ハンマーヘッド位置計算
	Vector3 CalculateHammerHeadPosition();

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
	// スカイドーム
	std::unique_ptr<Object3d> objSkyDome_;
	// ハンマー
	std::unique_ptr<Object3d> objHammer_;
	// 電磁レンジ
	std::unique_ptr<Object3d> objMicrowave_;

	//========================================
	// ヒットエフェクト用パーティクル
	std::unique_ptr<Particle> hitEffect_core_;		// コア（中心の爆発）
	std::unique_ptr<Particle> hitEffect_sparks_;	// 火花
	std::unique_ptr<Particle> hitEffect_smoke_;		// 煙
	std::unique_ptr<Particle> hitEffect_shockwave_; // 衝撃波

	// ダイナミックヒットエフェクト用パーティクル
	std::unique_ptr<Particle> dynamicHit_flash_;	  // 瞬間フラッシュ
	std::unique_ptr<Particle> dynamicHit_spiral_;	  // 螺旋火花
	std::unique_ptr<Particle> dynamicHit_explosion_;  // 爆発コア
	std::unique_ptr<Particle> dynamicHit_rings_;	  // 連続衝撃波
	std::unique_ptr<Particle> dynamicHit_fragments_;  // 高速破片
	std::unique_ptr<Particle> dynamicHit_embers_;	  // 余燼
	std::unique_ptr<Particle> dynamicHit_distortion_; // 歪みエフェクト

	// アーク溶接エフェクト用パーティクル
	std::unique_ptr<Particle> welding_arc_core_;	  // 溶接アーク中心部
	std::unique_ptr<Particle> welding_sparks_hot_;	  // 高温火花
	std::unique_ptr<Particle> welding_sparks_fine_;	  // 細かい火花
	std::unique_ptr<Particle> welding_metal_spatter_; // 金属飛散
	std::unique_ptr<Particle> welding_smoke_light_;	  // 軽い煙
	std::unique_ptr<Particle> welding_electric_glow_; // 電気光
	std::unique_ptr<Particle> welding_plasma_burst_;  // プラズマバースト
	std::unique_ptr<Particle> welding_flash_pulse_;	  // 瞬間閃光

	// ヒットエフェクト用エミッター
	std::unique_ptr<ParticleEmitter> hitEmitter_core_;
	std::unique_ptr<ParticleEmitter> hitEmitter_sparks_;
	std::unique_ptr<ParticleEmitter> hitEmitter_smoke_;
	std::unique_ptr<ParticleEmitter> hitEmitter_shockwave_;

	// ダイナミックヒットエフェクト用エミッター
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_flash_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_spiral_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_explosion_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_rings_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_fragments_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_embers_;
	std::unique_ptr<ParticleEmitter> dynamicHitEmitter_distortion_;

	// アーク溶接エフェクト用エミッター
	std::unique_ptr<ParticleEmitter> weldingEmitter_arc_core_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_sparks_hot_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_sparks_fine_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_metal_spatter_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_smoke_light_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_electric_glow_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_plasma_burst_;
	std::unique_ptr<ParticleEmitter> weldingEmitter_flash_pulse_;

	///--------------------------------------------------------------
	///						 アプリケーション固有
	Transform transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	// ハンマーアニメーション制御用
	Transform hammerTransform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 4.0f, -1.0f}};
	bool enableHammerAnimation_ = false;
	bool enableParticleFollowHammer_ = true; // パーティクルがハンマーに追従するかどうか
	float hammerAnimationTimer_ = 0.0f;
	float hammerAnimationCycleDuration_ = 2.0f; // アニメーションサイクル時間
	float hammerStartHeight_ = 4.0f;			// ハンマーの開始高さ（電子レンジを叩けるように調整）
	float hammerSwingDistance_ = 3.0f;			// 振り下ろし距離
	float microwaveHeight_ = 1.0f;				// 電子レンジの上部の高さ
	bool hammerHitGround_ = false;				// 電子レンジに当たったフラグ
	bool hammerHitDetected_ = false;			// 衝突検知済みフラグ
	bool enableAutoHitEffect_ = true;			// 自動ヒットエフェクト
	bool enableAutoDynamicHitEffect_ = true;	// 自動ダイナミックヒットエフェクト

	// ヒットエフェクト制御用
	bool triggerHitEffect_ = false;
	Vector3 hitEffectPosition_ = {0.0f, 0.0f, 0.0f};
	float hitEffectScale_ = 1.0f;
	Vector4 coreColor_ = {1.0f, 0.5f, 0.1f, 1.0f};		// オレンジ
	Vector4 sparksColor_ = {1.0f, 1.0f, 0.3f, 1.0f};	// 黄色
	Vector4 smokeColor_ = {0.3f, 0.3f, 0.3f, 0.8f};		// 灰色
	Vector4 shockwaveColor_ = {0.8f, 0.9f, 1.0f, 0.5f}; // 青白い

	// ダイナミックヒットエフェクト制御用
	bool triggerDynamicHitEffect_ = false;
	Vector3 dynamicHitEffectPosition_ = {0.0f, 0.0f, 0.0f};
	float dynamicHitEffectScale_ = 1.0f;
	Vector4 dynamicFlashColor_ = {1.0f, 0.9f, 0.8f, 1.0f};		// 明るい閃光
	Vector4 dynamicSpiralColor_ = {1.0f, 0.6f, 0.1f, 1.0f};		// オレンジ螺旋
	Vector4 dynamicExplosionColor_ = {1.0f, 0.3f, 0.0f, 1.0f};	// 赤い爆発
	Vector4 dynamicRingsColor_ = {0.8f, 1.0f, 1.0f, 0.7f};		// 青白いリング
	Vector4 dynamicFragmentsColor_ = {1.0f, 1.0f, 0.5f, 1.0f};	// 黄色い破片
	Vector4 dynamicEmbersColor_ = {1.0f, 0.4f, 0.1f, 0.8f};		// 赤い余燼
	Vector4 dynamicDistortionColor_ = {0.5f, 0.8f, 1.0f, 0.3f}; // 薄い歪み

	// アーク溶接エフェクト制御用
	bool triggerWeldingEffect_ = false;
	bool enableWeldingEffect_ = false; // 継続的な溶接エフェクト
	Vector3 weldingEffectPosition_ = {0.0f, 0.0f, 0.0f};
	float weldingEffectScale_ = 1.0f;
	float weldingIntensity_ = 1.0f;								  // 溶接強度
	Vector4 weldingArcCoreColor_ = {0.8f, 0.9f, 1.0f, 1.0f};	  // アーク中心（青白）
	Vector4 weldingSparksHotColor_ = {1.0f, 0.6f, 0.1f, 1.0f};	  // 高温火花（オレンジ）
	Vector4 weldingSparksFineColor_ = {1.0f, 1.0f, 0.7f, 1.0f};	  // 細かい火花（黄白）
	Vector4 weldingMetalSpatterColor_ = {1.0f, 0.4f, 0.0f, 1.0f}; // 金属飛散（赤オレンジ）
	Vector4 weldingSmokeLightColor_ = {0.7f, 0.8f, 0.9f, 0.4f};	  // 軽い煙（青っぽい）
	Vector4 weldingElectricGlowColor_ = {0.6f, 0.8f, 1.0f, 0.9f}; // 電気光（青）
	Vector4 weldingPlasmaBurstColor_ = {0.9f, 0.95f, 1.0f, 1.0f}; // プラズマ（青白）
	Vector4 weldingFlashPulseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};	  // 瞬間閃光（白）

	// 溶接エフェクトの強度制御
	float sparkBurstIntensity_ = 2.0f; // 火花バースト強度
	float smokeDensity_ = 1.5f;		   // 煙の密度
	bool enableSparkOverflow_ = true;  // 火花あふれ効果
	bool enableSmokeOverflow_ = true;  // 煙あふれ効果

	// ループ制御用
	bool enableHitEffectLoop_ = false;	 // ループ有効フラグ
	float hitEffectLoopInterval_ = 2.0f; // ループ間隔（秒）
	float hitEffectLoopTimer_ = 0.0f;	 // ループタイマー

	// ダイナミックヒットエフェクトループ制御用
	bool enableDynamicHitLoop_ = false;
	float dynamicHitLoopInterval_ = 1.0f;
	float dynamicHitLoopTimer_ = 0.0f;
};

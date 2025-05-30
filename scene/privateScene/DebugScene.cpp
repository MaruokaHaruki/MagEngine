/*********************************************************************
 * \file   DebugScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "DebugScene.h"
#include "Input.h"

///=============================================================================
///						初期化
void DebugScene::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup) {
	spriteSetup;
	particleSetup;

	///--------------------------------------------------------------
	///						 音声クラス
	audio_ = MAudioG::GetInstance();
	MAudioG::GetInstance()->LoadWav("Duke_Ellington.wav");

	///--------------------------------------------------------------
	///						 2D系クラス
	//========================================
	//// テクスチャマネージャ

	//========================================
	// スプライトクラス(Game)

	///--------------------------------------------------------------
	///						 3D系クラス
	// モデルの読み込み
	ModelManager::GetInstance()->LoadMedel("axisPlus.obj");
	ModelManager::GetInstance()->LoadMedel("ball.obj");
	ModelManager::GetInstance()->LoadMedel("terrain.obj");
	ModelManager::GetInstance()->LoadMedel("skydome.obj");
	ModelManager::GetInstance()->LoadMedel("hammer.obj");
	ModelManager::GetInstance()->LoadMedel("Microwave.obj");
	//========================================
	// 3Dオブジェクトクラス
	// モンスターボール
	objMonsterBall_ = std::make_unique<Object3d>();
	objMonsterBall_->Initialize(object3dSetup);
	objMonsterBall_->SetModel("ball.obj");
	// 地面
	objTerrain_ = std::make_unique<Object3d>();
	objTerrain_->Initialize(object3dSetup);
	objTerrain_->SetModel("terrain.obj");
	// スカイドーム
	objSkyDome_ = std::make_unique<Object3d>();
	objSkyDome_->Initialize(object3dSetup);
	objSkyDome_->SetModel("skydome.obj");
	// ハンマー
	objHammer_ = std::make_unique<Object3d>();
	objHammer_->Initialize(object3dSetup);
	objHammer_->SetModel("hammer.obj");
	// 電磁レンジ
	objMicrowave_ = std::make_unique<Object3d>();
	objMicrowave_->Initialize(object3dSetup);
	objMicrowave_->SetModel("Microwave.obj");

	///--------------------------------------------------------------
	///						 パーティクル系
	//========================================
	// ヒットエフェクト用パーティクルの初期化
	// コア（中心の爆発）
	hitEffect_core_ = std::make_unique<Particle>();
	hitEffect_core_->Initialize(particleSetup);
	hitEffect_core_->CreateParticleGroup("HitCore", "circle2.png", ParticleShape::Board);
	hitEffect_core_->SetBillboard(true);
	hitEffect_core_->SetVelocityRange({-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f});
	hitEffect_core_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.5f, 1.5f, 1.5f});
	hitEffect_core_->SetEndScaleRange({3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f});
	hitEffect_core_->SetLifetimeRange(0.3f, 0.6f);
	hitEffect_core_->SetColorRange(coreColor_, coreColor_);
	hitEffect_core_->SetFadeInOut(0.1f, 0.6f);

	// 火花
	hitEffect_sparks_ = std::make_unique<Particle>();
	hitEffect_sparks_->Initialize(particleSetup);
	hitEffect_sparks_->CreateParticleGroup("HitSparks", "circle2.png", ParticleShape::Board);
	hitEffect_sparks_->SetBillboard(true);
	hitEffect_sparks_->SetVelocityRange({-8.0f, -2.0f, -8.0f}, {8.0f, 8.0f, 8.0f});
	hitEffect_sparks_->SetInitialScaleRange({0.2f, 0.2f, 0.2f}, {0.5f, 0.5f, 0.5f});
	hitEffect_sparks_->SetEndScaleRange({0.0f, 0.0f, 0.0f}, {0.1f, 0.1f, 0.1f});
	hitEffect_sparks_->SetLifetimeRange(0.4f, 0.8f);
	hitEffect_sparks_->SetColorRange(sparksColor_, sparksColor_);
	hitEffect_sparks_->SetGravity({0.0f, -15.0f, 0.0f});
	hitEffect_sparks_->SetFadeInOut(0.0f, 0.3f);

	// 煙
	hitEffect_smoke_ = std::make_unique<Particle>();
	hitEffect_smoke_->Initialize(particleSetup);
	hitEffect_smoke_->CreateParticleGroup("HitSmoke", "circle2.png", ParticleShape::Board);
	hitEffect_smoke_->SetBillboard(true);
	hitEffect_smoke_->SetVelocityRange({-3.0f, 1.0f, -3.0f}, {3.0f, 5.0f, 3.0f});
	hitEffect_smoke_->SetInitialScaleRange({1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f});
	hitEffect_smoke_->SetEndScaleRange({4.0f, 4.0f, 4.0f}, {6.0f, 6.0f, 6.0f});
	hitEffect_smoke_->SetLifetimeRange(1.0f, 2.0f);
	hitEffect_smoke_->SetColorRange(smokeColor_, smokeColor_);
	hitEffect_smoke_->SetGravity({0.0f, -2.0f, 0.0f});
	hitEffect_smoke_->SetFadeInOut(0.2f, 0.4f);

	// 衝撃波
	hitEffect_shockwave_ = std::make_unique<Particle>();
	hitEffect_shockwave_->Initialize(particleSetup);
	hitEffect_shockwave_->CreateParticleGroup("HitShockwave", "circle2.png", ParticleShape::Ring);
	hitEffect_shockwave_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	hitEffect_shockwave_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
	hitEffect_shockwave_->SetEndScaleRange({8.0f, 8.0f, 8.0f}, {12.0f, 12.0f, 12.0f});
	hitEffect_shockwave_->SetLifetimeRange(0.5f, 0.8f);
	hitEffect_shockwave_->SetColorRange(shockwaveColor_, shockwaveColor_);
	hitEffect_shockwave_->SetFadeInOut(0.0f, 0.1f);

	//========================================
	// ダイナミックヒットエフェクト用パーティクルの初期化
	// 瞬間フラッシュ
	dynamicHit_flash_ = std::make_unique<Particle>();
	dynamicHit_flash_->Initialize(particleSetup);
	dynamicHit_flash_->CreateParticleGroup("DynamicFlash", "circle2.png", ParticleShape::Board);
	dynamicHit_flash_->SetBillboard(true);
	dynamicHit_flash_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	dynamicHit_flash_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
	dynamicHit_flash_->SetEndScaleRange({8.0f, 8.0f, 8.0f}, {12.0f, 12.0f, 12.0f});
	dynamicHit_flash_->SetLifetimeRange(0.05f, 0.1f);
	dynamicHit_flash_->SetColorRange(dynamicFlashColor_, dynamicFlashColor_);
	dynamicHit_flash_->SetFadeInOut(0.0f, 0.8f);

	// 螺旋火花
	dynamicHit_spiral_ = std::make_unique<Particle>();
	dynamicHit_spiral_->Initialize(particleSetup);
	dynamicHit_spiral_->CreateParticleGroup("DynamicSpiral", "circle2.png", ParticleShape::Board);
	dynamicHit_spiral_->SetBillboard(true);
	dynamicHit_spiral_->SetVelocityRange({-15.0f, -5.0f, -15.0f}, {15.0f, 15.0f, 15.0f});
	dynamicHit_spiral_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
	dynamicHit_spiral_->SetEndScaleRange({0.0f, 0.0f, 0.0f}, {0.05f, 0.05f, 0.05f});
	dynamicHit_spiral_->SetLifetimeRange(0.4f, 0.8f);
	dynamicHit_spiral_->SetColorRange(dynamicSpiralColor_, dynamicSpiralColor_);
	dynamicHit_spiral_->SetGravity({0.0f, -25.0f, 0.0f});
	dynamicHit_spiral_->SetFadeInOut(0.0f, 0.4f);

	// 爆発コア
	dynamicHit_explosion_ = std::make_unique<Particle>();
	dynamicHit_explosion_->Initialize(particleSetup);
	dynamicHit_explosion_->CreateParticleGroup("DynamicExplosion", "circle2.png", ParticleShape::Board);
	dynamicHit_explosion_->SetBillboard(true);
	dynamicHit_explosion_->SetVelocityRange({-8.0f, -8.0f, -8.0f}, {8.0f, 8.0f, 8.0f});
	dynamicHit_explosion_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
	dynamicHit_explosion_->SetEndScaleRange({4.0f, 4.0f, 4.0f}, {6.0f, 6.0f, 6.0f});
	dynamicHit_explosion_->SetLifetimeRange(0.2f, 0.4f);
	dynamicHit_explosion_->SetColorRange(dynamicExplosionColor_, dynamicExplosionColor_);
	dynamicHit_explosion_->SetFadeInOut(0.0f, 0.5f);

	// 連続衝撃波
	dynamicHit_rings_ = std::make_unique<Particle>();
	dynamicHit_rings_->Initialize(particleSetup);
	dynamicHit_rings_->CreateParticleGroup("DynamicRings", "circle2.png", ParticleShape::Ring);
	dynamicHit_rings_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	dynamicHit_rings_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
	dynamicHit_rings_->SetEndScaleRange({10.0f, 10.0f, 10.0f}, {15.0f, 15.0f, 15.0f});
	dynamicHit_rings_->SetLifetimeRange(0.6f, 1.0f);
	dynamicHit_rings_->SetColorRange(dynamicRingsColor_, dynamicRingsColor_);
	dynamicHit_rings_->SetFadeInOut(0.0f, 0.2f);

	// 高速破片
	dynamicHit_fragments_ = std::make_unique<Particle>();
	dynamicHit_fragments_->Initialize(particleSetup);
	dynamicHit_fragments_->CreateParticleGroup("DynamicFragments", "circle2.png", ParticleShape::Board);
	dynamicHit_fragments_->SetBillboard(true);
	dynamicHit_fragments_->SetVelocityRange({-20.0f, 5.0f, -20.0f}, {20.0f, 25.0f, 20.0f});
	dynamicHit_fragments_->SetInitialScaleRange({0.08f, 0.08f, 0.08f}, {0.2f, 0.2f, 0.2f});
	dynamicHit_fragments_->SetEndScaleRange({0.0f, 0.0f, 0.0f}, {0.05f, 0.05f, 0.05f});
	dynamicHit_fragments_->SetLifetimeRange(0.5f, 1.2f);
	dynamicHit_fragments_->SetColorRange(dynamicFragmentsColor_, dynamicFragmentsColor_);
	dynamicHit_fragments_->SetGravity({0.0f, -18.0f, 0.0f});
	dynamicHit_fragments_->SetFadeInOut(0.0f, 0.3f);

	// 余燼
	dynamicHit_embers_ = std::make_unique<Particle>();
	dynamicHit_embers_->Initialize(particleSetup);
	dynamicHit_embers_->CreateParticleGroup("DynamicEmbers", "circle2.png", ParticleShape::Board);
	dynamicHit_embers_->SetBillboard(true);
	dynamicHit_embers_->SetVelocityRange({-5.0f, 2.0f, -5.0f}, {5.0f, 8.0f, 5.0f});
	dynamicHit_embers_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
	dynamicHit_embers_->SetEndScaleRange({0.05f, 0.05f, 0.05f}, {0.1f, 0.1f, 0.1f});
	dynamicHit_embers_->SetLifetimeRange(1.5f, 3.0f);
	dynamicHit_embers_->SetColorRange(dynamicEmbersColor_, dynamicEmbersColor_);
	dynamicHit_embers_->SetGravity({0.0f, -3.0f, 0.0f});
	dynamicHit_embers_->SetFadeInOut(0.1f, 0.6f);

	// 歪みエフェクト
	dynamicHit_distortion_ = std::make_unique<Particle>();
	dynamicHit_distortion_->Initialize(particleSetup);
	dynamicHit_distortion_->CreateParticleGroup("DynamicDistortion", "circle2.png", ParticleShape::Board);
	dynamicHit_distortion_->SetBillboard(true);
	dynamicHit_distortion_->SetVelocityRange({-2.0f, -2.0f, -2.0f}, {2.0f, 2.0f, 2.0f});
	dynamicHit_distortion_->SetInitialScaleRange({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f});
	dynamicHit_distortion_->SetEndScaleRange({8.0f, 8.0f, 8.0f}, {12.0f, 12.0f, 12.0f});
	dynamicHit_distortion_->SetLifetimeRange(0.3f, 0.6f);
	dynamicHit_distortion_->SetColorRange(dynamicDistortionColor_, dynamicDistortionColor_);
	dynamicHit_distortion_->SetFadeInOut(0.0f, 0.1f);

	//========================================
	// アーク溶接エフェクト用パーティクルの初期化
	// 溶接アーク中心部（集中した青白い光）
	welding_arc_core_ = std::make_unique<Particle>();
	welding_arc_core_->Initialize(particleSetup);
	welding_arc_core_->CreateParticleGroup("WeldingArcCore", "circle2.png", ParticleShape::Board);
	welding_arc_core_->SetBillboard(true);
	welding_arc_core_->SetVelocityRange({-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f});
	welding_arc_core_->SetInitialScaleRange({0.2f, 0.2f, 0.2f}, {0.4f, 0.4f, 0.4f});
	welding_arc_core_->SetEndScaleRange({0.6f, 0.6f, 0.6f}, {1.0f, 1.0f, 1.0f});
	welding_arc_core_->SetLifetimeRange(0.1f, 0.2f);
	welding_arc_core_->SetColorRange(weldingArcCoreColor_, weldingArcCoreColor_);
	welding_arc_core_->SetFadeInOut(0.0f, 0.3f);

	// 高温火花（溶接特有のオレンジ火花）
	welding_sparks_hot_ = std::make_unique<Particle>();
	welding_sparks_hot_->Initialize(particleSetup);
	welding_sparks_hot_->CreateParticleGroup("WeldingSparksHot", "circle2.png", ParticleShape::Board);
	welding_sparks_hot_->SetBillboard(true);
	welding_sparks_hot_->SetVelocityRange({-8.0f, -2.0f, -8.0f}, {8.0f, 6.0f, 8.0f});
	welding_sparks_hot_->SetInitialScaleRange({0.04f, 0.04f, 0.04f}, {0.1f, 0.1f, 0.1f});
	welding_sparks_hot_->SetEndScaleRange({0.06f, 0.06f, 0.06f}, {0.12f, 0.12f, 0.12f});
	welding_sparks_hot_->SetLifetimeRange(0.3f, 0.6f);
	welding_sparks_hot_->SetColorRange(weldingSparksHotColor_, weldingSparksHotColor_);
	welding_sparks_hot_->SetGravity({0.0f, -12.0f, 0.0f});
	welding_sparks_hot_->SetFadeInOut(0.0f, 0.4f);

	// 細かい火花（高密度の小さな火花）
	welding_sparks_fine_ = std::make_unique<Particle>();
	welding_sparks_fine_->Initialize(particleSetup);
	welding_sparks_fine_->CreateParticleGroup("WeldingSparksFine", "circle2.png", ParticleShape::Board);
	welding_sparks_fine_->SetBillboard(true);
	welding_sparks_fine_->SetVelocityRange({-15.0f, -1.0f, -15.0f}, {15.0f, 10.0f, 15.0f});
	welding_sparks_fine_->SetInitialScaleRange({0.015f, 0.015f, 0.015f}, {0.04f, 0.04f, 0.04f});
	welding_sparks_fine_->SetEndScaleRange({0.05f, 0.05f, 0.05f}, {0.1f, 0.1f, 0.1f});
	welding_sparks_fine_->SetLifetimeRange(0.1f, 0.4f);
	welding_sparks_fine_->SetColorRange(weldingSparksFineColor_, weldingSparksFineColor_);
	welding_sparks_fine_->SetGravity({0.0f, -18.0f, 0.0f});
	welding_sparks_fine_->SetFadeInOut(0.0f, 0.2f);

	// 金属飛散（大きめの溶融金属）
	welding_metal_spatter_ = std::make_unique<Particle>();
	welding_metal_spatter_->Initialize(particleSetup);
	welding_metal_spatter_->CreateParticleGroup("WeldingMetalSpatter", "circle2.png", ParticleShape::Board);
	welding_metal_spatter_->SetBillboard(true);
	welding_metal_spatter_->SetVelocityRange({-6.0f, 1.0f, -6.0f}, {6.0f, 8.0f, 6.0f});
	welding_metal_spatter_->SetInitialScaleRange({0.08f, 0.08f, 0.08f}, {0.15f, 0.15f, 0.15f});
	welding_metal_spatter_->SetEndScaleRange({0.03f, 0.03f, 0.03f}, {0.06f, 0.06f, 0.06f});
	welding_metal_spatter_->SetLifetimeRange(0.6f, 1.2f);
	welding_metal_spatter_->SetColorRange(weldingMetalSpatterColor_, weldingMetalSpatterColor_);
	welding_metal_spatter_->SetGravity({0.0f, -10.0f, 0.0f});
	welding_metal_spatter_->SetFadeInOut(0.0f, 0.5f);

	// 軽い煙（溶接時の軽微な煙）
	welding_smoke_light_ = std::make_unique<Particle>();
	welding_smoke_light_->Initialize(particleSetup);
	welding_smoke_light_->CreateParticleGroup("WeldingSmokeLight", "circle2.png", ParticleShape::Board);
	welding_smoke_light_->SetBillboard(true);
	welding_smoke_light_->SetVelocityRange({-2.0f, 1.0f, -2.0f}, {2.0f, 4.0f, 2.0f});
	welding_smoke_light_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
	welding_smoke_light_->SetEndScaleRange({0.8f, 0.8f, 0.8f}, {1.5f, 1.5f, 1.5f});
	welding_smoke_light_->SetLifetimeRange(1.0f, 2.0f);
	welding_smoke_light_->SetColorRange(weldingSmokeLightColor_, weldingSmokeLightColor_);
	welding_smoke_light_->SetGravity({0.0f, -0.5f, 0.0f});
	welding_smoke_light_->SetFadeInOut(0.2f, 0.4f);

	// 電気光（アーク周辺の電気的な光）
	welding_electric_glow_ = std::make_unique<Particle>();
	welding_electric_glow_->Initialize(particleSetup);
	welding_electric_glow_->CreateParticleGroup("WeldingElectricGlow", "circle2.png", ParticleShape::Board);
	welding_electric_glow_->SetBillboard(true);
	welding_electric_glow_->SetVelocityRange({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
	welding_electric_glow_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.6f, 0.6f, 0.6f});
	welding_electric_glow_->SetEndScaleRange({1.0f, 1.0f, 1.0f}, {1.8f, 1.8f, 1.8f});
	welding_electric_glow_->SetLifetimeRange(0.15f, 0.3f);
	welding_electric_glow_->SetColorRange(weldingElectricGlowColor_, weldingElectricGlowColor_);
	welding_electric_glow_->SetFadeInOut(0.0f, 0.4f);

	// プラズマバースト（瞬間的な強いプラズマ）
	welding_plasma_burst_ = std::make_unique<Particle>();
	welding_plasma_burst_->Initialize(particleSetup);
	welding_plasma_burst_->CreateParticleGroup("WeldingPlasmaBurst", "circle2.png", ParticleShape::Board);
	welding_plasma_burst_->SetBillboard(true);
	welding_plasma_burst_->SetVelocityRange({-3.0f, -3.0f, -3.0f}, {3.0f, 3.0f, 3.0f});
	welding_plasma_burst_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
	welding_plasma_burst_->SetEndScaleRange({1.5f, 1.5f, 1.5f}, {2.5f, 2.5f, 2.5f});
	welding_plasma_burst_->SetLifetimeRange(0.08f, 0.15f);
	welding_plasma_burst_->SetColorRange(weldingPlasmaBurstColor_, weldingPlasmaBurstColor_);
	welding_plasma_burst_->SetFadeInOut(0.0f, 0.6f);

	// 瞬間閃光（アーク発生時の強烈な光）
	welding_flash_pulse_ = std::make_unique<Particle>();
	welding_flash_pulse_->Initialize(particleSetup);
	welding_flash_pulse_->CreateParticleGroup("WeldingFlashPulse", "circle2.png", ParticleShape::Board);
	welding_flash_pulse_->SetBillboard(true);
	welding_flash_pulse_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	welding_flash_pulse_->SetInitialScaleRange({0.05f, 0.05f, 0.05f}, {0.1f, 0.1f, 0.1f});
	welding_flash_pulse_->SetEndScaleRange({2.0f, 2.0f, 2.0f}, {3.5f, 3.5f, 3.5f});
	welding_flash_pulse_->SetLifetimeRange(0.03f, 0.08f);
	welding_flash_pulse_->SetColorRange(weldingFlashPulseColor_, weldingFlashPulseColor_);
	welding_flash_pulse_->SetFadeInOut(0.0f, 0.8f);

	//========================================
	// エミッターの作成
	// ヒットエフェクト用エミッター（非リピート）
	hitEmitter_core_ = std::make_unique<ParticleEmitter>(
		hitEffect_core_.get(), "HitCore",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, hitEffectPosition_},
		8, 0.1f, false);

	hitEmitter_sparks_ = std::make_unique<ParticleEmitter>(
		hitEffect_sparks_.get(), "HitSparks",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, hitEffectPosition_},
		20, 0.1f, false);

	hitEmitter_smoke_ = std::make_unique<ParticleEmitter>(
		hitEffect_smoke_.get(), "HitSmoke",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, hitEffectPosition_},
		12, 0.1f, false);

	hitEmitter_shockwave_ = std::make_unique<ParticleEmitter>(
		hitEffect_shockwave_.get(), "HitShockwave",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, hitEffectPosition_},
		3, 0.1f, false);

	// ダイナミックヒットエフェクト用エミッター（非リピート）
	dynamicHitEmitter_flash_ = std::make_unique<ParticleEmitter>(
		dynamicHit_flash_.get(), "DynamicFlash",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		2, 0.0f, false);

	dynamicHitEmitter_spiral_ = std::make_unique<ParticleEmitter>(
		dynamicHit_spiral_.get(), "DynamicSpiral",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		40, 0.1f, false);

	dynamicHitEmitter_explosion_ = std::make_unique<ParticleEmitter>(
		dynamicHit_explosion_.get(), "DynamicExplosion",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		12, 0.05f, false);

	dynamicHitEmitter_rings_ = std::make_unique<ParticleEmitter>(
		dynamicHit_rings_.get(), "DynamicRings",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		5, 0.15f, false);

	dynamicHitEmitter_fragments_ = std::make_unique<ParticleEmitter>(
		dynamicHit_fragments_.get(), "DynamicFragments",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		35, 0.08f, false);

	dynamicHitEmitter_embers_ = std::make_unique<ParticleEmitter>(
		dynamicHit_embers_.get(), "DynamicEmbers",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		20, 0.2f, false);

	dynamicHitEmitter_distortion_ = std::make_unique<ParticleEmitter>(
		dynamicHit_distortion_.get(), "DynamicDistortion",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, dynamicHitEffectPosition_},
		4, 0.1f, false);

	// アーク溶接エフェクト用エミッター（継続的）
	weldingEmitter_arc_core_ = std::make_unique<ParticleEmitter>(
		welding_arc_core_.get(), "WeldingArcCore",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		15, 0.02f, true);

	weldingEmitter_sparks_hot_ = std::make_unique<ParticleEmitter>(
		welding_sparks_hot_.get(), "WeldingSparksHot",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		150, 0.005f, true); // 数を150に増加、頻度を0.005fに短縮（ほぼ連続）

	weldingEmitter_sparks_fine_ = std::make_unique<ParticleEmitter>(
		welding_sparks_fine_.get(), "WeldingSparksFine",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		250, 0.003f, true); // 数を250に増加、頻度を0.003fに短縮（非常に高頻度）

	weldingEmitter_metal_spatter_ = std::make_unique<ParticleEmitter>(
		welding_metal_spatter_.get(), "WeldingMetalSpatter",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		50, 0.0004f, true); // 数を50に増加、頻度を0.015fに短縮

	weldingEmitter_smoke_light_ = std::make_unique<ParticleEmitter>(
		welding_smoke_light_.get(), "WeldingSmokeLight",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		40, 0.02f, true); // 数を40に増加、頻度を0.02fに短縮（より連続的）

	weldingEmitter_electric_glow_ = std::make_unique<ParticleEmitter>(
		welding_electric_glow_.get(), "WeldingElectricGlow",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		25, 0.015f, true); // 数を25に増加、頻度を0.015fに短縮

	weldingEmitter_plasma_burst_ = std::make_unique<ParticleEmitter>(
		welding_plasma_burst_.get(), "WeldingPlasmaBurst",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		12, 0.04f, true); // 数を12に増加、頻度を0.04fに短縮

	weldingEmitter_flash_pulse_ = std::make_unique<ParticleEmitter>(
		welding_flash_pulse_.get(), "WeldingFlashPulse",
		Transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, weldingEffectPosition_},
		5, 0.03f, true); // 数を5に増加、頻度を0.03fに短縮
}

//========================================
///						終了処理
void DebugScene::Finalize() {
}

///=============================================================================
///						更新
void DebugScene::Update() {
	///--------------------------------------------------------------
	///						更新処理

	//========================================
	// 3D更新
	// モンスターボール
	objMonsterBall_->SetScale(Vector3{transform.scale.x, transform.scale.y, transform.scale.z});
	objMonsterBall_->SetRotation(Vector3{transform.rotate.x, transform.rotate.y, transform.rotate.z});
	objMonsterBall_->SetPosition(Vector3{transform.translate.x, transform.translate.y, transform.translate.z});
	objMonsterBall_->Update();
	// 地面
	objTerrain_->SetScale(Vector3{1.0f, 1.0f, 1.0f});
	objTerrain_->SetRotation(Vector3{0.0f, 0.0f, 0.0f});
	objTerrain_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});
	objTerrain_->Update();
	// スカイドーム
	objSkyDome_->SetScale(Vector3{1.0f, 1.0f, 1.0f});
	objSkyDome_->SetRotation(Vector3{0.0f, 0.0f, 0.0f});
	objSkyDome_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});
	objSkyDome_->Update();
	// ハンマー
	objHammer_->SetScale(Vector3{transform.scale.x, transform.scale.y, transform.scale.z});
	objHammer_->SetRotation(Vector3{transform.rotate.x, transform.rotate.y, transform.rotate.z});
	objHammer_->SetPosition(Vector3{transform.translate.x, transform.translate.y, transform.translate.z});
	objHammer_->Update();
	// 電子レンジ
	objMicrowave_->SetScale(Vector3{ transform.scale.x, transform.scale.y, transform.scale.z });
	objMicrowave_->SetRotation(Vector3{ transform.rotate.x, transform.rotate.y, transform.rotate.z });
	objMicrowave_->SetPosition(Vector3{ transform.translate.x, transform.translate.y, transform.translate.z });
	objMicrowave_->Update();

	//========================================
	// ヒットエフェクトのトリガー処理
	if (triggerHitEffect_) {
		// エミッター位置を更新
		hitEmitter_core_->SetTranslate(hitEffectPosition_);
		hitEmitter_sparks_->SetTranslate(hitEffectPosition_);
		hitEmitter_smoke_->SetTranslate(hitEffectPosition_);
		hitEmitter_shockwave_->SetTranslate(hitEffectPosition_);

		// スケールを適用
		hitEffect_core_->SetInitialScaleRange(
			{0.5f * hitEffectScale_, 0.5f * hitEffectScale_, 0.5f * hitEffectScale_},
			{1.5f * hitEffectScale_, 1.5f * hitEffectScale_, 1.5f * hitEffectScale_});

		// エフェクトを発動
		hitEmitter_core_->Emit();
		hitEmitter_sparks_->Emit();
		hitEmitter_smoke_->Emit();
		hitEmitter_shockwave_->Emit();

		triggerHitEffect_ = false;
	}

	// ヒットエフェクトのループ処理
	if (enableHitEffectLoop_) {
		hitEffectLoopTimer_ += 1.0f / 60.0f;
		if (hitEffectLoopTimer_ >= hitEffectLoopInterval_) {
			triggerHitEffect_ = true;
			hitEffectLoopTimer_ = 0.0f;
		}
	}

	// ダイナミックヒットエフェクトのトリガー処理
	if (triggerDynamicHitEffect_) {
		// エミッター位置を更新
		dynamicHitEmitter_flash_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_spiral_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_explosion_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_rings_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_fragments_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_embers_->SetTranslate(dynamicHitEffectPosition_);
		dynamicHitEmitter_distortion_->SetTranslate(dynamicHitEffectPosition_);

		// スケールを適用
		dynamicHit_flash_->SetInitialScaleRange(
			{0.1f * dynamicHitEffectScale_, 0.1f * dynamicHitEffectScale_, 0.1f * dynamicHitEffectScale_},
			{0.2f * dynamicHitEffectScale_, 0.2f * dynamicHitEffectScale_, 0.2f * dynamicHitEffectScale_});

		// エフェクトを時間差で発動
		dynamicHitEmitter_flash_->Emit();
		dynamicHitEmitter_distortion_->Emit();
		dynamicHitEmitter_explosion_->Emit();
		dynamicHitEmitter_spiral_->Emit();
		dynamicHitEmitter_rings_->Emit();
		dynamicHitEmitter_fragments_->Emit();
		dynamicHitEmitter_embers_->Emit();

		triggerDynamicHitEffect_ = false;
	}

	// ダイナミックヒットエフェクトのループ処理
	if (enableDynamicHitLoop_) {
		dynamicHitLoopTimer_ += 1.0f / 60.0f;
		if (dynamicHitLoopTimer_ >= dynamicHitLoopInterval_) {
			triggerDynamicHitEffect_ = true;
			dynamicHitLoopTimer_ = 0.0f;
		}
	}

	// アーク溶接エフェクトの処理
	if (enableWeldingEffect_) {
		// エミッター位置を更新
		weldingEmitter_arc_core_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_sparks_hot_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_sparks_fine_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_metal_spatter_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_smoke_light_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_electric_glow_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_plasma_burst_->SetTranslate(weldingEffectPosition_);
		weldingEmitter_flash_pulse_->SetTranslate(weldingEffectPosition_);

		// スケールを適用
		welding_arc_core_->SetInitialScaleRange(
			{0.2f * weldingEffectScale_, 0.2f * weldingEffectScale_, 0.2f * weldingEffectScale_},
			{0.4f * weldingEffectScale_, 0.4f * weldingEffectScale_, 0.4f * weldingEffectScale_});
	}

	// 溶接エフェクトのエミッター更新
	if (enableWeldingEffect_) {
		weldingEmitter_arc_core_->Update();
		weldingEmitter_sparks_hot_->Update();
		weldingEmitter_sparks_fine_->Update();
		weldingEmitter_metal_spatter_->Update();
		weldingEmitter_smoke_light_->Update();
		weldingEmitter_electric_glow_->Update();
		weldingEmitter_plasma_burst_->Update();
		weldingEmitter_flash_pulse_->Update();
	}

	// パーティクルの更新
	hitEffect_core_->Update();
	hitEffect_sparks_->Update();
	hitEffect_smoke_->Update();
	hitEffect_shockwave_->Update();

	// ダイナミックヒットエフェクト用パーティクルの更新
	dynamicHit_flash_->Update();
	dynamicHit_spiral_->Update();
	dynamicHit_explosion_->Update();
	dynamicHit_rings_->Update();
	dynamicHit_fragments_->Update();
	dynamicHit_embers_->Update();
	dynamicHit_distortion_->Update();

	// アーク溶接エフェクト用パーティクルの更新
	welding_arc_core_->Update();
	welding_sparks_hot_->Update();
	welding_sparks_fine_->Update();
	welding_metal_spatter_->Update();
	welding_smoke_light_->Update();
	welding_electric_glow_->Update();
	welding_plasma_burst_->Update();
	welding_flash_pulse_->Update();

	//========================================
	// 音声の再生
	if (audio_->IsWavPlaying("Duke_Ellington.wav") == false) {
		// audio_->PlayWav("Duke_Ellington.wav", true, 1.0f, 1.0f);
	}
}

///=============================================================================
///						2D描画
void DebugScene::Object2DDraw() {
}

///=============================================================================
///						3D描画
void DebugScene::Object3DDraw() {
	// モンスターボール
	// objMonsterBall_->Draw();
	// 地面
	// objTerrain_->Draw();
	// スカイドーム
	objSkyDome_->Draw();
	// ハンマー
	objHammer_->Draw();
	// 電子レンジ
	objMicrowave_->Draw();
}

///=============================================================================
///						パーティクル描画
void DebugScene::ParticleDraw() {
	// ヒットエフェクトの描画
	hitEffect_shockwave_->Draw(); // 最初に衝撃波
	hitEffect_smoke_->Draw();	  // 次に煙
	hitEffect_core_->Draw();	  // コア
	hitEffect_sparks_->Draw();	  // 最後に火花

	// ダイナミックヒットエフェクトの描画（層状に描画）
	dynamicHit_distortion_->Draw(); // 歪み（最背面）
	dynamicHit_rings_->Draw();		// 衝撃波リング
	dynamicHit_embers_->Draw();		// 余燼
	dynamicHit_explosion_->Draw();	// 爆発コア
	dynamicHit_fragments_->Draw();	// 高速破片
	dynamicHit_spiral_->Draw();		// 螺旋火花
	dynamicHit_flash_->Draw();		// フラッシュ（最前面）

	// アーク溶接エフェクトの描画（背面から前面へ）
	// welding_smoke_light_->Draw();	// 煙（最背面）
	welding_electric_glow_->Draw(); // 電気光
	welding_metal_spatter_->Draw(); // 金属飛散
	welding_sparks_hot_->Draw();	// 高温火花
	welding_sparks_fine_->Draw();	// 細かい火花
	welding_plasma_burst_->Draw();	// プラズマバースト
	welding_arc_core_->Draw();		// アーク中心
	welding_flash_pulse_->Draw();	// 瞬間閃光（最前面）
}

///=============================================================================
///						ImGui描画
void DebugScene::ImGuiDraw() {
	// DebugSceneのImGui描画
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, DebugScene!");
	ImGui::End();

	//========================================
	// 3DオブジェクトのImGui描画
	ImGui::Begin("3DObject");
	ImGui::Text("TransformSetting");
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -180.0f, 180.0f);
	ImGui::SliderFloat3("Translate", &transform.translate.x, -10.0f, 10.0f);
	ImGui::Separator();
	ImGui::End();

	//========================================
	// ヒットエフェクトのImGui描画
	ImGui::Begin("Hit Effect Control");
	ImGui::Text("Hit Effect Settings");

	// エフェクト発動ボタン
	if (ImGui::Button("Trigger Hit Effect")) {
		triggerHitEffect_ = true;
	}

	ImGui::SameLine();

	// ループ制御
	if (ImGui::Checkbox("Enable Loop", &enableHitEffectLoop_)) {
		if (enableHitEffectLoop_) {
			hitEffectLoopTimer_ = 0.0f;
		}
	}

	ImGui::Separator();

	// ループ設定
	if (enableHitEffectLoop_) {
		ImGui::Text("Loop Settings");
		ImGui::SliderFloat("Loop Interval", &hitEffectLoopInterval_, 0.5f, 10.0f, "%.1f sec");

		float progress = hitEffectLoopTimer_ / hitEffectLoopInterval_;
		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), "Next Effect");

		ImGui::Separator();
	}

	// エフェクト位置とスケール
	ImGui::SliderFloat3("Effect Position", &hitEffectPosition_.x, -10.0f, 10.0f);
	ImGui::SliderFloat("Effect Scale", &hitEffectScale_, 0.1f, 3.0f);

	ImGui::Separator();

	// 色設定
	ImGui::ColorEdit4("Core Color", &coreColor_.x);
	ImGui::ColorEdit4("Sparks Color", &sparksColor_.x);
	ImGui::ColorEdit4("Smoke Color", &smokeColor_.x);
	ImGui::ColorEdit4("Shockwave Color", &shockwaveColor_.x);

	// 色を反映
	hitEffect_core_->SetColorRange(coreColor_, coreColor_);
	hitEffect_sparks_->SetColorRange(sparksColor_, sparksColor_);
	hitEffect_smoke_->SetColorRange(smokeColor_, smokeColor_);
	hitEffect_shockwave_->SetColorRange(shockwaveColor_, shockwaveColor_);

	ImGui::End();

	//========================================
	// ダイナミックヒットエフェクトのImGui描画
	ImGui::Begin("Dynamic Hit Effect Control");
	ImGui::Text("Dynamic Cinematic Hit Effect");

	// エフェクト発動ボタン
	if (ImGui::Button("Trigger Dynamic Hit Effect")) {
		triggerDynamicHitEffect_ = true;
	}

	ImGui::SameLine();

	// ループ制御
	if (ImGui::Checkbox("Enable Dynamic Hit Loop", &enableDynamicHitLoop_)) {
		if (enableDynamicHitLoop_) {
			dynamicHitLoopTimer_ = 0.0f;
		}
	}

	ImGui::Separator();

	// ループ設定
	if (enableDynamicHitLoop_) {
		ImGui::Text("Dynamic Hit Loop Settings");
		ImGui::SliderFloat("Dynamic Hit Loop Interval", &dynamicHitLoopInterval_, 0.2f, 3.0f, "%.1f sec");

		float progress = dynamicHitLoopTimer_ / dynamicHitLoopInterval_;
		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), "Next Dynamic Hit");

		ImGui::Separator();
	}

	// エフェクト位置とスケール
	ImGui::SliderFloat3("Dynamic Hit Position", &dynamicHitEffectPosition_.x, -10.0f, 10.0f);
	ImGui::SliderFloat("Dynamic Hit Scale", &dynamicHitEffectScale_, 0.1f, 5.0f);

	ImGui::Separator();

	// ダイナミックヒットエフェクトの色設定
	ImGui::Text("Dynamic Hit Colors");
	if (ImGui::CollapsingHeader("Flash & Core Colors")) {
		ImGui::ColorEdit4("Flash Color", &dynamicFlashColor_.x);
		ImGui::ColorEdit4("Explosion Color", &dynamicExplosionColor_.x);
		ImGui::ColorEdit4("Distortion Color", &dynamicDistortionColor_.x);
	}

	if (ImGui::CollapsingHeader("Particle Colors")) {
		ImGui::ColorEdit4("Spiral Color", &dynamicSpiralColor_.x);
		ImGui::ColorEdit4("Rings Color", &dynamicRingsColor_.x);
		ImGui::ColorEdit4("Fragments Color", &dynamicFragmentsColor_.x);
		ImGui::ColorEdit4("Embers Color", &dynamicEmbersColor_.x);
	}

	// 色を反映
	dynamicHit_flash_->SetColorRange(dynamicFlashColor_, dynamicFlashColor_);
	dynamicHit_spiral_->SetColorRange(dynamicSpiralColor_, dynamicSpiralColor_);
	dynamicHit_explosion_->SetColorRange(dynamicExplosionColor_, dynamicExplosionColor_);
	dynamicHit_rings_->SetColorRange(dynamicRingsColor_, dynamicRingsColor_);
	dynamicHit_fragments_->SetColorRange(dynamicFragmentsColor_, dynamicFragmentsColor_);
	dynamicHit_embers_->SetColorRange(dynamicEmbersColor_, dynamicEmbersColor_);
	dynamicHit_distortion_->SetColorRange(dynamicDistortionColor_, dynamicDistortionColor_);

	ImGui::End();

	//========================================
	// アーク溶接エフェクトのImGui描画
	ImGui::Begin("Arc Welding Effect Control");
	ImGui::Text("Arc Welding Sparks & Plasma Effect");

	// エフェクト制御
	if (ImGui::Checkbox("Enable Welding Effect", &enableWeldingEffect_)) {
		if (!enableWeldingEffect_) {
			// エフェクトが無効になった時の処理
		}
	}

	ImGui::Separator();

	// 溶接設定
	ImGui::SliderFloat3("Welding Position", &weldingEffectPosition_.x, -10.0f, 10.0f);
	ImGui::SliderFloat("Welding Scale", &weldingEffectScale_, 0.1f, 3.0f);
	ImGui::SliderFloat("Welding Intensity", &weldingIntensity_, 0.1f, 3.0f);

	ImGui::Separator();

	// アーク溶接エフェクトの色設定
	ImGui::Text("Arc Welding Effect Colors");
	if (ImGui::CollapsingHeader("Arc & Plasma Colors")) {
		ImGui::ColorEdit4("Arc Core", &weldingArcCoreColor_.x);
		ImGui::ColorEdit4("Electric Glow", &weldingElectricGlowColor_.x);
		ImGui::ColorEdit4("Plasma Burst", &weldingPlasmaBurstColor_.x);
		ImGui::ColorEdit4("Flash Pulse", &weldingFlashPulseColor_.x);
	}

	if (ImGui::CollapsingHeader("Sparks & Material Colors")) {
		ImGui::ColorEdit4("Hot Sparks", &weldingSparksHotColor_.x);
		ImGui::ColorEdit4("Fine Sparks", &weldingSparksFineColor_.x);
		ImGui::ColorEdit4("Metal Spatter", &weldingMetalSpatterColor_.x);
		ImGui::ColorEdit4("Light Smoke", &weldingSmokeLightColor_.x);
	}

	// 色を反映
	welding_arc_core_->SetColorRange(weldingArcCoreColor_, weldingArcCoreColor_);
	welding_sparks_hot_->SetColorRange(weldingSparksHotColor_, weldingSparksHotColor_);
	welding_sparks_fine_->SetColorRange(weldingSparksFineColor_, weldingSparksFineColor_);
	welding_metal_spatter_->SetColorRange(weldingMetalSpatterColor_, weldingMetalSpatterColor_);
	welding_smoke_light_->SetColorRange(weldingSmokeLightColor_, weldingSmokeLightColor_);
	welding_electric_glow_->SetColorRange(weldingElectricGlowColor_, weldingElectricGlowColor_);
	welding_plasma_burst_->SetColorRange(weldingPlasmaBurstColor_, weldingPlasmaBurstColor_);
	welding_flash_pulse_->SetColorRange(weldingFlashPulseColor_, weldingFlashPulseColor_);

	ImGui::End();
}

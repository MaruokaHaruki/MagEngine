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

	///--------------------------------------------------------------
	///						 パーティクル系
	//========================================
	// パーティクルの作成

	//========================================
	// パーティクルクラス
	particle_ = std::make_unique<Particle>();
	particle2_ = std::make_unique<Particle>();
	particle3_ = std::make_unique<Particle>();
	particle_->Initialize(particleSetup);
	particle2_->Initialize(particleSetup);
	particle3_->Initialize(particleSetup);
	// パーティクルのグループを作成
	particle_->CreateParticleGroup("Test", "circle2.png", ParticleShape::Board);
	particle2_->CreateParticleGroup("Test2", "circle2.png", ParticleShape::Ring);
	particle3_->CreateParticleGroup("Test3", "circle2.png", ParticleShape::Cylinder);

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
	hitEffect_core_->SetFadeInOut(0.1f, 0.f);

	// 火花
	hitEffect_sparks_ = std::make_unique<Particle>();
	hitEffect_sparks_->Initialize(particleSetup);
	hitEffect_sparks_->CreateParticleGroup("HitSparks", "circle2.png", ParticleShape::Board);
	hitEffect_sparks_->SetBillboard(true);
	hitEffect_sparks_->SetVelocityRange({-8.0f, -2.0f, -8.0f}, {8.0f, 8.0f, 8.0f});
	hitEffect_sparks_->SetInitialScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
	hitEffect_sparks_->SetEndScaleRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
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
	// エミッターの作成
	particleEmitter_ =
		std::make_unique<ParticleEmitter>(particle_.get(),
										  "Test",
										  Transform{{0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  2.0f,
										  true);
	particleEmitter2_ =
		std::make_unique<ParticleEmitter>(particle2_.get(),
										  "Test2",
										  Transform{{0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  2.0f,
										  true);
	particleEmitter3_ =
		std::make_unique<ParticleEmitter>(particle3_.get(),
										  "Test3",
										  Transform{{0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  2.0f,
										  true);

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
}

///=============================================================================
///						終了処理
void DebugScene::Finalize() {
}

///=============================================================================
///						更新
void DebugScene::Update() {
	///--------------------------------------------------------------
	///						更新処理

	//========================================
	// カメラの更新

	//========================================
	// 2D更新

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

	//========================================
	// パーティクル系
	// パーティクルの更新
	particle_->Update();
	particle2_->Update();
	particle3_->Update();
	// エミッターの更新
	particleEmitter_->Update();
	particleEmitter2_->Update();
	particleEmitter3_->Update();

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
		hitEffectLoopTimer_ += 1.0f / 60.0f; // フレーム時間を加算（60FPS想定）

		if (hitEffectLoopTimer_ >= hitEffectLoopInterval_) {
			// エフェクトを発動
			triggerHitEffect_ = true;
			hitEffectLoopTimer_ = 0.0f; // タイマーをリセット
		}
	}

	// ヒットエフェクト用パーティクルの更新
	hitEffect_core_->Update();
	hitEffect_sparks_->Update();
	hitEffect_smoke_->Update();
	hitEffect_shockwave_->Update();

	//========================================
	// 音声の再生
	if (audio_->IsWavPlaying("Duke_Ellington.wav") == false) {
		// audio_->PlayWavReverse("Duke_Ellington.wav", true, 1.0f, 1.0f);
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
}

///=============================================================================
///						パーティクル描画
void DebugScene::ParticleDraw() {
	// パーティクルの描画
	particle_->Draw();
	particle2_->Draw();
	particle3_->Draw();

	// ヒットエフェクトの描画
	hitEffect_shockwave_->Draw(); // 最初に衝撃波
	hitEffect_smoke_->Draw();	  // 次に煙
	hitEffect_core_->Draw();	  // コア
	hitEffect_sparks_->Draw();	  // 最後に火花
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
	// ライトの設定
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
			hitEffectLoopTimer_ = 0.0f; // ループ開始時にタイマーをリセット
		}
	}

	ImGui::Separator();

	// ループ設定
	if (enableHitEffectLoop_) {
		ImGui::Text("Loop Settings");
		ImGui::SliderFloat("Loop Interval", &hitEffectLoopInterval_, 0.5f, 10.0f, "%.1f sec");

		// 進行状況バー
		float progress = hitEffectLoopTimer_ / hitEffectLoopInterval_;
		ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), "Next Effect");

		ImGui::Separator();
	}

	// エフェクト位置
	ImGui::SliderFloat3("Effect Position", &hitEffectPosition_.x, -10.0f, 10.0f);

	// エフェクトスケール
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
}

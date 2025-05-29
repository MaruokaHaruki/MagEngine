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
	particle_->Initialize(particleSetup);
	// パーティクルのグループを作成
	particle_->CreateParticleGroup("Test", "gradationLine_top.png", ParticleShape::Cylinder);
	//========================================
	// エミッターの作成
	particleEmitter_ =
		std::make_unique<ParticleEmitter>(particle_.get(),
										  "Test",
										  Transform{{0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  2.0f,
										  true);

	///--------------------------------------------------------------
	///						 TestPlayer系
	//========================================
	// TestPlayerの初期化
	// FPS設定: 60, 50, 40, 30, 20, 10, 8, 6, 4, 2
	targetFPSList_ = {60.0f, 50.0f, 40.0f, 30.0f, 20.0f, 10.0f, 8.0f, 6.0f, 4.0f, 2.0f};
	testPlayers_.resize(targetFPSList_.size());

	// 各TestPlayerを初期化し、横並びに配置（間隔を短く調整）
	float startX = -14.0f; // 開始位置を調整
	float spacing = 1.2f;  // 間隔を短くする

	for (size_t i = 0; i < testPlayers_.size(); ++i) {
		testPlayers_[i] = std::make_unique<TestPlayer>();
		testPlayers_[i]->Initialize();

		// 各プレイヤーの初期位置を設定（横並び、Y=0で地面レベル）
		Vector2 playerPos = {startX + (i * spacing), 0.0f};
		testPlayers_[i]->SetPosition(playerPos);

		// 各プレイヤーの目標FPSを設定
		testPlayers_[i]->SetTargetFPS(targetFPSList_[i]);
	}

	// TestPlayer表示フラグ
	showTestPlayers_ = true;
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
	// TestPlayer更新
	if (showTestPlayers_) {
		for (auto &player : testPlayers_) {
			player->Update();
		}
	}

	//========================================
	// パーティクル系
	// パーティクルの更新
	particle_->Update();
	// エミッターの更新
	particleEmitter_->Update();

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
	//========================================
	// TestPlayer描画
	if (showTestPlayers_) {
		for (auto &player : testPlayers_) {
			player->Draw();
		}
	}
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
}

///=============================================================================
///						ImGui描画
void DebugScene::ImGuiDraw() {
	// DebugSceneのImGui描画
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, DebugScene!");

	//========================================
	// TestPlayer制御
	ImGui::Separator();
	ImGui::Text("TestPlayer Control");
	ImGui::Checkbox("Show TestPlayers", &showTestPlayers_);

	if (ImGui::Button("Reset All Positions")) {
		float startX = -14.0f;
		float spacing = 3.2f;
		for (size_t i = 0; i < testPlayers_.size(); ++i) {
			Vector2 resetPos = {startX + (i * spacing), 0.0f};
			testPlayers_[i]->SetPosition(resetPos);
		}
	}

	// FPSテスト用の説明
	ImGui::Separator();
	ImGui::Text("FPS Test Players:");
	ImGui::Text("各プレイヤーは異なるFPSで動作します");
	ImGui::Text("60, 50, 40, 30, 20, 10, 8, 6, 4, 2 FPS");
	ImGui::Text("WASD/矢印キーで全プレイヤーが同時に移動");
	ImGui::Text("理論上、全プレイヤーが同じ速度で移動するはずです");

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
	// 各TestPlayerのImGui描画（コンパクトに）
	if (showTestPlayers_) {
		ImGui::Begin("All TestPlayers Status");

		if (ImGui::BeginTable("TestPlayersTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
			ImGui::TableSetupColumn("Player");
			ImGui::TableSetupColumn("Target FPS");
			ImGui::TableSetupColumn("Actual FPS");
			ImGui::TableSetupColumn("Position X");
			ImGui::TableSetupColumn("Position Y");
			ImGui::TableSetupColumn("Update Rate");
			ImGui::TableSetupColumn("Grounded");
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < testPlayers_.size(); ++i) {
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%zu", i + 1);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.0f", targetFPSList_[i]);

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.1f", testPlayers_[i]->GetCurrentFPS());

				Vector2 pos = testPlayers_[i]->GetPosition();
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.2f", pos.x);

				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.2f", pos.y);

				ImGui::TableSetColumnIndex(5);
				float rate = (targetFPSList_[i] > 0 ? (testPlayers_[i]->GetCurrentFPS() / targetFPSList_[i]) * 100.0f : 0.0f);
				ImGui::Text("%.1f%%", rate);

				ImGui::TableSetColumnIndex(6);
				ImGui::Text("%s", testPlayers_[i]->IsGrounded() ? "Yes" : "No");
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}
}

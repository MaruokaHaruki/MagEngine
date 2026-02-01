/*********************************************************************
 * \file   DebugScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "DebugScene.h"
#include "CameraManager.h"
#include "ParticlePreset.h"
using namespace MagEngine;

///=============================================================================
///						初期化
void DebugScene::Initialize(MagEngine::SpriteSetup *spriteSetup,
							MagEngine::Object3dSetup *object3dSetup,
							MagEngine::ParticleSetup *particleSetup,
							MagEngine::SkyboxSetup *skyboxSetup,
							MagEngine::CloudSetup *cloudSetup) {
	spriteSetup;
	particleSetup;

	// object3dSetupを保存（再読み込み時に使用）
	object3dSetup_ = object3dSetup;

	///--------------------------------------------------------------
	///						 音声クラス
	audio_ = MAudioG::GetInstance();

	///--------------------------------------------------------------
	///						 2D系クラス
	//========================================
	//// テクスチャマネージャ
	TextureManager::GetInstance()->LoadTexture("rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("qwantani_dusk_2_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("overcast_soil_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("moonless_golf_4k.dds");
	TextureManager::GetInstance()->LoadTexture("kloppenheim_02_puresky_4k.dds");
	//========================================
	// スプライトクラス(Game)

	///--------------------------------------------------------------
	///						 3D系クラス
	// モデルの読み込み
	ModelManager::GetInstance()->LoadModel("axisPlus.obj");
	ModelManager::GetInstance()->LoadModel("ball.obj");
	ModelManager::GetInstance()->LoadModel("terrain.obj");
	ModelManager::GetInstance()->LoadModel("jet.obj"); // モデルは事前にロードしておく
	//========================================
	// 3Dオブジェクトクラス
	// 映り込みの設定
	ModelManager::GetInstance()->GetModelSetup()->SetEnvironmentTexture("moonless_golf_4k.dds");
	// モンスターボール
	objMonsterBall_ = std::make_unique<Object3d>();
	objMonsterBall_->Initialize(object3dSetup);
	objMonsterBall_->SetModel("ball.obj");
	objMonsterBall_->SetEnvironmentMapEnabled(true);
	// 地面
	objTerrain_ = std::make_unique<Object3d>();
	objTerrain_->Initialize(object3dSetup);
	objTerrain_->SetModel("terrain.obj");
	objTerrain_->SetEnvironmentMapEnabled(true);

	//========================================
	// レベルデータローダーの初期化と読み込み
	levelDataLoader_ = std::make_unique<LevelDataLoader>();
	levelDataLoader_->Initialize();
	// テスト用JSONファイルを読み込み（ファイルパスは適宜変更）
	bool loadResult = levelDataLoader_->LoadLevelFromJson("resources/levels/test.json");
	if (loadResult) {
		// レベルデータからObject3Dを作成
		levelDataLoader_->CreateObjectsFromLevelData(object3dSetup, levelObjects_);
	}

	///--------------------------------------------------------------
	///						 パーティクル系
	//========================================
	// パーティクルの作成
	particle_ = std::make_unique<Particle>();
	particle_->Initialize(particleSetup);

	// パーティクルのグループを作成
	particle_->CreateParticleGroup("Test", "gradationLine_top.png", ParticleShape::Board);

	//========================================
	// エミッターの作成（プリセットを使用）
	particleEmitter_ = std::make_unique<ParticleEmitter>(
		particle_.get(),
		"Test",
		Transform{{0.2f, 0.2f, 0.2f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
		1024,
		0.01f,
		true);

	// プリセットを適用（炎エフェクト）
	particleEmitter_->ApplyConfig(ParticlePresets::Fire());

	///--------------------------------------------------------------
	///						 Skybox系
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(skyboxSetup);
	// Skyboxのモデルを設定
	skybox_->SetTexture("moonless_golf_4k.dds");

	///--------------------------------------------------------------
	///						 Cloud系
	cloud_ = std::make_unique<Cloud>();
	cloud_->Initialize(cloudSetup);

	// 雲の初期設定 - より確実に見える設定
	cloud_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});
	cloud_->SetSize(Vector3{100.0f, 100.0f, 100.0f});

	// 確実に見えるパラメータ
	cloud_->GetMutableParams().density = 3.0f;	 // 高密度
	cloud_->GetMutableParams().coverage = 0.25f; // 低いカバレッジ
	cloud_->GetMutableParams().stepSize = 4.0f;
	cloud_->GetMutableParams().baseNoiseScale = 0.008f;
	cloud_->GetMutableParams().detailNoiseScale = 0.025f;
	cloud_->GetMutableParams().ambient = 0.4f;
	cloud_->GetMutableParams().sunIntensity = 2.0f;
	cloud_->GetMutableParams().debugFlag = 0.0f; // 通常モード
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
	// レベルデータオブジェクトの更新
	for (auto &levelObj : levelObjects_) {
		if (levelObj) {
			levelObj->Update();
		}
	}

	//========================================
	// パーティクル系
	particleEmitter_->Update();

	//========================================
	// 音声の再生
	if (audio_->IsWavPlaying("Duke_Ellington.wav") == false) {
		// audio_->PlayWavReverse("Duke_Ellington.wav", true, 1.0f, 1.0f);
		// audio_->PlayWav("Duke_Ellington.wav", true, 1.0f, 1.0f);
	}

	//=========================================
	// Skyboxの更新
	if (skybox_) {
		skybox_->Update();
	}

	//========================================
	// Cloudの更新
	cloud_->Update(*CameraManager::GetInstance()->GetCamera("DebugCamera"), 1.0f / 60.0f);

	//========================================
	// 雲の穴開けテスト（デバッグ用）
	auto input = Input::GetInstance();
	auto camera = CameraManager::GetInstance()->GetCamera("DebugCamera");

	// Bキー: カメラ位置から前方に弾痕を作成
	if (input->PushKey(DIK_B)) {
		Vector3 cameraPos = camera->GetTransform().translate;
		// カメラの前方ベクトルを計算
		float yaw = camera->GetTransform().rotate.y;
		Vector3 forward = {sin(yaw), 0.0f, cos(yaw)};

		cloud_->AddBulletHole(
			cameraPos,
			forward,
			bulletHoleStartRadius_,
			bulletHoleEndRadius_,
			bulletHoleConeLength_,
			bulletHoleLifeTime_);
		Logger::Log("Added bullet hole at camera position", Logger::LogLevel::Info);
	}

	// Nキー: ランダムな位置に弾痕を作成
	if (input->PushKey(DIK_N)) {
		// 雲の範囲内でランダムな位置を生成
		Vector3 cloudPos = cloud_->GetMutableParams().cloudCenter;
		Vector3 cloudSize = cloud_->GetMutableParams().cloudSize;

		float randomX = cloudPos.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.x;
		float randomY = cloudPos.y + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.y;
		float randomZ = cloudPos.z + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.z;

		// ランダムな方向
		float randomYaw = static_cast<float>(rand()) / RAND_MAX * 3.14159f * 2.0f;
		Vector3 randomDir = {sin(randomYaw), 0.0f, cos(randomYaw)};

		cloud_->AddBulletHole(
			Vector3{randomX, randomY, randomZ},
			randomDir,
			bulletHoleStartRadius_,
			bulletHoleEndRadius_,
			bulletHoleConeLength_,
			bulletHoleLifeTime_);
		Logger::Log("Added random bullet hole", Logger::LogLevel::Info);
	}

	// Mキー: すべての弾痕をクリア
	if (input->PushKey(DIK_M)) {
		cloud_->ClearBulletHoles();
		Logger::Log("Cleared all bullet holes", Logger::LogLevel::Info);
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

	//========================================
	// レベルデータオブジェクトの描画
	for (auto &levelObj : levelObjects_) {
		if (levelObj) {
			// levelObj->Draw();
		}
	}
}

///=============================================================================
///						パーティクル描画
void DebugScene::ParticleDraw() {
	// パーティクルの描画
	// particleEmitter_->Draw();
}

///=============================================================================
///						Skybox描画
void DebugScene::SkyboxDraw() {
	// Skyboxの描画
	if (skybox_) {
		// skybox_->Draw();
	}
}

///=============================================================================
///						Cloud描画
void DebugScene::CloudDraw() {
	// Cloudの描画
	cloud_->Draw();
}

///=============================================================================
///						ImGui描画
void DebugScene::ImGuiDraw() {
	// DebugSceneのImGui描画
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, DebugScene!");
	ImGui::End();

	//========================================
	// Cloudのデバッグ表示
	if (cloud_) {
		cloud_->DrawImGui();

		// 雲の穴開けテスト用UI
		ImGui::Begin("Cloud Bullet Hole Test");
		ImGui::Text("Controls:");
		ImGui::BulletText("B Key: Add bullet hole at camera position");
		ImGui::BulletText("N Key: Add random bullet hole");
		ImGui::BulletText("M Key: Clear all bullet holes");
		ImGui::Separator();

		ImGui::Text("Bullet Hole Parameters (Cone Shape):");
		ImGui::SliderFloat("Start Radius (Entry)", &bulletHoleStartRadius_, 0.1f, 5.0f);
		ImGui::SliderFloat("End Radius (Exit)", &bulletHoleEndRadius_, 0.05f, 2.0f);
		ImGui::SliderFloat("Cone Length", &bulletHoleConeLength_, 1.0f, 30.0f);
		ImGui::SliderFloat("Life Time", &bulletHoleLifeTime_, 1.0f, 30.0f);
		ImGui::Separator();

		// マニュアル追加ボタン
		ImGui::Text("Manual Add:");
		ImGui::DragFloat3("Origin", &manualBulletOrigin_.x, 1.0f, -1000.0f, 1000.0f);
		ImGui::DragFloat3("Direction", &manualBulletDirection_.x, 0.01f, -1.0f, 1.0f);
		if (ImGui::Button("Add Manual Bullet Hole")) {
			// 方向ベクトルを正規化
			float length = sqrt(manualBulletDirection_.x * manualBulletDirection_.x +
								manualBulletDirection_.y * manualBulletDirection_.y +
								manualBulletDirection_.z * manualBulletDirection_.z);
			if (length > 0.0f) {
				manualBulletDirection_.x /= length;
				manualBulletDirection_.y /= length;
				manualBulletDirection_.z /= length;
			}
			cloud_->AddBulletHole(manualBulletOrigin_, manualBulletDirection_,
								  bulletHoleStartRadius_, bulletHoleEndRadius_,
								  bulletHoleConeLength_, bulletHoleLifeTime_);
			Logger::Log("Added manual bullet hole", Logger::LogLevel::Info);
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear All")) {
			cloud_->ClearBulletHoles();
		}
		ImGui::End();
	}

	//========================================
	// 3DオブジェクトのImGui描画
	// ライトの設定
	ImGui::Begin("3DObject");
	ImGui::Text("TransformSetting");
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -180.0f, 180.0f);
	ImGui::SliderFloat3("Translate", &transform.translate.x, -10.0f, 10.0f);
	ImGui::Separator();

	//========================================
	// レベルデータローダー情報表示
	ImGui::Text("Level Data Loader");
	if (levelDataLoader_) {
		ImGui::Text("Loaded: %s", levelDataLoader_->IsLoaded() ? "Yes" : "No");
		if (levelDataLoader_->IsLoaded()) {
			const auto &levelData = levelDataLoader_->GetLevelData();
			ImGui::Text("Scene Name: %s", levelData.name.c_str());
			ImGui::Text("Root Objects: %zu", levelData.objects.size());
			ImGui::Text("Created Object3D Count: %zu", levelObjects_.size());
		}
		// 再読み込みボタン
		if (ImGui::Button("Reload Level Data")) {
			levelObjects_.clear(); // 既存オブジェクトをクリア
			bool reloadResult = levelDataLoader_->LoadLevelFromJson("resources/levels/test.json");
			if (reloadResult) {
				// レベルデータからObject3Dを再作成
				levelDataLoader_->CreateObjectsFromLevelData(object3dSetup_, levelObjects_);
			}
		}

		ImGui::Separator();
		// レベルオブジェクトの操作UI
		ImGui::Text("Level Object Controls");
		levelDataLoader_->ImGuiDraw(levelObjects_);
	}

	//========================================
	// SkyBoxの移動
	ImGui::Separator();
	// 移動
	ImGui::SliderFloat3("Skybox Position", &skybox_->GetTransform()->translate.x, -10.0f, 10.0f);
	// 回転
	ImGui::SliderFloat3("Skybox Rotation", &skybox_->GetTransform()->rotate.x, -180.0f, 180.0f);
	// スケール
	ImGui::SliderFloat3("Skybox Scale", &skybox_->GetTransform()->scale.x, 0.1f, 10.0f);

	ImGui::End();

	DebugTextManager::GetInstance()->AddText3D("Hello, DebugScene!", Vector3{0.0f, 0.0f, 0.0f}, Vector4{1.0f, 1.0f, 1.0f, 1.0f}, -1.0f, 1.0f);
}

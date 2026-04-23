/*********************************************************************
 * \file   DebugScene.cpp
 * \brief  デバッグシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#define _USE_MATH_DEFINES
 // 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "DebugScene.h"
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "Input.h"
#include "LevelDataLoader.h"
#include "Logger.h"
#include "MAudioG.h"
#include "ModelManager.h"
#include "ParticlePreset.h"
#include "SceneContext.h"
#include "TrailEffectManager.h"
#include "TrailEffectPreset.h"
#include "imgui.h"
using namespace MagEngine;

///=============================================================================
/// 初期化
/// NOTE: contextからセットアップを取得
void DebugScene::Initialize(SceneContext *context) {
	//========================================
	// NOTE: contextがnullptrでないかチェック
	if (!context) {
		return;
	}

	// NOTE: contextからセットアップを取得
	MagEngine::Object3dSetup *object3dSetup = context->GetObject3dSetup();
	MagEngine::CloudSetup *cloudSetup = context->GetCloudSetup();
	MagEngine::TrailEffectManager *trailEffectManager = context->GetTrailEffectManager();

	// object3dSetupを保存（再読み込み時に使用）
	object3dSetup_ = object3dSetup;

	///--------------------------------------------------------------
	/// 音声クラス
	audio_ = MAudioG::GetInstance();

	///--------------------------------------------------------------
	/// 2D系クラス
	//========================================
	//// テクスチャマネージャ

	//========================================
	// スプライトクラス(Game)

	///--------------------------------------------------------------
	/// 3D系クラス
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

	///--------------------------------------------------------------
	///						 Skybox系

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

	///--------------------------------------------------------------
	///						 TrailEffect系
	// 受け取ったマネージャーポインターを保存
	trailEffectManager_ = trailEffectManager;

	///--------------------------------------------------------------
	///						 マイク入力系
	// マイク入力ブリッジの初期化
	voiceBridge_ = std::make_unique<MagVoiceBridge>();
	if (voiceBridge_->Initialize()) {
		Logger::Log("MagVoiceBridge initialized successfully", Logger::LogLevel::Info);
	} else {
		Logger::Log("Failed to initialize MagVoiceBridge", Logger::LogLevel::Error);
	}

	///--------------------------------------------------------------
}

///=============================================================================
///						終了処理
void DebugScene::Finalize() {
	// マイク入力の停止とシャットダウン
	if (voiceBridge_) {
		voiceBridge_->Stop();
		voiceBridge_->Shutdown();
	}
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

	//========================================
	// 音声の再生

	//=========================================
	// マイク入力データの取得と処理
	if (voiceBridge_) {
		// マイク入力を更新（フレーム毎に呼び出す）
		voiceBridge_->Update();
		
		// 現在の記録中はサンプルバッファからデータを取得
		if (voiceIsRecording_) {
			voiceDisplaySamples_ = voiceBridge_->GetSamples();
		}
	}

	//=========================================
	// Skyboxの更新

	//========================================
	// Cloudの更新
	cloud_->Update(*CameraManager::GetInstance()->GetCamera("DebugCamera"), 1.0f / 60.0f);

	//========================================
	// TrailEffectの更新
	if (trailEffectManager_) {

		trailEffectManager_->Update(1.0f / 60.0f);

		trailLoopTimer_ += 1.0f / 60.0f * trailLoopSpeed_;
		if (trailLoopTimer_ >= 2.0f * 3.14159f) {
			trailLoopTimer_ -= 2.0f * 3.14159f; // リセット
		}

		// 円形パスを計算
		float angle = trailLoopTimer_;
		Vector3 currentPos = {
			trailLoopCenter_.x + cos(angle) * trailLoopRadius_,
			trailLoopCenter_.y + sin(angle) * 3.0f, // 簡単な上下動
			trailLoopCenter_.z + sin(angle) * trailLoopRadius_};

		// トレイルを発生させる
		auto trailEffect = trailEffectManager_->GetEffect("test_trail");
		if (trailEffect) {
			Vector3 velocity = Vector3{-sin(angle) * trailLoopSpeed_ * trailLoopRadius_, cos(angle) * 3.0f, cos(angle) * trailLoopSpeed_ * trailLoopRadius_};
			trailEffect->EmitAt(currentPos, velocity);
		}
	}

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

	//========================================
	// レベルデータオブジェクトの描画
	for (auto &levelObj : levelObjects_) {
		if (levelObj) {
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
}

///=============================================================================
///						Cloud描画
void DebugScene::CloudDraw() {
	// Cloudの描画
	cloud_->Draw();
}

///=============================================================================
///						TrailEffect描画
void DebugScene::TrailEffectDraw() {
	// TrailEffectの描画
	if (trailEffectManager_) {
		trailEffectManager_->Draw();
	}
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
#if ENABLE_IMGUI
#ifdef _DEBUG
		cloud_->DrawImGui();
#endif // _DEBUG
#endif // ENABLE_IMGUI

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
	// TrailEffectのデバッグ表示
	// トレイルループテスト用UI
	if (trailEffectManager_) {
		ImGui::Begin("Trail Loop Test");
		ImGui::Text("Trail Loop Animation");
		ImGui::SliderFloat("Loop Radius##trail", &trailLoopRadius_, 5.0f, 50.0f);
		ImGui::SliderFloat("Loop Height##trail", &trailLoopHeight_, 10.0f, 100.0f);
		ImGui::SliderFloat("Loop Speed##trail", &trailLoopSpeed_, 0.5f, 5.0f);
		ImGui::DragFloat3("Loop Center##trail", &trailLoopCenter_.x, 1.0f);

		ImGui::Separator();
		ImGui::Text("Current Position: (%.1f, %.1f, %.1f)",
					trailLoopCenter_.x + cos(trailLoopTimer_) * trailLoopRadius_,
					trailLoopCenter_.y + sin(trailLoopTimer_) * 3.0f,
					trailLoopCenter_.z + sin(trailLoopTimer_) * trailLoopRadius_);
		ImGui::Text("Timer: %.2f / %.2f", trailLoopTimer_, 2.0f * 3.14159f);

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

	ImGui::End();

	//========================================
	// SkyBoxの移動

	//========================================
	// マイク入力テスト用UI
	if (voiceBridge_) {
		ImGui::Begin("Voice Input Monitor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		// ===== デバイス情報 =====
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "DEVICE INFORMATION");
		ImGui::Separator();
		ImGui::Text("Sample Rate: %u Hz | Channels: %u | Bits: %u", 
		           voiceBridge_->GetSampleRate(), voiceBridge_->GetChannelCount(), 
		           16);  // WASAPI Shared Mode は常に 32-bit float

		ImGui::Spacing();
		ImGui::Spacing();

		// ===== 記録制御 =====
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "RECORDING CONTROL");
		ImGui::Separator();
		
		if (!voiceIsRecording_) {
			if (ImGui::Button("Start Recording##voice", ImVec2(150, 35))) {
				if (voiceBridge_->Start()) {
					voiceIsRecording_ = true;
					voiceDisplaySamples_.clear();
					Logger::Log("Started microphone recording", Logger::LogLevel::Info);
				} else {
					Logger::Log("Failed to start microphone recording", Logger::LogLevel::Error);
				}
			}
		} else {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "●  RECORDING");
			ImGui::SameLine();
			if (ImGui::Button("Stop Recording##voice", ImVec2(150, 35))) {
				voiceBridge_->Stop();
				voiceIsRecording_ = false;
				Logger::Log("Stopped microphone recording", Logger::LogLevel::Info);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear Buffer##voice", ImVec2(120, 35))) {
			voiceDisplaySamples_.clear();
			voiceBridge_->ClearSamples();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		// ===== リアルタイム音量分析 =====
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "REAL-TIME ANALYSIS");
		ImGui::Separator();

		// VolumeStats 構造体から全ての値を取得
		auto stats = voiceBridge_->GetVolumeStats();

		// --- 現在の RMS 値 ---
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "RMS Volume (Current)");
		ImGui::Text("  Normalized: %.4f (0.0 ~ 1.0)", stats.currentRMS);
		ImGui::Text("  Decibels:   %.2f dB", stats.currentRMSDB);
		ImGui::ProgressBar(stats.currentRMS, ImVec2(-1, 18.0f), "");
		
		ImGui::Spacing();

		// --- ピークレベル ---
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Peak Volume (Instantaneous Max)");
		ImGui::Text("  Normalized: %.4f (0.0 ~ 1.0)", stats.peakValue);
		ImGui::Text("  Decibels:   %.2f dB", stats.peakDB);
		ImGui::ProgressBar(stats.peakValue, ImVec2(-1, 18.0f), "");

		ImGui::Spacing();

		// --- スムージング済み音量 ---
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Smoothed Volume (UI-Friendly)");
		ImGui::Text("  Normalized: %.4f (0.0 ~ 1.0)", stats.smoothedRMS);
		ImGui::Text("  Decibels:   %.2f dB", stats.smoothedRMSDB);
		ImGui::Text("  Percentage: %.1f %%", stats.percentage);
		ImGui::ProgressBar(stats.smoothedRMS, ImVec2(-1, 18.0f), "");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ===== 音声検出情報 =====
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "VOICE DETECTION");
		ImGui::Separator();

		// 音声検出ステータス
		if (stats.isVoiceDetected) {
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Voice Detected: YES");
		} else {
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "✗ Voice Detected: NO");
		}

		// 音声特性スコア（0.0～1.0、1.0に近いほど人の声）
		ImGui::Text("Voice Characteristic Score: %.2f (0.0 ~ 1.0)", stats.voiceScore);
		ImGui::ProgressBar(stats.voiceScore, ImVec2(-1, 20.0f), "");
		if (stats.voiceScore > 0.7f) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Human Voice");
		} else if (stats.voiceScore > 0.4f) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Uncertain");
		} else {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Noise");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ===== デバッグ情報 =====
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "DEBUG INFORMATION");
		ImGui::Separator();

		// ゼロクロス率の詳細
		float zeroCrossingRate = voiceBridge_->CalculateZeroCrossingRate();
		ImGui::Text("Zero Crossing Rate: %.4f", zeroCrossingRate);
		ImGui::SameLine();
		ImGui::TextDisabled("(Threshold: 0.25)");
		
		// ZCR判定
		if (zeroCrossingRate < 0.25f) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ OK (Low)");
		} else {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ NG (High)");
		}

		// 音量判定
		ImGui::Text("RMS Volume:          %.2f dB", stats.currentRMSDB);
		ImGui::SameLine();
		ImGui::TextDisabled("(Threshold: -40dB)");
		
		if (stats.currentRMSDB > -40.0f) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ OK (Loud)");
		} else {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ NG (Quiet)");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ===== パラメータ調整セクション =====
		ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "PARAMETER TUNING");
		ImGui::Separator();

		// スムージング係数
		static float smoothingFactor = 0.4f;
		if (ImGui::SliderFloat("Smoothing Factor##voice", &smoothingFactor, 0.0f, 1.0f, "%.3f")) {
			voiceBridge_->SetSmoothingFactor(smoothingFactor);
		}
		ImGui::TextDisabled("Low: Responsive, High: Stable");

		ImGui::Spacing();

		// ノイズフロア設定
		static float noiseFloor = -50.0f;
		if (ImGui::SliderFloat("Noise Floor##voice", &noiseFloor, -80.0f, -20.0f, "%.1f dB")) {
			voiceBridge_->SetNoiseFloor(noiseFloor);
		}
		ImGui::TextDisabled("Sounds below this level are ignored");

		ImGui::Spacing();

		// ゼロクロス率閾値
		static float zcThreshold = 0.25f;
		ImGui::SliderFloat("ZC Rate Threshold##voice", &zcThreshold, 0.05f, 0.5f, "%.3f");
		ImGui::TextDisabled("Low = More selective for voice");

		ImGui::Spacing();

		// 音量判定閾値
		static float volumeThreshold = -40.0f;
		ImGui::SliderFloat("Volume Threshold##voice", &volumeThreshold, -80.0f, -10.0f, "%.1f dB");
		ImGui::TextDisabled("Minimum volume to consider as voice");

		// パラメータ適用ボタン
		if (ImGui::Button("Apply Thresholds##voice", ImVec2(-1, 30))) {
			voiceBridge_->SetVoiceDetectionThresholds(zcThreshold, volumeThreshold);
			Logger::Log(
				"Voice detection parameters updated - ZCR: " + std::to_string(zcThreshold) + 
				", Volume: " + std::to_string(volumeThreshold) + " dB",
				Logger::LogLevel::Info);
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ===== 波形表示設定 =====
		ImGui::TextColored(ImVec4(0.8f, 0.2f, 1.0f, 1.0f), "WAVEFORM DISPLAY");
		ImGui::Separator();
		
		ImGui::SliderFloat("Waveform Scale##voice", &voiceWaveformScale_, 1.0f, 500.0f, "%.0f");
		ImGui::SliderFloat("Waveform Sensitivity##voice", &voiceSensitivity_, 0.1f, 5.0f, "%.2f");

		ImGui::Spacing();

		// ===== 波形表示 =====
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "WAVEFORM");
		
		ImDrawList *waveDrawList = ImGui::GetWindowDrawList();
		ImVec2 waveCanvasPos = ImGui::GetCursorScreenPos();
		ImVec2 waveCanvasSize(ImGui::GetContentRegionAvail().x, 150.0f);

		// 背景
		waveDrawList->AddRectFilled(
			waveCanvasPos, 
			ImVec2(waveCanvasPos.x + waveCanvasSize.x, waveCanvasPos.y + waveCanvasSize.y),
			ImGui::GetColorU32(ImVec4(0.05f, 0.05f, 0.1f, 1.0f)));
		
		// 枠線
		waveDrawList->AddRect(
			waveCanvasPos, 
			ImVec2(waveCanvasPos.x + waveCanvasSize.x, waveCanvasPos.y + waveCanvasSize.y),
			ImGui::GetColorU32(ImGuiCol_Border));

		if (voiceDisplaySamples_.size() > 1) {
			// 中央線（ゼロライン）
			float centerY = waveCanvasPos.y + waveCanvasSize.y * 0.5f;
			waveDrawList->AddLine(
				ImVec2(waveCanvasPos.x, centerY), 
				ImVec2(waveCanvasPos.x + waveCanvasSize.x, centerY),
				ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.5f)), 
				1.0f);

			// +1.0 / -1.0 ラインを描画
			float topLineY = waveCanvasPos.y + (waveCanvasSize.y * 0.1f);
			float bottomLineY = waveCanvasPos.y + (waveCanvasSize.y * 0.9f);
			waveDrawList->AddLine(
				ImVec2(waveCanvasPos.x, topLineY), 
				ImVec2(waveCanvasPos.x + waveCanvasSize.x, topLineY),
				ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.5f, 0.3f)), 
				1.0f);
			waveDrawList->AddLine(
				ImVec2(waveCanvasPos.x, bottomLineY), 
				ImVec2(waveCanvasPos.x + waveCanvasSize.x, bottomLineY),
				ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.5f, 0.3f)), 
				1.0f);

			// ダウンサンプリング
			size_t displaySamples = std::min(voiceDisplaySamples_.size(), size_t(waveCanvasSize.x * 2));
			size_t skipSamples = std::max(size_t(1), voiceDisplaySamples_.size() / displaySamples);

			// 波形描画
			for (size_t i = 0; i + skipSamples < displaySamples; ++i) {
				float x1 = waveCanvasPos.x + (i * waveCanvasSize.x) / displaySamples;
				float y1 = centerY - voiceDisplaySamples_[i * skipSamples] * voiceWaveformScale_ * voiceSensitivity_;
				y1 = std::max(waveCanvasPos.y, std::min(y1, waveCanvasPos.y + waveCanvasSize.y));

				float x2 = waveCanvasPos.x + ((i + 1) * waveCanvasSize.x) / displaySamples;
				float y2 = centerY - voiceDisplaySamples_[(i + 1) * skipSamples] * voiceWaveformScale_ * voiceSensitivity_;
				y2 = std::max(waveCanvasPos.y, std::min(y2, waveCanvasPos.y + waveCanvasSize.y));

				waveDrawList->AddLine(
					ImVec2(x1, y1), 
					ImVec2(x2, y2),
					ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.5f, 0.9f)), 
					1.5f);
			}
		} else {
			ImGui::SetCursorScreenPos(ImVec2(waveCanvasPos.x + 10, waveCanvasPos.y + 60));
			ImGui::TextDisabled("Waiting for audio data...");
		}

		ImGui::Dummy(waveCanvasSize);

		ImGui::End();
	}

	//========================================
	// SkyBoxの移動

	DebugTextManager::GetInstance()->AddText3D("Hello, DebugScene!", Vector3{0.0f, 0.0f, 0.0f}, Vector4{1.0f, 1.0f, 1.0f, 1.0f}, -1.0f, 1.0f);
}

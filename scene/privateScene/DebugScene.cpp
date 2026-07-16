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
using namespace MagMath;
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "EditorUiSystem.h"
#include "EngineContext.h"
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
#include <cassert>
using namespace MagEngine;

namespace {
	Vector3 NormalizeOrDefaultLocal(const Vector3 &value, const Vector3 &fallback) {
		float length = sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
		if (length <= 0.0001f) {
			return fallback;
		}
		return Vector3{value.x / length, value.y / length, value.z / length};
	}

	Vector3 GetCameraForward(const Transform &transform) {
		MagMath::Matrix4x4 rotationMatrix = MagMath::MakeRotateMatrix(transform.rotate);
		Vector3 forward = {rotationMatrix.m[0][2], rotationMatrix.m[1][2], rotationMatrix.m[2][2]};
		return NormalizeOrDefaultLocal(forward, Vector3{0.0f, 0.0f, 1.0f});
	}

	Camera *GetCloudTestCamera(CameraManager &cameraManager) {
		if (auto debugCamera = cameraManager.GetCamera("DebugCamera")) {
			return debugCamera;
		}
		return cameraManager.GetCurrentCamera();
	}

	bool IntersectRayAabb(const Vector3 &rayOrigin, const Vector3 &rayDirection, const Vector3 &boxCenter, const Vector3 &boxSize, float &hitDistance) {
		Vector3 boxMin = {
			boxCenter.x - boxSize.x * 0.5f,
			boxCenter.y - boxSize.y * 0.5f,
			boxCenter.z - boxSize.z * 0.5f};
		Vector3 boxMax = {
			boxCenter.x + boxSize.x * 0.5f,
			boxCenter.y + boxSize.y * 0.5f,
			boxCenter.z + boxSize.z * 0.5f};

		float tMin = -1000000.0f;
		float tMax = 1000000.0f;
		const float epsilon = 0.0001f;

		auto testAxis = [&](float origin, float direction, float minValue, float maxValue) {
			if (std::abs(direction) < epsilon) {
				return origin >= minValue && origin <= maxValue;
			}

			float invDir = 1.0f / direction;
			float t1 = (minValue - origin) * invDir;
			float t2 = (maxValue - origin) * invDir;
			if (t1 > t2) {
				float temp = t1;
				t1 = t2;
				t2 = temp;
			}

			if (t1 > tMin) {
				tMin = t1;
			}
			if (t2 < tMax) {
				tMax = t2;
			}
			return tMin <= tMax;
		};

		if (!testAxis(rayOrigin.x, rayDirection.x, boxMin.x, boxMax.x) ||
			!testAxis(rayOrigin.y, rayDirection.y, boxMin.y, boxMax.y) ||
			!testAxis(rayOrigin.z, rayDirection.z, boxMin.z, boxMax.z) ||
			tMax < 0.0f) {
			return false;
		}

		hitDistance = (tMin > 0.0f) ? tMin : 0.0f;
		return true;
	}
}

///=============================================================================
/// 初期化
/// NOTE: contextからセットアップを取得
void DebugScene::Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) {
	engineContext.Validate();
	engineContext_ = &engineContext;
	sceneContext_ = &sceneContext;

	// NOTE: EngineサービスはEngineContextから取得し、SceneContextとの重複を避ける
	MagEngine::Object3dSetup *object3dSetup = engineContext_->object3dSetup;
	MagEngine::CloudSetup *cloudSetup = engineContext_->cloudSetup;
	MagEngine::TrailEffectManager *trailEffectManager = engineContext_->trailEffectManager;
	MagEngine::ModelManager *modelManager = engineContext_->modelManager;

	// object3dSetupを保存（再読み込み時に使用）
	object3dSetup_ = object3dSetup;

	///--------------------------------------------------------------
	/// 音声クラス
	audio_ = engineContext_->audio;

	///--------------------------------------------------------------
	/// 2D系クラス
	//========================================
	//// テクスチャマネージャ

	//========================================
	// スプライトクラス(Game)

	///--------------------------------------------------------------
	/// 3D系クラス
	// モデルの読み込み
	modelManager->LoadModel("axisPlus.obj");
	modelManager->LoadModel("ball.obj");
	modelManager->LoadModel("terrain.obj");
	modelManager->LoadModel("jet.obj"); // モデルは事前にロードしておく
	//========================================
	// 3Dオブジェクトクラス
	// 映り込みの設定
	modelManager->GetModelSetup()->SetEnvironmentTexture("moonless_golf_4k.dds");
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

	// 原点基準で穴あけの挙動を確認しやすくするため、雲の中心をワールド原点に固定する。
	cloud_->SetSize(Vector3{500.0f, 500.0f, 500.0f});
	cloud_->SetEnabled(true);
	cloud_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});

	// 穴の輪郭を確認しやすいように、ゲームプレイより濃いデバッグ用密度にする。
	cloud_->GetMutableParams().density = 4.5f;
	cloud_->GetMutableParams().coverage = 0.20f;
	cloud_->GetMutableParams().stepSize = 4.0f;

	cloud_->GetMutableParams().baseNoiseScale = 0.007f;
	cloud_->GetMutableParams().detailNoiseScale = 0.025f;
	cloud_->GetMutableParams().ambient = 0.82f;
	cloud_->GetMutableParams().sunIntensity = 1.6f;
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
	RegisterEditorPanels();
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
void DebugScene::Update(const FrameTime &frameTime) {
	const float deltaTime = frameTime.unscaledDeltaTime;
	///--------------------------------------------------------------
	///						更新処理

	//========================================
	// カメラの更新

	//========================================
	// 2D更新

	//========================================
	// 3D更新
	// モンスターボール
	if (objMonsterBall_) {
		objMonsterBall_->SetScale(Vector3{transform.scale.x, transform.scale.y, transform.scale.z});
		objMonsterBall_->SetRotation(Vector3{transform.rotate.x, transform.rotate.y, transform.rotate.z});
		objMonsterBall_->SetPosition(Vector3{transform.translate.x, transform.translate.y, transform.translate.z});
		objMonsterBall_->Update();
	}
	// 地面
	if (objTerrain_) {
		objTerrain_->SetScale(Vector3{1.0f, 1.0f, 1.0f});
		objTerrain_->SetRotation(Vector3{0.0f, 0.0f, 0.0f});
		objTerrain_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});
		objTerrain_->Update();
	}

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
	if (cloud_ && enableCloudTest_) {
		if (auto cloudCamera = GetCloudTestCamera(*engineContext_->cameraManager)) {
			cloud_->Update(*cloudCamera, 1.0f / 60.0f);
		}
	}

	//========================================
	// TrailEffectの更新
	if (trailEffectManager_) {

		trailEffectManager_->Update(1.0f / 60.0f);

		if (enableTrailLoop_) {
			trailLoopTimer_ += 1.0f / 60.0f * trailLoopSpeed_;
			if (trailLoopTimer_ >= 2.0f * 3.14159f) {
				trailLoopTimer_ -= 2.0f * 3.14159f; // リセット
			}

			// 円形パスを計算
			float angle = trailLoopTimer_;
			Vector3 currentPos = {
				trailLoopCenter_.x + cos(angle) * trailLoopRadius_,
				trailLoopCenter_.y + sin(angle) * trailLoopHeight_,
				trailLoopCenter_.z + sin(angle) * trailLoopRadius_};

			// トレイルを発生させる
			auto trailEffect = trailEffectManager_->GetEffect("test_trail");
			if (trailEffect) {
				Vector3 velocity = Vector3{-sin(angle) * trailLoopSpeed_ * trailLoopRadius_, cos(angle) * trailLoopSpeed_ * trailLoopHeight_, cos(angle) * trailLoopSpeed_ * trailLoopRadius_};
				trailEffect->EmitAt(currentPos, velocity);
			}
		}
	}

	//========================================
	// 雲の穴開けテスト（デバッグ用）
	assert(engineContext_);
	Input *input = engineContext_->input;

	// Jキー: カメラ位置から前方に弾痕を作成
	if (enableCloudTest_ && input->TriggerKey(DIK_J)) {
		AddCloudHoleFromDebugCamera();
	}

	// Kキー: ランダムな位置に弾痕を作成
	if (enableCloudTest_ && input->TriggerKey(DIK_K)) {
		AddRandomCloudHole();
	}

	// Lキー: すべての弾痕をクリア
	if (enableCloudTest_ && input->TriggerKey(DIK_L) && cloud_) {
		cloud_->ClearBulletHoles();
		Logger::Log("Cleared all bullet holes", Logger::LogLevel::Info);
	}

	if (enableCloudTest_ && input->TriggerKey(DIK_N)) {
		AdvanceCloudHoleShape(1);
	}
	if (enableCloudTest_ && input->TriggerKey(DIK_M)) {
		AdvanceCloudHoleShape(-1);
	}
	if (enableCloudTest_ && input->TriggerKey(DIK_B)) {
		AdvanceCloudHoleCategory(1);
	}
	if (enableCloudTest_ && input->TriggerKey(DIK_V)) {
		AdvanceCloudHoleCategory(-1);
	}
	if (enableCloudTest_ && input->TriggerKey(DIK_R)) {
		cloudHoleRoundEnabled_ = !cloudHoleRoundEnabled_;
	}
	if (enableCloudTest_ && input->TriggerKey(DIK_O)) {
		cloudHoleOnionEnabled_ = !cloudHoleOnionEnabled_;
	}

	if (enableCloudTest_ && autoCloudHole_) {
		autoCloudHoleTimer_ += 1.0f / 60.0f;
		if (autoCloudHoleTimer_ >= autoCloudHoleInterval_) {
			autoCloudHoleTimer_ = 0.0f;
			AddRandomCloudHole();
		}
	} else {
		autoCloudHoleTimer_ = 0.0f;
	}
}

MagEngine::CloudHoleData DebugScene::BuildSelectedCloudHole(const Vector3 &origin, const Vector3 &direction) const {
	CloudHoleData hole = MakeCloudHolePreset(cloudHoleShape_);
	hole.position = origin;
	hole.direction = NormalizeOrDefaultLocal(direction, Vector3{0.0f, 0.0f, 1.0f});
	hole.startRadius = bulletHoleStartRadius_;
	hole.endRadius = bulletHoleEndRadius_;
	hole.coneLength = bulletHoleConeLength_;
	hole.lifetime = bulletHoleLifeTime_;
	hole.maxLifetime = bulletHoleLifeTime_;
	hole.rotation = cloudHoleRotation_;
	hole.aspectRatio = cloudHoleAspectRatio_;
	if (cloudHoleRoundEnabled_) {
		hole.flags |= CloudHoleFlag_Rounded;
	}
	if (cloudHoleOnionEnabled_) {
		hole.flags |= CloudHoleFlag_Onion;
	}
	return SanitizeCloudHoleData(hole);
}

void DebugScene::AdvanceCloudHoleShape(int direction) {
	cloudHoleShape_ = AdvanceCloudHoleShapeInCategory(cloudHoleShape_, cloudHoleCategory_, direction);
}

void DebugScene::AdvanceCloudHoleCategory(int direction) {
	cloudHoleCategory_ = MagEngine::AdvanceCloudHoleCategory(cloudHoleCategory_, direction);
	cloudHoleShape_ = GetFirstShapeInCategory(cloudHoleCategory_);
}

void DebugScene::AddCloudHoleFromDebugCamera() {
	if (!cloud_) {
		return;
	}

	assert(engineContext_);
	auto camera = GetCloudTestCamera(*engineContext_->cameraManager);
	if (!camera) {
		return;
	}

	Transform cameraTransform = camera->GetTransform();
	Vector3 forward = engineContext_->cameraManager->GetDebugCameraForward();
	Vector3 origin = cameraTransform.translate;
	const auto &cloudParams = cloud_->GetParams();
	float hitDistance = 0.0f;
	if (IntersectRayAabb(cameraTransform.translate, forward, cloudParams.cloudCenter, cloudParams.cloudSize, hitDistance)) {
		// 雲の入口より少し手前から始めることで、シェーダー側の軸方向判定に確実に乗せる。
		float startDistance = hitDistance - bulletHoleStartRadius_ * 2.0f;
		if (startDistance < 0.0f) {
			startDistance = 0.0f;
		}
		origin = Vector3{
			cameraTransform.translate.x + forward.x * startDistance,
			cameraTransform.translate.y + forward.y * startDistance,
			cameraTransform.translate.z + forward.z * startDistance};
	} else {
		Logger::Log("Cloud hole ray did not hit cloud AABB", Logger::LogLevel::Warning);
	}

	cloud_->AddBulletHole(BuildSelectedCloudHole(origin, forward));
	Logger::Log("Added cloud hole from DebugCamera", Logger::LogLevel::Info);
}

void DebugScene::AddRandomCloudHole() {
	if (!cloud_) {
		return;
	}

	Vector3 cloudPos = cloud_->GetMutableParams().cloudCenter;
	Vector3 cloudSize = cloud_->GetMutableParams().cloudSize;

	float randomX = cloudPos.x + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.x * cloudHoleRandomSpread_;
	float randomY = cloudPos.y + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.y * cloudHoleRandomSpread_;
	float randomZ = cloudPos.z + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * cloudSize.z * cloudHoleRandomSpread_;

	float randomYaw = static_cast<float>(rand()) / RAND_MAX * 3.14159f * 2.0f;
	float randomPitch = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f;
	Vector3 randomDir = NormalizeOrDefaultLocal(
		Vector3{sin(randomYaw), randomPitch, cos(randomYaw)},
		Vector3{0.0f, 0.0f, 1.0f});

	cloud_->AddBulletHole(BuildSelectedCloudHole(Vector3{randomX, randomY, randomZ}, randomDir));
}

void DebugScene::AddManualCloudHole() {
	if (!cloud_) {
		return;
	}

	manualBulletDirection_ = NormalizeOrDefaultLocal(manualBulletDirection_, Vector3{0.0f, 0.0f, 1.0f});
	cloud_->AddBulletHole(BuildSelectedCloudHole(manualBulletOrigin_, manualBulletDirection_));
	Logger::Log("Added manual cloud hole", Logger::LogLevel::Info);
}

///=============================================================================
void DebugScene::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	if(drawTestObjects_) {
		if(objTerrain_) {
			objTerrain_->RegisterRenderables(renderWorld);
		}
		if(objMonsterBall_) {
			objMonsterBall_->RegisterRenderables(renderWorld);
		}
	}
	if(drawLevelObjects_) {
		for(auto &levelObj : levelObjects_) {
			if(levelObj) {
				levelObj->RegisterRenderables(renderWorld);
			}
		}
	}
	if (cloud_ && enableCloudTest_) {
		cloud_->RegisterRenderables(renderWorld);
	}
	if (trailEffectManager_) {
		trailEffectManager_->RegisterRenderables(renderWorld);
	}
}

///=============================================================================
///						ImGui描画
void DebugScene::DrawOverviewImGui() {
	ImGui::Text("DebugScene Test Hub");
	ImGui::Separator();

	if (engineContext_ && engineContext_->cameraManager) {
		if (auto debugCamera = engineContext_->cameraManager->GetCamera("DebugCamera")) {
		Transform cameraTransform = debugCamera->GetTransform();
		ImGui::Text("DebugCamera");
		ImGui::Text("Position: %.2f, %.2f, %.2f",
					cameraTransform.translate.x,
					cameraTransform.translate.y,
					cameraTransform.translate.z);
		ImGui::Text("Rotation: %.2f, %.2f, %.2f",
					cameraTransform.rotate.x,
					cameraTransform.rotate.y,
					cameraTransform.rotate.z);
		} else {
			ImGui::TextDisabled("DebugCamera is not available.");
		}
	} else {
		ImGui::TextDisabled("DebugCamera is not available.");
	}

	ImGui::Separator();
	ImGui::Checkbox("Draw Test Objects", &drawTestObjects_);
	ImGui::Checkbox("Draw Level Objects", &drawLevelObjects_);
	ImGui::Checkbox("Cloud Test Enabled", &enableCloudTest_);
	ImGui::Checkbox("Trail Loop Enabled", &enableTrailLoop_);

	ImGui::Separator();
	ImGui::Text("Scene");
	if (ImGui::Button("Title")) {
		SetSceneNo(SCENE::TITLE);
	}
	ImGui::SameLine();
	if (ImGui::Button("Gameplay")) {
		SetSceneNo(SCENE::GAMEPLAY);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		SetSceneNo(SCENE::CLEAR);
	}
}

void DebugScene::DrawCloudTestImGui() {
	ImGui::Checkbox("Enable Cloud Test", &enableCloudTest_);

	if (!cloud_) {
		ImGui::TextDisabled("Cloud is not available.");
		return;
	}

	auto &cloudParams = cloud_->GetMutableParams();
	ImGui::Text("Center: %.1f, %.1f, %.1f", cloudParams.cloudCenter.x, cloudParams.cloudCenter.y, cloudParams.cloudCenter.z);
	ImGui::Text("Size: %.1f, %.1f, %.1f", cloudParams.cloudSize.x, cloudParams.cloudSize.y, cloudParams.cloudSize.z);
	ImGui::Text("Active Holes: %d", cloudParams.bulletHoleCount);
	if (ImGui::Button("Apply Visible Preset")) {
		cloud_->SetSize(Vector3{500.0f, 100.0f, 500.0f});
		cloud_->SetPosition(Vector3{0.0f, 0.0f, 0.0f});
		cloudParams.density = 4.5f;
		cloudParams.coverage = 0.20f;
		cloudParams.ambient = 0.82f;
		cloudParams.sunIntensity = 1.6f;
		cloudParams.baseNoiseScale = 0.007f;
		cloudParams.detailNoiseScale = 0.025f;
		cloudParams.debugFlag = 0.0f;
	}

#if ENABLE_IMGUI
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Cloud Renderer Params")) {
		cloud_->DrawImGui();
	}
#endif // _DEBUG
#endif // ENABLE_IMGUI

	ImGui::Separator();
	ImGui::Text("Hole Controls");
	ImGui::BulletText("J: Add from camera forward");
	ImGui::BulletText("K: Add random hole");
	ImGui::BulletText("L: Clear all holes");
	ImGui::BulletText("N/M: Next/Previous shape");
	ImGui::BulletText("B/V: Next/Previous category");
	ImGui::BulletText("R/O: Toggle Round/Onion");

	if (ImGui::Button("Add From Camera Forward")) {
		AddCloudHoleFromDebugCamera();
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Random")) {
		AddRandomCloudHole();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All")) {
		cloud_->ClearBulletHoles();
	}

	ImGui::DragInt("Burst Count", &cloudHoleBurstCount_, 1, 1, 64);
	if (ImGui::Button("Burst Random Holes")) {
		for (int i = 0; i < cloudHoleBurstCount_; ++i) {
			AddRandomCloudHole();
		}
	}

	ImGui::Checkbox("Auto Random Holes", &autoCloudHole_);
	ImGui::SliderFloat("Auto Interval", &autoCloudHoleInterval_, 0.05f, 3.0f, "%.2f sec");
	ImGui::SliderFloat("Random Spread", &cloudHoleRandomSpread_, 0.1f, 1.5f);

	ImGui::Separator();
	ImGui::Text("Hole Shape");
	ImGui::Text("Cloud Hole Shape: %s", ToString(cloudHoleShape_).data());
	ImGui::Text("Category: %s", ToString(cloudHoleCategory_).data());
	ImGui::Text("Round: %s", cloudHoleRoundEnabled_ ? "On" : "Off");
	ImGui::Text("Onion: %s", cloudHoleOnionEnabled_ ? "On" : "Off");
	if (IsCloudHoleExperimentalShape(cloudHoleShape_)) {
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Experimental Shape Warning");
	}
	if (ImGui::Button("Previous Shape")) {
		AdvanceCloudHoleShape(-1);
	}
	ImGui::SameLine();
	if (ImGui::Button("Next Shape")) {
		AdvanceCloudHoleShape(1);
	}
	if (ImGui::Button("Previous Category")) {
		AdvanceCloudHoleCategory(-1);
	}
	ImGui::SameLine();
	if (ImGui::Button("Next Category")) {
		AdvanceCloudHoleCategory(1);
	}
	ImGui::Checkbox("Round Modifier", &cloudHoleRoundEnabled_);
	ImGui::Checkbox("Onion Modifier", &cloudHoleOnionEnabled_);
	ImGui::SliderFloat("Rotation", &cloudHoleRotation_, -3.14159f, 3.14159f, "%.2f rad");
	ImGui::SliderFloat("Aspect Ratio", &cloudHoleAspectRatio_, 0.05f, 5.0f, "%.2f");
	ImGui::SliderFloat("Start Radius", &bulletHoleStartRadius_, 0.1f, 20.0f);
	ImGui::SliderFloat("End Radius", &bulletHoleEndRadius_, 0.05f, 20.0f);
	ImGui::SliderFloat("Cone Length", &bulletHoleConeLength_, 1.0f, 2000.0f);
	ImGui::SliderFloat("Life Time", &bulletHoleLifeTime_, 0.1f, 60.0f);

	ImGui::Separator();
	ImGui::Text("Manual Hole");
	ImGui::DragFloat3("Origin", &manualBulletOrigin_.x, 1.0f, -2000.0f, 2000.0f);
	ImGui::DragFloat3("Direction", &manualBulletDirection_.x, 0.01f, -1.0f, 1.0f);
	if (ImGui::Button("Add Manual Hole")) {
		AddManualCloudHole();
	}
}

void DebugScene::DrawTrailTestImGui() {
	ImGui::Checkbox("Enable Trail Loop", &enableTrailLoop_);

	if (!trailEffectManager_) {
		ImGui::TextDisabled("TrailEffectManager is not available.");
		return;
	}

	ImGui::SliderFloat("Loop Radius", &trailLoopRadius_, 0.1f, 50.0f);
	ImGui::SliderFloat("Loop Height", &trailLoopHeight_, 0.1f, 100.0f);
	ImGui::SliderFloat("Loop Speed", &trailLoopSpeed_, 0.1f, 10.0f);
	ImGui::DragFloat3("Loop Center", &trailLoopCenter_.x, 0.5f);

	ImGui::Separator();
	float angle = trailLoopTimer_;
	Vector3 currentPos = {
		trailLoopCenter_.x + cos(angle) * trailLoopRadius_,
		trailLoopCenter_.y + sin(angle) * trailLoopHeight_,
		trailLoopCenter_.z + sin(angle) * trailLoopRadius_};
	ImGui::Text("Current Position: %.1f, %.1f, %.1f", currentPos.x, currentPos.y, currentPos.z);
	ImGui::Text("Timer: %.2f", trailLoopTimer_);
}

void DebugScene::DrawObjectTestImGui() {
	ImGui::Checkbox("Draw Test Objects", &drawTestObjects_);
	ImGui::Checkbox("Draw Level Objects", &drawLevelObjects_);

	ImGui::Separator();
	ImGui::Text("Test Ball Transform");
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -180.0f, 180.0f);
	ImGui::SliderFloat3("Translate", &transform.translate.x, -50.0f, 50.0f);

	ImGui::Separator();
	ImGui::Text("Level Data");
	if (!levelDataLoader_) {
		ImGui::TextDisabled("LevelDataLoader is not available.");
		return;
	}

	ImGui::Text("Loaded: %s", levelDataLoader_->IsLoaded() ? "Yes" : "No");
	if (levelDataLoader_->IsLoaded()) {
		const auto &levelData = levelDataLoader_->GetLevelData();
		ImGui::Text("Scene Name: %s", levelData.name.c_str());
		ImGui::Text("Root Objects: %zu", levelData.objects.size());
		ImGui::Text("Created Object3D Count: %zu", levelObjects_.size());
	}

	if (ImGui::Button("Reload Level Data")) {
		levelObjects_.clear();
		bool reloadResult = levelDataLoader_->LoadLevelFromJson("resources/levels/test.json");
		if (reloadResult) {
			levelDataLoader_->CreateObjectsFromLevelData(object3dSetup_, levelObjects_);
		}
	}

	ImGui::Separator();
	levelDataLoader_->ImGuiDraw(levelObjects_);
}

void DebugScene::DrawVoiceTestImGui() {
	if (!voiceBridge_) {
		ImGui::TextDisabled("VoiceBridge is not available.");
		return;
	}

	ImGui::Text("Sample Rate: %u Hz | Channels: %u", voiceBridge_->GetSampleRate(), voiceBridge_->GetChannelCount());
	if (!voiceIsRecording_) {
		if (ImGui::Button("Start Recording", ImVec2(150, 30))) {
			if (voiceBridge_->Start()) {
				voiceIsRecording_ = true;
				voiceDisplaySamples_.clear();
				Logger::Log("Started microphone recording", Logger::LogLevel::Info);
			} else {
				Logger::Log("Failed to start microphone recording", Logger::LogLevel::Error);
			}
		}
	} else {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Recording");
		ImGui::SameLine();
		if (ImGui::Button("Stop Recording", ImVec2(150, 30))) {
			voiceBridge_->Stop();
			voiceIsRecording_ = false;
			Logger::Log("Stopped microphone recording", Logger::LogLevel::Info);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Clear Buffer", ImVec2(120, 30))) {
		voiceDisplaySamples_.clear();
		voiceBridge_->ClearSamples();
	}

	auto stats = voiceBridge_->GetVolumeStats();
	ImGui::Separator();
	ImGui::Text("RMS: %.4f / %.2f dB", stats.currentRMS, stats.currentRMSDB);
	ImGui::ProgressBar(stats.currentRMS, ImVec2(-1, 18.0f), "");
	ImGui::Text("Smoothed: %.4f / %.2f dB", stats.smoothedRMS, stats.smoothedRMSDB);
	ImGui::ProgressBar(stats.smoothedRMS, ImVec2(-1, 18.0f), "");
	ImGui::Text("Voice Score: %.2f", stats.voiceScore);
	ImGui::ProgressBar(stats.voiceScore, ImVec2(-1, 18.0f), stats.isVoiceDetected ? "Voice" : "No Voice");

	ImGui::Separator();
	static float smoothingFactor = 0.4f;
	if (ImGui::SliderFloat("Smoothing Factor", &smoothingFactor, 0.0f, 1.0f, "%.3f")) {
		voiceBridge_->SetSmoothingFactor(smoothingFactor);
	}
	static float noiseFloor = -50.0f;
	if (ImGui::SliderFloat("Noise Floor", &noiseFloor, -80.0f, -20.0f, "%.1f dB")) {
		voiceBridge_->SetNoiseFloor(noiseFloor);
	}
	static float zcThreshold = 0.25f;
	static float volumeThreshold = -40.0f;
	ImGui::SliderFloat("ZC Rate Threshold", &zcThreshold, 0.05f, 0.5f, "%.3f");
	ImGui::SliderFloat("Volume Threshold", &volumeThreshold, -80.0f, -10.0f, "%.1f dB");
	if (ImGui::Button("Apply Thresholds")) {
		voiceBridge_->SetVoiceDetectionThresholds(zcThreshold, volumeThreshold);
	}

	ImGui::Separator();
	ImGui::SliderFloat("Waveform Scale", &voiceWaveformScale_, 1.0f, 500.0f, "%.0f");
	ImGui::SliderFloat("Waveform Sensitivity", &voiceSensitivity_, 0.1f, 5.0f, "%.2f");

	ImDrawList *waveDrawList = ImGui::GetWindowDrawList();
	ImVec2 waveCanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 waveCanvasSize(ImGui::GetContentRegionAvail().x, 140.0f);
	waveDrawList->AddRectFilled(
		waveCanvasPos,
		ImVec2(waveCanvasPos.x + waveCanvasSize.x, waveCanvasPos.y + waveCanvasSize.y),
		ImGui::GetColorU32(ImVec4(0.05f, 0.05f, 0.1f, 1.0f)));
	waveDrawList->AddRect(
		waveCanvasPos,
		ImVec2(waveCanvasPos.x + waveCanvasSize.x, waveCanvasPos.y + waveCanvasSize.y),
		ImGui::GetColorU32(ImGuiCol_Border));

	if (voiceDisplaySamples_.size() > 1) {
		float centerY = waveCanvasPos.y + waveCanvasSize.y * 0.5f;
		waveDrawList->AddLine(
			ImVec2(waveCanvasPos.x, centerY),
			ImVec2(waveCanvasPos.x + waveCanvasSize.x, centerY),
			ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.3f, 0.5f)),
			1.0f);

		size_t displaySamples = std::min(voiceDisplaySamples_.size(), size_t(waveCanvasSize.x * 2.0f));
		size_t skipSamples = std::max(size_t(1), voiceDisplaySamples_.size() / displaySamples);
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
		ImGui::SetCursorScreenPos(ImVec2(waveCanvasPos.x + 10.0f, waveCanvasPos.y + 60.0f));
		ImGui::TextDisabled("Waiting for audio data...");
	}
	ImGui::Dummy(waveCanvasSize);
}


///=============================================================================
///						Editor Panel登録
void DebugScene::RegisterEditorPanels() {
	// NOTE: Scene所有リソースを参照するため、Scene切替時にEditorUiSystem側でScene Panelを破棄する。
	engineContext_->editorUiSystem->RegisterPanel("Debug Scene", MagEngine::EditorUiCategory::Scene, true, [this]() {
		DrawDebugUi();
	});
}

///=============================================================================
///						DebugScene Debug UI
void DebugScene::DrawDebugUi() {
	if (ImGui::BeginTabBar("DebugSceneTabs")) {
		if (ImGui::BeginTabItem("Overview")) {
			DrawOverviewImGui();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Objects")) {
			DrawObjectTestImGui();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Cloud")) {
			DrawCloudTestImGui();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Trail")) {
			DrawTrailTestImGui();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Voice")) {
			DrawVoiceTestImGui();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	engineContext_->debugTextManager->AddText3D("Hello, DebugScene!", Vector3{0.0f, 0.0f, 0.0f}, Vector4{1.0f, 1.0f, 1.0f, 1.0f}, -1.0f, 1.0f);
}

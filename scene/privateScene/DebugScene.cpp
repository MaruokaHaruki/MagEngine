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
	//モデルの読み込み
	ModelManager::GetInstance()->LoadMedel("axisPlus.obj");
	ModelManager::GetInstance()->LoadMedel("ball.obj");
	ModelManager::GetInstance()->LoadMedel("terrain.obj");
	//========================================
	// 3Dオブジェクトクラス
	//モンスターボール
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
	particle_->CreateParticleGroup("Test", "circle2.png");
	//========================================
	// エミッターの作成
	particleEmitter_ =
		std::make_unique<ParticleEmitter>(particle_.get(),
			"Test",
			Transform{ {0.2f,0.2f,0.2f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} },
			100,
			0.1f,
			true);
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
	objMonsterBall_->SetScale(Vector3{ transform.scale.x,transform.scale.y,transform.scale.z });
	objMonsterBall_->SetRotation(Vector3{ transform.rotate.x,transform.rotate.y,transform.rotate.z });
	objMonsterBall_->SetPosition(Vector3{ transform.translate.x,transform.translate.y,transform.translate.z });
	objMonsterBall_->Update();
	// 地面
	objTerrain_->SetScale(Vector3{ 1.0f,1.0f,1.0f });
	objTerrain_->SetRotation(Vector3{ 0.0f,0.0f,0.0f });
	objTerrain_->SetPosition(Vector3{ 0.0f,0.0f,0.0f });
	objTerrain_->Update();

	//========================================
	// パーティクル系
	// パーティクルの更新
	particle_->Update();
	// エミッターの更新
	particleEmitter_->Update();

	//========================================
	// 音声の再生
	if(audio_->IsWavPlaying("Duke_Ellington.wav") == false) {
		//audio_->PlayWavReverse("Duke_Ellington.wav", true, 1.0f, 1.0f);
		//audio_->PlayWav("Duke_Ellington.wav", true, 1.0f, 1.0f);
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
	//objMonsterBall_->Draw();
	// 地面
	//objTerrain_->Draw();
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
	//DebugSceneのImGui描画
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, DebugScene!");
	ImGui::End();


	//========================================
	// 3DオブジェクトのImGui描画
	//ライトの設定
	ImGui::Begin("3DObject");
	ImGui::Text("TransformSetting");
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -180.0f, 180.0f);
	ImGui::SliderFloat3("Translate", &transform.translate.x, -10.0f, 10.0f);
	ImGui::Separator();
	ImGui::End();
}

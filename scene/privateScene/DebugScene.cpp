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
	////========================================
	//// テクスチャマネージャ

	//========================================
	// スプライトクラス(Game)


	///--------------------------------------------------------------
	///						 3D系クラス
	//モデルの読み込み
	ModelManager::GetInstance()->LoadMedel("axisPlus.obj");
	ModelManager::GetInstance()->LoadMedel("ball.obj");
	//========================================
	// 3Dオブジェクトクラス
	objMonsterBall_ = std::make_unique<Object3d>();
	//3Dオブジェクトの初期化
	objMonsterBall_->Initialize(object3dSetup);
	objMonsterBall_->SetModel("ball.obj");

	//========================================
	// ライト情報の取得
	lightColor = objMonsterBall_->GetDirectionalLight().color;
	lightDirection = objMonsterBall_->GetDirectionalLight().direction;
	lightIntensity = objMonsterBall_->GetDirectionalLight().intensity;

	///--------------------------------------------------------------
	///						 パーティクル系
	//========================================
	// パーティクルクラス
	
	//========================================
	// パーティクルエミッター
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
	//CameraManager::GetInstance()->UpdateAll();

	//========================================
	// 2D更新


	//========================================
	// 3D更新 
	//大きさのセット
	objMonsterBall_->SetScale(Vector3{ transform.scale.x,transform.scale.y,transform.scale.z });
	//回転のセット
	objMonsterBall_->SetRotation(Vector3{ transform.rotate.x,transform.rotate.y,transform.rotate.z });
	//座標のセット
	objMonsterBall_->SetPosition(Vector3{ transform.translate.x,transform.translate.y,transform.translate.z });
	//更新
	objMonsterBall_->Update();

	//========================================
	// パーティクル系


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
	//========================================
	// 3D描画
	objMonsterBall_->Draw();
}

///=============================================================================
///						パーティクル描画
void DebugScene::ParticleDraw() {

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
	//大きさ
	ImGui::SliderFloat3("Scale", &transform.scale.x, 0.1f, 10.0f);
	//回転
	ImGui::SliderFloat3("Rotate", &transform.rotate.x, -180.0f, 180.0f);
	//座標
	ImGui::SliderFloat3("Translate", &transform.translate.x, -10.0f, 10.0f);
	//セパレート
	ImGui::Separator();
	//ライトの設定
	ImGui::Text("LightSetting");
	//ライトの色
	ImGui::ColorEdit4("LightColor", &lightColor.x);
	//ライトの方向
	ImGui::SliderFloat3("LightDirection", &lightDirection.x, -1.0f, 1.0f);
	//ライトの強度
	ImGui::SliderFloat("LightIntensity", &lightIntensity, 0.2f, 100.0f);
	//ライトの設定
	objMonsterBall_->SetDirectionalLight(lightColor, lightDirection, lightIntensity);
	//光沢度の設定
	ImGui::SliderFloat("Shininess", &lightIntensity, 1.0f, 100.0f);
	objMonsterBall_->SetShininess(lightIntensity);
	ImGui::End();

}

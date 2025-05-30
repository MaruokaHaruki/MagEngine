/*********************************************************************
 * \file   GamePlayScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "GamePlayScene.h"
#include "CameraManager.h"
#include "Input.h" // Inputクラスを使用するためにインクルード
#include "ModelManager.h"
#include "Player.h"

///=============================================================================
///						初期化
void GamePlayScene::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup) {
	// 適当に引数を使用
	// 引数を使用しない場合は警告を出さないようにする
	spriteSetup;
	object3dSetup;
	particleSetup;
	//========================================
	// カメラ設定
	CameraManager::GetInstance()->GetCamera("DefaultCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.3f, -8.0f}});

	//========================================
	// テクスチャの読み込み

	//========================================
	// モデルの読み込み
	ModelManager::GetInstance()->LoadMedel("jet.obj"); // モデルは事前にロードしておく

	//========================================
	// スプライトクラス(Game)

	//========================================
	// 地面

	//========================================
	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dSetup, "jet.obj"); // PlayerにObject3dSetupとモデル名を渡す

	//========================================
	// 敵

	//========================================
	// パーティクルクラス
	particle_ = std::make_unique<Particle>();
	// パーティクルの初期化
	particle_->Initialize(particleSetup);
	particle_->SetCustomTextureSize({10.0f, 10.0f});
	// パーティクルグループの作成
	particle_->CreateParticleGroup("Particle", "sandWind.png", ParticleShape::Board);
	// パーティクルエミッター
	particleEmitter_ =
		std::make_unique<ParticleEmitter>(particle_.get(),
										  "Particle",
										  Transform{{0.1f, 0.1f, 0.1f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  0.5f,
										  true);

	//========================================
	// 当たり判定
	collisionManager_ = std::make_unique<CollisionManager>();
	collisionManager_->Initialize();
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	// 仮のdeltaTime (実際のゲームループから取得するのが望ましい)
	const float deltaTime = 1.0f / 60.0f;

	Input *input = Input::GetInstance();

	//========================================
	// プレイヤー
	if (player_) {
		bool pressW = input->PushKey(DIK_W);
		bool pressS = input->PushKey(DIK_S);
		bool pressA = input->PushKey(DIK_A);
		bool pressD = input->PushKey(DIK_D);
		player_->Update(deltaTime, pressW, pressS, pressA, pressD);
	}
}

///=============================================================================
///                        スプライト描画
void GamePlayScene::Object2DDraw() {
}

///=============================================================================
///                        3D描画
void GamePlayScene::Object3DDraw() {
	//========================================
	// 地面

	//========================================
	// プレイヤー
	if (player_) {
		player_->Draw();
	}

	//========================================
	// 複数の敵

	//========================================
	// 当たり判定
	collisionManager_->Draw();
}

///=============================================================================
///
void GamePlayScene::ParticleDraw() {
	//========================================
	// パーティクル描画
}

///=============================================================================
///						ImGui描画
void GamePlayScene::ImGuiDraw() {
#ifdef _DEBUG
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, GamePlayScene!");
	ImGui::End();
	//========================================
	// プレイヤー
	if (player_) {
		player_->DrawImGui();
	}

	//========================================
	// 敵

	//========================================
	// 当たり判定
	collisionManager_->DrawImGui();
#endif // _DEBUG
}

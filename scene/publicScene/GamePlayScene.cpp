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
#include "Cloud.h"
#include "Enemy.h"
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "HUD.h"
#include "Input.h"
#include "LineManager.h"
#include "ModelManager.h"
#include "Player.h"

///=============================================================================
///						初期化
void GamePlayScene::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup, SkyboxSetup *skyboxSetup) {
	// 適当に引数を使用
	// 引数を使用しない場合は警告を出さないようにする
	spriteSetup;
	object3dSetup;
	particleSetup;
	//========================================
	// カメラ設定
	CameraManager::GetInstance()->AddCamera("FollowCamera");
	CameraManager::GetInstance()->GetCamera("DefaultCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.3f, -8.0f}});

	//========================================
	// FollowCameraの初期化
	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize("FollowCamera");

	//========================================
	// DebugTextManagerにカメラを設定
	DebugTextManager::GetInstance()->SetCamera(CameraManager::GetInstance()->GetCamera("FollowCamera"));

	//=======================================
	// テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture("rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("qwantani_dusk_2_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("overcast_soil_puresky_4k.dds");
	TextureManager::GetInstance()->LoadTexture("moonless_golf_4k.dds");
	TextureManager::GetInstance()->LoadTexture("kloppenheim_02_puresky_4k.dds");

	//========================================
	// モデルの読み込み
	ModelManager::GetInstance()->LoadModel("jet.obj");		// モデルは事前にロードしておく
	ModelManager::GetInstance()->LoadModel("axisPlus.obj"); // 弾のモデル
	ModelManager::GetInstance()->LoadModel("ground.obj");	// 地形のモデル
	ModelManager::GetInstance()->LoadModel("skydome.obj");	// 地面のモデルもロード
	// モデルの環境マップ設定
	ModelManager::GetInstance()->GetModelSetup()->SetEnvironmentTexture("overcast_soil_puresky_4k.dds");

	//========================================
	// スプライトクラス(Game)

	//========================================
	// スカイドーム
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(object3dSetup, "skydome.obj");

	//========================================
	// スカイボックス
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(skyboxSetup);
	// Skyboxのモデルを設定
	skybox_->SetTexture("overcast_soil_puresky_4k.dds");
	// SkyboxのTransformを設定
	skybox_->SetTransform({{1000.0f, 1000.0f, 1000.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});

	//========================================
	// パーティクルクラス
	particle_ = std::make_unique<Particle>();
	// パーティクルの初期化
	particle_->Initialize(particleSetup);
	particle_->SetCustomTextureSize({10.0f, 10.0f});
	particle_->SetBillboard(true); // ビルボードを有効化
	// 雲パーティクルグループの作成（Board形状、白っぽいテクスチャ）
	particle_->CreateParticleGroup("CloudParticles", "sandWind.png", ParticleShape::Board);
	// 爆発エフェクト用の複数の形状を作成
	// 1. メインの爆発エフェクト（Board形状 - 火花）
	particle_->CreateParticleGroup("ExplosionSparks", "sandWind.png", ParticleShape::Board);
	// 2. リング形状の衝撃波
	particle_->CreateParticleGroup("ExplosionRing", "sandWind.png", ParticleShape::Ring);
	// 3. シリンダー形状の煙柱
	particle_->CreateParticleGroup("ExplosionSmoke", "sandWind.png", ParticleShape::Cylinder);

	//========================================
	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dSetup, "jet.obj");
	// プレイヤーにパーティクルシステムを設定
	player_->SetParticleSystem(particle_.get(), particleSetup);
	// FollowCameraにプレイヤーを設定
	followCamera_->SetTarget(player_.get());
	// FollowCameraをメインカメラに設定
	CameraManager::GetInstance()->SetCurrentCamera("FollowCamera");

	//========================================
	// 雲システムの初期化

	//========================================
	// 敵マネージャー
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(object3dSetup, particle_.get(), particleSetup);
	// プレイヤー参照を設定
	enemyManager_->SetPlayer(player_.get());

	// プレイヤーにEnemyManagerを設定（ミサイル用）
	player_->SetEnemyManager(enemyManager_.get());

	//========================================
	// 当たり判定（軽量システムで初期化）
	collisionManager_ = std::make_unique<CollisionManager>();
	collisionManager_->Initialize(32.0f, 256); // セルサイズ32.0f、最大256オブジェクト

	//========================================
	// 敵の位置にデバッグテキストを配置（固定位置）
	DebugTextManager::GetInstance()->AddText3D("Enemy", {5.0f, 1.0f, 5.0f}, {1.0f, 0.0f, 0.0f, 1.0f});

	//========================================
	// HUDの初期化
	hud_ = std::make_unique<HUD>();
	hud_->Initialize();

	// HUDにFollowCameraを設定（FollowCameraが初期化された後）
	if (followCamera_) {
		hud_->SetFollowCamera(followCamera_.get());
	}

	//========================================
	// トランジションの初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize(spriteSetup);
	sceneTransition_->SetColor({0.0f, 0.0f, 0.0f, 1.0f}); // 黒

	// シーン開始時にオープニングトランジション
	sceneTransition_->StartOpening(TransitionType::Curtain, 1.5f);
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	//========================================
	// トランジションの更新
	if (sceneTransition_) {
		sceneTransition_->Update();
	}

	//========================================
	// FollowCameraの更新
	if (followCamera_) {
		followCamera_->Update();
	}

	//========================================
	// プレイヤー
	if (player_) {
		player_->Update();

		// プレイヤーの位置にデバッグテキストを配置
		Vector3 playerPos = player_->GetPosition();
		playerPos.y += 2.0f; // プレイヤーの少し上に表示
		DebugTextManager::GetInstance()->AddText3D("Player", playerPos, {0.0f, 1.0f, 0.0f, 1.0f});
	}

	//========================================
	// 敵の更新
	if (enemyManager_) {
		enemyManager_->Update();
	}

	//========================================
	// パーティクルの更新
	if (particle_) {
		particle_->Update();
	}

	//========================================
	// スカイドーム
	if (skydome_) {
		skydome_->Update();
	}

	//=========================================
	// Skyboxの更新
	if (skybox_) {
		skybox_->Update();
	}

	//=========================================
	//  当たり判定（最適化済み）
	//  リセットではなく登録解除/登録で管理
	collisionManager_->Reset(); // 一旦リセット（簡単のため）
	//  プレイヤーの当たり判定を登録
	if (player_) {
		collisionManager_->RegisterObject(player_.get());
	}
	//  敵の当たり判定を登録
	if (enemyManager_) {
		enemyManager_->RegisterCollisions(collisionManager_.get());
	}
	//  プレイヤーの弾とミサイルの当たり判定を登録
	if (player_) {
		const auto &bullets = player_->GetBullets();
		for (const auto &bullet : bullets) {
			if (bullet->IsAlive()) {
				collisionManager_->RegisterObject(bullet.get());
			}
		}

		const auto &missiles = player_->GetMissiles();
		for (const auto &missile : missiles) {
			if (missile->IsAlive()) {
				collisionManager_->RegisterObject(missile.get());
			}
		}
	}
	//  当たり判定の更新
	collisionManager_->Update();

	//========================================
	// HUDの更新
	if (hud_ && player_) {
		hud_->Update(player_.get());
	}

	//========================================
	// LineManagerの更新（ミサイルデバッグ表示用）
	LineManager::GetInstance()->Update();
}

///=============================================================================
///                        スプライト描画
void GamePlayScene::Object2DDraw() {
	//========================================
	// トランジションの描画
	if (sceneTransition_) {
		sceneTransition_->Draw();
	}
}

///=============================================================================
///                        3D描画
void GamePlayScene::Object3DDraw() {

	//=========================================
	// スカイドーム
	if (skydome_) {
		// skydome_->Draw();
	}

	//========================================
	// プレイヤー
	if (player_) {
		player_->Draw();
		// プレイヤーの弾とミサイルも描画
		player_->DrawBullets();
		player_->DrawMissiles();
	}

	//========================================
	// 敵マネージャー
	if (enemyManager_) {
		enemyManager_->Draw();
	}

	//========================================
	// LineManagerの描画（デバッグライン）
	LineManager::GetInstance()->Draw();

	//========================================
	// 当たり判定
	collisionManager_->Draw();

	//========================================
	// 雲システムの描画

	//========================================
	// HUDの描画
	if (hud_) {
		hud_->Draw();
	}
}

///=============================================================================
///
void GamePlayScene::ParticleDraw() {
	//========================================
	// パーティクル描画
	if (particle_) {
		particle_->Draw();
	}

	//========================================
	// 雲システムの描画
}

///=============================================================================
///						Skybox描画
void GamePlayScene::SkyboxDraw() {
	//=========================================
	// Skyboxの描画
	if (skybox_) {
		skybox_->Draw();
	}
}

///=============================================================================
///						ImGui描画
void GamePlayScene::ImGuiDraw() {
#ifdef _DEBUG
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, GamePlayScene!");

	// FollowCameraの制御
	if (followCamera_) {
		followCamera_->DrawImGui();
	}
	// 雲システムの制御

	ImGui::Separator();
	ImGui::End();

	//========================================
	// プレイヤー
	if (player_) {
		player_->DrawImGui();

		// ミサイルのImGui表示
		const auto &missiles = player_->GetMissiles();
		for (size_t i = 0; i < missiles.size(); ++i) {
			if (missiles[i] && missiles[i]->IsAlive()) {
				missiles[i]->DrawImGui();
			}
		}
	}

	//========================================
	// 敵マネージャー
	if (enemyManager_) {
		enemyManager_->DrawImGui();
	}

	//========================================
	// 当たり判定
	collisionManager_->DrawImGui();

	//========================================
	// HUD
	if (hud_) {
		hud_->DrawImGui();
	}

	//========================================
	// LineManagerのImGui
	LineManager::GetInstance()->DrawImGui();
#endif // _DEBUG
}

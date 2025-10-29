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
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "HUD.h"
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
	CameraManager::GetInstance()->GetCamera("FollowCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.3f, -8.0f}});

	//========================================
	// FollowCameraの初期化
	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize("FollowCamera");

	//========================================
	// DebugTextManagerにカメラを設定
	DebugTextManager::GetInstance()->SetCamera(CameraManager::GetInstance()->GetCamera("FollowCamera"));

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
	// ゲームオーバーUIの初期化
	gameOverUI_ = std::make_unique<GameOverUI>();
	gameOverUI_->Initialize(spriteSetup);
	gameOverUI_->SetTextTexture("WolfOne_GameOver.png"); // ゲームオーバー画像
	gameOverUI_->SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
	gameOverUI_->SetTextSize({1000.0f, 200.0f}); // より大きなサイズに設定
	isGameOver_ = false;

	// ゲームオーバー完了後のコールバック設定
	gameOverUI_->SetOnCompleteCallback([this]() {
		// タイトルへ戻る等の処理
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::TITLE;
			});
		}
	});

	//========================================
	// トランジションの初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize(spriteSetup);
	sceneTransition_->SetColor({0.0f, 0.0f, 0.0f, 1.0f}); // 黒

	//========================================
	// スタートアニメーションの初期化
	startAnimation_ = std::make_unique<StartAnimation>();
	startAnimation_->SetTextTexture("WolfOne_Engage.png"); // 仮のテキスト画像
	startAnimation_->Initialize(spriteSetup);
	startAnimation_->SetBarColor({0.0f, 0.0f, 0.0f, 1.0f}); // 黒
	startAnimation_->SetBarHeightRatio(0.15f);				// 画面の15%
	startAnimation_->SetTextSize({600.0f, 100.0f});

	// シーン開始時にスタートアニメーション（シネマスコープ演出）
	startAnimation_->StartOpening(2.0f, 1.0f, 1.0f);

	// HUDの展開はスタートアニメーション完了後に開始
	startAnimation_->SetOnCompleteCallback([this]() {
		if (hud_) {
			hud_->StartDeployAnimation(2.0f);
		}
	});

	// トランジションは使用しない（スタートアニメーションで代用）
	sceneTransition_->StartOpening(TransitionType::ZoomIn, 1.5f);
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	//========================================
	// スタートアニメーションの更新
	if (startAnimation_) {
		startAnimation_->Update();
	}

	//========================================
	// ゲームオーバーチェック
	if (player_ && !isGameOver_) {
		// プレイヤーの墜落が完了したらゲームオーバー演出開始
		if (player_->IsCrashComplete()) {
			isGameOver_ = true;
			if (gameOverUI_) {
				gameOverUI_->StartGameOver(2.0f, 3.0f);
			}
			// HUDを格納
			if (hud_ && !hud_->IsAnimating()) {
				hud_->StartRetractAnimation(1.0f);
			}
		}
	}

	//========================================
	// ゲームオーバーUI更新
	if (gameOverUI_) {
		gameOverUI_->Update();
	}

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

	// ゲームオーバー中は以降の更新をスキップ
	if (isGameOver_) {
		//========================================
		// パーティクルの更新（墜落エフェクト用）
		if (particle_) {
			particle_->Update();
		}

		//========================================
		// HUDの更新
		if (hud_ && player_) {
			hud_->Update(player_.get());
		}
		return;
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
	// タイトルへのシーン遷移（デバッグ用）
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		// トランジション開始（スタートアニメーションの逆再生も実行）
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::TITLE;
			});
		}
	}
}

///=============================================================================
///                        スプライト描画
void GamePlayScene::Object2DDraw() {
	//========================================
	// ゲームオーバーUI（最前面）
	if (gameOverUI_) {
		gameOverUI_->Draw();
	}

	//========================================
	// スタートアニメーションの描画（最前面）
	if (startAnimation_) {
		startAnimation_->Draw();
	}

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
	// 当たり判定
	collisionManager_->Draw();

	//========================================
	// 雲システムの描画

	//========================================
	// HUDの描画
	if (hud_) {
		// トランジション完了後にHUDを表示開始
		if (!sceneTransition_ || !sceneTransition_->IsTransitioning()) {
			hud_->Draw();
		}
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
	// スタートアニメーション
	if (startAnimation_) {
		startAnimation_->DrawImGui();
	}

	//========================================
	// トランジション
	if (sceneTransition_) {
		sceneTransition_->DrawImGui();
	}

	//========================================
	// ゲームオーバーUI
	if (gameOverUI_) {
		gameOverUI_->DrawImGui();
	}
#endif // _DEBUG
}

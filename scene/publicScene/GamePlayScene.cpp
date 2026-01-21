/*********************************************************************
 * \file   GamePlayScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "GamePlayScene.h"
//========================================
// Game
#include "CameraManager.h"
#include "CollisionManager.h"
#include "DebugTextManager.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "ModelManager.h"
#include "Player.h"
#include "SceneTransition.h"
#include "Skydome.h"
using namespace MagEngine;

///=============================================================================
///						初期化
void GamePlayScene::Initialize(MagEngine::SpriteSetup *spriteSetup,
							   MagEngine::Object3dSetup *object3dSetup,
							   MagEngine::ParticleSetup *particleSetup,
							   MagEngine::SkyboxSetup *skyboxSetup,
							   MagEngine::CloudSetup *cloudSetup) {
	//========================================
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
	particle_ = std::make_unique<MagEngine::Particle>();
	// パーティクルの初期化
	particle_->Initialize(particleSetup);
	particle_->SetCustomTextureSize({10.0f, 10.0f});
	particle_->SetBillboard(true); // ビルボードを有効化
	// 雲パーティクルグループの作成（Board形状、白っぽいテクスチャ）
	particle_->CreateParticleGroup("CloudParticles", "circle2.png", ParticleShape::Board);
	// 爆発エフェクト用の複数の形状を作成
	// 1. メインの爆発エフェクト（Board形状 - 火花）
	particle_->CreateParticleGroup("ExplosionSparks", "circle2.png", ParticleShape::Board);
	// 2. リング形状の衝撃波（ヒットリアクション用にも使用）
	particle_->CreateParticleGroup("ExplosionRing", "circle2.png", ParticleShape::Ring);
	// 3. シリンダー形状の煙柱
	particle_->CreateParticleGroup("ExplosionSmoke", "circle2.png", ParticleShape::Cylinder);

	//========================================
	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dSetup, "jet.obj");
	// FollowCameraにプレイヤーを設定
	followCamera_->SetTarget(player_.get());
	// FollowCameraをメインカメラに設定
	CameraManager::GetInstance()->SetCurrentCamera("FollowCamera");

	//========================================
	// 雲
	cloud_ = std::make_unique<Cloud>();
	cloud_->Initialize(cloudSetup);

	// 雲のサイズ設定（広い範囲に配置）
	cloud_->SetSize({500.0f, 100.0f, 500.0f});
	cloud_->SetEnabled(true);

	// 雲のTransform設定
	cloud_->GetTransform().translate = {0.0f, -50.0f, 250.0f};

	// 雲の密度と速度を調整（まばらな雲に）
	auto &cloudParams = cloud_->GetMutableParams();
	// 密度：雲の濃さ（値を下げてより透明に）
	cloudParams.density = 1.2f; // かなり薄い雲（以前: 1.8f）
	// カバレッジ：雲の分布（値を下げてよりまばらに）
	cloudParams.coverage = 0.40f; // 25%の領域に雲が存在（以前: 0.45f）
	// ノイズ速度：雲の流れる速さ（ゆっくりとした動き）
	cloudParams.noiseSpeed = 4.5f; // 少し遅めの流れ（以前: 0.4f）
	// 環境光：雲の明るさ（値を上げて明るく）
	cloudParams.ambient = 0.6f; // 明るめの雲（以前: 0.5f）
	// 太陽光強度：太陽光による照明の強さ
	cloudParams.sunIntensity = 1.2f; // 柔らかい光（以前: 1.5f）
	// ベースノイズスケール：大きな雲の形状
	cloudParams.baseNoiseScale = 0.007f; // より大きな雲の塊
	// ディテールウェイト：細かいディテールの影響度
	cloudParams.detailWeight = 0.2f; // なめらかな雲の表面

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
	// UI管理の初期化
	uiManager_ = std::make_unique<UIManager>();
	uiManager_->Initialize(spriteSetup, object3dSetup);

	// HUDにFollowCameraを設定
	if (followCamera_ && uiManager_->GetHUD()) {
		uiManager_->GetHUD()->SetFollowCamera(followCamera_.get());
	}

	// ゲームオーバーUI の設定
	if (auto gameOverUI = uiManager_->GetGameOverUI()) {
		gameOverUI->SetTextTexture("WolfOne_GameOver.png");
		gameOverUI->SetBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f});
		gameOverUI->SetTextSize({1000.0f, 200.0f});
		gameOverUI->SetOnCompleteCallback([this]() {
			if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
				sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
				sceneTransition_->SetOnCompleteCallback([this]() {
					sceneNo = SCENE::TITLE;
				});
			}
		});
	}
	isGameOver_ = false;

	// ゲームクリアアニメーション の設定
	if (auto gameClearAnim = uiManager_->GetGameClearAnimation()) {
		gameClearAnim->SetFollowCamera(followCamera_.get());
		gameClearAnim->SetPlayer(player_.get());
		gameClearAnim->SetTextTexture("WolfOne_Comprete.png");
		gameClearAnim->SetBarColor({0.0f, 0.0f, 0.0f, 1.0f});
		gameClearAnim->SetBarHeightRatio(0.15f);
		gameClearAnim->SetTextSize({800.0f, 150.0f});
		gameClearAnim->SetCameraUpParameters(20.0f, -30.0f);
		gameClearAnim->SetFlightParameters(18.0f, 2.5f, 10.0f);
		gameClearAnim->SetOnCompleteCallback([this]() {
			if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
				sceneTransition_->StartClosing(TransitionType::Fade, 1.5f);
				sceneTransition_->SetOnCompleteCallback([this]() {
					sceneNo = SCENE::TITLE;
				});
			}
		});
	}
	isGameClear_ = false;

	//========================================
	// トランジションの初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize(spriteSetup);
	sceneTransition_->SetColor({0.0f, 0.0f, 0.0f, 1.0f});

	// スタートアニメーション の設定
	if (auto startAnim = uiManager_->GetStartAnimation()) {
		startAnim->SetTextTexture("WolfOne_Engage.png");
		startAnim->SetBarColor({0.0f, 0.0f, 0.0f, 1.0f});
		startAnim->SetBarHeightRatio(0.15f);
		startAnim->SetTextSize({600.0f, 100.0f});
		startAnim->StartOpening(2.0f, 1.0f, 1.0f);
		startAnim->SetOnCompleteCallback([this]() {
			if (auto hud = uiManager_->GetHUD()) {
				hud->StartDeployAnimation(2.0f);
			}
		});
	}

	// トランジション開始
	sceneTransition_->StartOpening(TransitionType::ZoomIn, 1.5f);

	// OperationGuideUI の設定
	if (auto operationGuideUI = uiManager_->GetOperationGuideUI()) {
		operationGuideUI->SetGuidePosition({50.0f, 370.0f});
		operationGuideUI->SetVisible(true);
	}
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	//========================================
	// 雲の更新
	if (cloud_) {
		cloud_->Update(*CameraManager::GetInstance()->GetCurrentCamera(), 1.0f / 60.0f);
	}

	//========================================
	// UI系の更新
	if (uiManager_) {
		uiManager_->Update(player_.get());
	}

	//========================================
	// ゲーム終了チェック（用語変更）
	if (player_ && !isGameOver_ && !isGameClear_) {
		// プレイヤーの敗北演出が完了したらゲーム終了演出開始
		if (player_->IsDefeatAnimationComplete()) { // IsCrashComplete から変更
			isGameOver_ = true;
			if (auto gameOverUI = uiManager_->GetGameOverUI()) {
				gameOverUI->StartGameOver(2.0f, 3.0f);
			}
			// HUDを格納
			if (auto hud = uiManager_->GetHUD()) {
				if (!hud->IsAnimating()) {
					hud->StartRetractAnimation(1.0f);
				}
			}
		}
	}

	//========================================
	// ゲームクリアチェック
	if (enemyManager_ && !isGameOver_ && !isGameClear_) {
		if (enemyManager_->IsGameClear()) {
			isGameClear_ = true;
			// クリア演出を開始
			if (auto gameClearAnim = uiManager_->GetGameClearAnimation()) {
				gameClearAnim->StartClearAnimation(1.0f, 2.0f, 3.0f, 1.0f);
			}
			// HUDを格納
			if (auto hud = uiManager_->GetHUD()) {
				if (!hud->IsAnimating()) {
					hud->StartRetractAnimation(1.0f);
				}
			}
		}
	}

#ifdef _DEBUG
	// デバック用にキーボードでゲームクリアを強制発動
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		isGameClear_ = true;
		// HUDを格納
		if (auto hud = uiManager_->GetHUD()) {
			hud->StartRetractAnimation(1.0f);
		}
		// クリア演出を開始
		if (auto gameClearAnim = uiManager_->GetGameClearAnimation()) {
			gameClearAnim->StartClearAnimation(1.0f, 2.0f, 3.0f, 1.0f);
		}
	}
#endif

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

	// ゲームオーバーまたはクリア中は以降の更新をスキップ
	if (isGameOver_ || isGameClear_) {
		//========================================
		// パーティクルの更新（墜落エフェクト用）
		if (particle_) {
			particle_->Update();
		}

		//========================================
		// ゲームオーバー/クリア中の処理スキップ
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
	//  敵の弾の当たり判定を登録
	if (enemyManager_) {
		auto enemyBullets = enemyManager_->GetAllEnemyBullets();
		for (auto *bullet : enemyBullets) {
			if (bullet) {
				collisionManager_->RegisterObject(bullet);
			}
		}
	}
	//  当たり判定の更新
	collisionManager_->Update();

#ifdef _DEBUG
	//========================================
	// タイトルへのシーン遷移（デバッグ用）
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		// トランジション開始
		if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
			sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
			sceneTransition_->SetOnCompleteCallback([this]() {
				sceneNo = SCENE::TITLE;
			});
		}
	}
#endif
}

///=============================================================================
///                        スプライト描画
void GamePlayScene::Object2DDraw() {
	//========================================
	// UI系の描画（UIManager で統一管理）
	if (uiManager_) {
		uiManager_->Draw();
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
	if (enemyManager_ && !isGameClear_) {
		enemyManager_->Draw();
	}

	//========================================
	// 当たり判定
	collisionManager_->Draw();

	//========================================
	// HUDはUIManager で描画管理
}

///=============================================================================
///
void GamePlayScene::ParticleDraw() {
	//========================================
	// パーティクル描画
	if (particle_) {
		particle_->Draw();
	}
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
///						Cloud描画
void GamePlayScene::CloudDraw() {
	//========================================
	// 雲の描画
	if (cloud_) {
		cloud_->Draw();
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
	// 雲
	if (cloud_) {
		cloud_->DrawImGui();
	}

	//========================================
	// 当たり判定
	collisionManager_->DrawImGui();

	//========================================
	// UI系（UIManager で統一管理）
	if (uiManager_) {
		uiManager_->DrawImGui();
	}

	//========================================
	// トランジション
	if (sceneTransition_) {
		sceneTransition_->DrawImGui();
	}
#endif // _DEBUG
}

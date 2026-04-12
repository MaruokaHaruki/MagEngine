/*********************************************************************
 * \file   GamePlayScene.cpp
 * \brief  ゲームプレイシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#include "GamePlayScene.h"
#include "SceneContext.h"
//========================================
// Game
#include "CameraManager.h"
#include "CollisionManager.h"
#include "DebugTextManager.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "FollowCamera.h"
#include "MenuUI.h"
#include "ModelManager.h"
#include "Player.h"
#include "SceneTransition.h"
#include "Skydome.h"
using namespace MagEngine;

///=============================================================================
/// 初期化
/// NOTE: contextからセットアップを取得
void GamePlayScene::Initialize(SceneContext *context) {
	//========================================
	// NOTE: contextがnullptrでないかチェック
	if (!context) {
		return;
	}

	// NOTE: contextからセットアップを取得
	MagEngine::SpriteSetup *spriteSetup = context->GetSpriteSetup();
	MagEngine::Object3dSetup *object3dSetup = context->GetObject3dSetup();
	MagEngine::ParticleSetup *particleSetup = context->GetParticleSetup();
	MagEngine::SkyboxSetup *skyboxSetup = context->GetSkyboxSetup();
	MagEngine::CloudSetup *cloudSetup = context->GetCloudSetup();

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
	particle_->CreateParticleGroup("CloudParticles", "circle2.dds", ParticleShape::Board);
	// 爆発エフェクト用の複数の形状を作成
	// 1. メインの爆発エフェクト（Board形状 - 火花）
	particle_->CreateParticleGroup("ExplosionSparks", "circle2.dds", ParticleShape::Board);
	// 2. リング形状の衝撃波（ヒットリアクション用にも使用）
	particle_->CreateParticleGroup("ExplosionRing", "circle2.dds", ParticleShape::Ring);
	// 3. シリンダー形状の煙柱
	particle_->CreateParticleGroup("ExplosionSmoke", "circle2.dds", ParticleShape::Cylinder);

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

	// 雲の密度と速度を調整（美しい表現）
	auto &cloudParams = cloud_->GetMutableParams();
	// 密度：雲の濃さ（自然な透け感）
	// NOTE : 1.5→2.2 雲の量を増加、ボリューム感UP
	cloudParams.density = 2.2f;
	// カバレッジ：雲の分布（豊かな分布）
	// NOTE : 0.35→0.20 分布範囲を拡大、もこもこ量増加
	cloudParams.coverage = 0.20f;
	// ノイズ速度：雲の流れる速さ（自然な流れ）
	cloudParams.noiseSpeed = 8.5f;
	// 環境光：雲の明るさ（明るく映える）
	// NOTE : 0.75→0.82 雲全体を明るく、量増加時の見映え向上
	cloudParams.ambient = 0.82f;
	// 太陽光強度：太陽光による照明の強さ（影がはっきり）
	cloudParams.sunIntensity = 1.6f;
	// ベースノイズスケール：大きな雲の形状（自然なサイズ）
	// NOTE : 0.0085→0.0070 より大きな塊のスケール、もこもこ感強調
	cloudParams.baseNoiseScale = 0.0070f;
	// ディテールウェイト：細かいディテールの影響度（より詳細に）
	// NOTE : 0.35→0.42 ディテール強調で表情豊かに
	cloudParams.detailWeight = 0.42f;

	//========================================
	// 敵マネージャー
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(object3dSetup, particle_.get(), particleSetup);
	// プレイヤー参照を設定
	enemyManager_->SetPlayer(player_.get());

	// プレイヤーにEnemyManagerを設定（ミサイル用）
	player_->SetEnemyManager(enemyManager_.get());

	// プレイヤーにTrailEffectManagerを設定（弾・ミサイルトレイル用）
	MagEngine::TrailEffectManager *trailEffectManager = context->GetTrailEffectManager();
	if (trailEffectManager) {
		player_->SetTrailEffectManager(trailEffectManager);
	}

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
		gameOverUI->SetTextTexture("WolfOne_GameOver.dds");
		gameOverUI->SetTextSize({1000.0f, 250.0f});
		gameOverUI->SetTextColor({1.0f, 0.2f, 0.2f, 1.0f}); // 鮮やかな赤
		gameOverUI->SetFadeBackgroundColor({0.0f, 0.0f, 0.0f, 0.7f}); // 濃い黒
		gameOverUI->SetOnComplete([this]() {
			if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
				sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
				sceneTransition_->SetOnCompleteCallback([this]() {
					SetSceneNo(SCENE::TITLE);
				});
			}
		});
	}
	isGameOver_ = false;

	// ゲームクリアアニメーション の設定
	if (auto gameClearAnim = uiManager_->GetGameClearAnimation()) {
		gameClearAnim->SetFollowCamera(followCamera_.get());
		gameClearAnim->SetPlayer(player_.get());
		gameClearAnim->SetTextTexture("WolfOne_Comprete.dds");
		gameClearAnim->SetBarColor({0.0f, 0.0f, 0.0f, 1.0f});
		gameClearAnim->SetBarHeightRatio(0.15f);
		gameClearAnim->SetTextSize({800.0f, 150.0f});
		gameClearAnim->SetCameraUpParameters(20.0f, -30.0f);
		gameClearAnim->SetFlightParameters(18.0f, 2.5f, 10.0f);
		gameClearAnim->SetOnCompleteCallback([this]() {
			if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
				sceneTransition_->StartClosing(TransitionType::Fade, 1.5f);
				sceneTransition_->SetOnCompleteCallback([this]() {
					SetSceneNo(SCENE::TITLE);
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
	}

	// トランジション開始
	sceneTransition_->StartOpening(TransitionType::ZoomIn, 1.5f);

	// OperationGuideUI の設定（初期状態は非表示）
	if (auto operationGuideUI = uiManager_->GetOperationGuideUI()) {
		operationGuideUI->SetGuidePosition({50.0f, 370.0f});
		operationGuideUI->SetVisible(false); // スタート演出終了後に表示
	}

	// LockOnHUD の設定
	if (auto lockOnHUD = uiManager_->GetLockOnHUD()) {
		lockOnHUD->Initialize(player_.get(), enemyManager_.get());
		lockOnHUD->SetVisible(true);
	}

	// UI展開開始フラグをリセット
	hasUIDeploymentStarted_ = false;
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
	// リソースの適切なクリーンアップ
	// unique_ptrは自動的に破棄されますが、明示的な終了処理が
	// 必要なコンポーネントがあれば追加します
	if (collisionManager_) {
		collisionManager_.reset();
	}
	if (particle_) {
		particle_.reset();
	}
	if (enemyManager_) {
		enemyManager_.reset();
	}
	if (player_) {
		player_.reset();
	}
	if (cloud_) {
		cloud_.reset();
	}
	if (skybox_) {
		skybox_.reset();
	}
	if (skydome_) {
		skydome_.reset();
	}
	if (followCamera_) {
		followCamera_.reset();
	}
	if (uiManager_) {
		uiManager_.reset();
	}
	if (sceneTransition_) {
		sceneTransition_.reset();
	}
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	//========================================
	// UI系の更新（メニュー状態確認用）
	if (uiManager_) {
		uiManager_->Update(player_.get());
	}

	//========================================
	// スタートアニメーション終了後の各種UI展開
	if (!hasUIDeploymentStarted_ && uiManager_ && uiManager_->GetStartAnimation()) {
		StartAnimation *startAnim = uiManager_->GetStartAnimation();
		if (startAnim->IsDone()) {
			hasUIDeploymentStarted_ = true;

			// HUDの展開を開始
			if (auto hud = uiManager_->GetHUD()) {
				hud->StartDeployAnimation(1.5f);
			}

			// OperationGuideUIの展開を開始
			if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
				operationGuide->SetVisible(true);
				operationGuide->StartDeployAnimation(1.0f);
			}
		}
	}

	//========================================
	// メニュー処理（ゲーム停止中でも処理）
	// メニューが開いている時だけボタン処理を行う
	if (uiManager_ && uiManager_->GetMenuUI() && uiManager_->GetMenuUI()->IsOpen()) {
		MenuUI *menuUI = uiManager_->GetMenuUI();
		if (menuUI->IsButtonPressed()) {
			MenuButton selectedButton = menuUI->GetSelectedButton();
			menuUI->ResetButtonPressedFlag();

			// ボタンに応じた処理
			if (selectedButton == MenuButton::ResumeGame) {
				// ゲームに戻る
				menuUI->Close();
			} else if (selectedButton == MenuButton::OperationGuide) {
				// 操作説明を表示（将来実装）
				// TODO: 操作説明を表示する処理を実装
			} else if (selectedButton == MenuButton::ReturnToTitle) {
				// メニューを閉じてからタイトルへ戻る
				menuUI->Close();
				// タイトルに戻る
				if (sceneTransition_ && !sceneTransition_->IsTransitioning()) {
					sceneTransition_->StartClosing(TransitionType::Fade, 1.0f);
					sceneTransition_->SetOnCompleteCallback([this]() {
						SetSceneNo(SCENE::TITLE);
					});
				}
			}
		}
	}

	//========================================
	// メニュー中はゲーム更新をスキップ
	if (uiManager_ && uiManager_->GetMenuUI() && uiManager_->GetMenuUI()->IsOpen()) {
		return;
	}

	//========================================
	// タイムスケール計算（ジャスト回避スロー効果）
	gameTimeScale_ = 1.0f; // デフォルト: 通常速度
	if (player_) {
		gameTimeScale_ = player_->GetJustAvoidanceComponent()->GetGameTimeScale();
	}
	const float baseDeltaTime = 1.0f / 60.0f;
	const float effectiveDeltaTime = baseDeltaTime * gameTimeScale_;

	//========================================
	// 雲の更新
	if (cloud_) {
		cloud_->Update(*CameraManager::GetInstance()->GetCurrentCamera(), effectiveDeltaTime);
	}

	//========================================
	// ゲーム終了チェック（用語変更）
	if (player_ && !isGameOver_ && !isGameClear_) {
		// プレイヤーの敗北演出が完了したらゲーム終了演出開始
		if (player_->IsDefeatAnimationComplete()) { // IsCrashComplete から変更
			isGameOver_ = true;
			// UIManagerにゲームオーバー状態を通知
			if (uiManager_) {
				uiManager_->SetGameOver(true);
			}
			if (auto gameOverUI = uiManager_->GetGameOverUI()) {
				gameOverUI->Play(0.8f, 2.5f, 1.2f);
			}
			// HUDを格納
			if (auto hud = uiManager_->GetHUD()) {
				if (!hud->IsAnimating()) {
					hud->StartRetractAnimation(1.0f);
				}
			}
			// 操作ガイドUIを格納
			if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
				operationGuide->StartRetractAnimation(0.6f);
			}
		}
	}

	//========================================
	// ゲームクリアチェック
	if (enemyManager_ && !isGameOver_ && !isGameClear_) {
		if (enemyManager_->IsGameClear()) {
			isGameClear_ = true;
			// UIManagerにゲームクリア状態を通知
			if (uiManager_) {
				uiManager_->SetGameClear(true);
			}
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
			// 操作ガイドUIを格納
			if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
				operationGuide->StartRetractAnimation(0.6f);
			}
		}
	}

	// デバック用にキーボードでゲームクリアを強制発動
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		isGameClear_ = true;
		// UIManagerにゲームクリア状態を通知
		if (uiManager_) {
			uiManager_->SetGameClear(true);
		}
		// HUDを格納
		if (auto hud = uiManager_->GetHUD()) {
			hud->StartRetractAnimation(1.0f);
		}
		// 操作ガイドUIを格納
		if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
			operationGuide->StartRetractAnimation(0.6f);
		}
		// クリア演出を開始
		if (auto gameClearAnim = uiManager_->GetGameClearAnimation()) {
			gameClearAnim->StartClearAnimation(1.0f, 2.0f, 3.0f, 1.0f);
		}
	}

	//========================================
	// 弾痕テスト用のデバッグコード
	// SPACEキーで雲に弾痕を追加
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->GetRightTrigger()) {
		if (player_) {
			// プレイヤーの位置から前方向に弾痕を作成
			Vector3 origin = player_->GetPosition();

			// プレイヤーのRotationから前方ベクトルを計算
			// Y軸回転（ヨー）から前方向を計算
			float yaw = player_->GetTransform()->rotate.y;
			Vector3 forward = {
				std::sin(yaw),
				0.0f,
				std::cos(yaw)};
			forward = MagMath::Normalize(forward);

			// 弾痕を追加（原点、方向、半径、残存時間）
			cloud_->AddBulletHole(origin - Vector3(0, 0, 100.0f), forward, 16.0f, 8.0f, 700.0f, 2.0f);

			// ログ出力
			Logger::Log("BulletHole added at player position", Logger::LogLevel::Info);
		}
	}

	// Nキーでランダムな位置に弾痕を追加
	if (Input::GetInstance()->TriggerKey(DIK_N) && cloud_) {
		// 雲の中心付近にランダムな弾痕を作成
		Vector3 cloudCenter = cloud_->GetTransform().translate;
		Vector3 randomOffset = {
			static_cast<float>((rand() % 200) - 100), // -100 ~ 100
			static_cast<float>((rand() % 40) - 20),	  // -20 ~ 20
			static_cast<float>((rand() % 200) - 100)  // -100 ~ 100
		};

		Vector3 origin = cloudCenter + randomOffset;

		// ランダムな方向
		Vector3 direction = {
			static_cast<float>((rand() % 200) - 100) / 100.0f, // -1.0 ~ 1.0
			static_cast<float>((rand() % 200) - 100) / 100.0f,
			static_cast<float>((rand() % 200) - 100) / 100.0f};
		direction = MagMath::Normalize(direction);

		// 弾痕を追加
		cloud_->AddBulletHole(origin, direction, 2.0f, 0.2f, 300.0f, 20.0f);

		Logger::Log("Random BulletHole added", Logger::LogLevel::Info);
	}

	// Mキーで全ての弾痕をクリア
	if (Input::GetInstance()->TriggerKey(DIK_M) && cloud_) {
		cloud_->ClearBulletHoles();
		Logger::Log("All bullet holes cleared", Logger::LogLevel::Info);
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
				SetSceneNo(SCENE::TITLE);
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
///						TrailEffect描画
void GamePlayScene::TrailEffectDraw() {
	// プレイヤーの弾のトレイル描画
	if (player_) {
		player_->DrawBulletsTrails();
		player_->DrawMissilesTrails();
	}
}

///=============================================================================
///						ImGui描画
void GamePlayScene::ImGuiDraw() {
#ifdef _DEBUG
	// DebugScene ウィンドウ
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("DebugScene");
		ImGui::Text("Hello, GamePlayScene!");

		// FollowCameraの制御
		if (followCamera_) {
			followCamera_->DrawImGui();
		}

		ImGui::Separator();
		ImGui::End();
	}

	//========================================
	// プレイヤー ウィンドウ
	{
		if (player_) {
			ImGui::SetNextWindowPos(ImVec2(400.0f, 0.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
			ImGui::Begin("Player Debug");
			player_->DrawImGui();

			// ミサイルのImGui表示
			if (ImGui::CollapsingHeader("Missiles")) {
				const auto &missiles = player_->GetMissiles();
				for (size_t i = 0; i < missiles.size(); ++i) {
					if (missiles[i] && missiles[i]->IsAlive()) {
						missiles[i]->DrawImGui();
					}
				}
			}
			ImGui::End();
		}
	}

	//========================================
	// 敵マネージャー ウィンドウ
	{
		if (enemyManager_) {
			ImGui::SetNextWindowPos(ImVec2(800.0f, 0.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
			ImGui::Begin("Enemy Manager Debug");
			enemyManager_->DrawImGui();
			ImGui::End();
		}
	}

	//========================================
	// 雲 ウィンドウ
	{
		if (cloud_) {
			ImGui::SetNextWindowPos(ImVec2(0.0f, 300.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
			ImGui::Begin("Cloud Debug");
#if ENABLE_IMGUI
#ifdef _DEBUG
			cloud_->DrawImGui();
#endif // _DEBUG
#endif // ENABLE_IMGUI

			// 弾痕テスト用のImGui
			ImGui::Separator();
			ImGui::Text("Bullet Hole System Test");
			ImGui::Text("Press SPACE: Add bullet hole at player");
			ImGui::Text("Press N: Add random bullet hole");
			ImGui::Text("Press M: Clear all bullet holes");

			// 現在の弾痕数を表示
			auto &cloudParams = cloud_->GetMutableParams();
			ImGui::Text("Active Bullet Holes: %d", cloudParams.bulletHoleCount);

			// 弾痕パラメータの調整
			ImGui::SliderFloat("Fade Start", &cloudParams.bulletHoleFadeStart, -2.0f, 2.0f);
			ImGui::SliderFloat("Fade End", &cloudParams.bulletHoleFadeEnd, 0.0f, 5.0f);

			ImGui::End();
		}
	}

	//========================================
	// 当たり判定 ウィンドウ
	{
		ImGui::SetNextWindowPos(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Collision Manager Debug");
		ImGui::Text("Collision Manager");
		// collisionManager_->DrawImGui(); // コメントアウト
		ImGui::End();
	}

	//========================================
	// UI系 - UIManagerが独立したウィンドウを管理するため、
	// 外側のウィンドウなしで直接呼び出す
	if (uiManager_) {
		uiManager_->DrawImGui();
	}

	//========================================
	// トランジション ウィンドウ
	{
		if (sceneTransition_) {
			ImGui::SetNextWindowPos(ImVec2(800.0f, 600.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f), ImGuiCond_FirstUseEver);
			ImGui::Begin("Scene Transition Debug");
			sceneTransition_->DrawImGui();
			ImGui::End();
		}
	}
#endif // _DEBUG
}

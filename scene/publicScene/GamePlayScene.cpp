/*********************************************************************
 * \file   GamePlayScene.cpp
 * \brief  ゲームプレイシーン実装
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップにアクセス
 *********************************************************************/
#define NOMINMAX
#include "GamePlayScene.h"
#include "EditorUiSystem.h"
#include "EngineContext.h"
#include "Input.h"
#include "Logger.h"
#include "SceneContext.h"
//========================================
// Game
#include "CameraManager.h"
#include "CollisionManager.h"
#include "DebugTextManager.h"
#include "EnemyManager.h"
#include "EnemyBullet.h"
#include "FollowCamera.h"
#include "MenuUI.h"
#include "ModelManager.h"
#include "Player.h"
#include "SceneTransition.h"
#include "Skydome.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace MagEngine;

namespace {
	constexpr const char *kDefaultStagePath = "resources/config/stage/stage_01.json";

	const char *GetGamePlayPhaseName(GamePlayPhase phase) {
		switch (phase) {
		case GamePlayPhase::Starting:
			return "Starting";
		case GamePlayPhase::Playing:
			return "Playing";
		case GamePlayPhase::GameClearPresentation:
			return "GameClearPresentation";
		case GamePlayPhase::GameOverPresentation:
			return "GameOverPresentation";
		case GamePlayPhase::TransitionOut:
			return "TransitionOut";
		}
		return "Unknown";
	}
}

///=============================================================================
/// 初期化
/// NOTE: contextからセットアップを取得
void GamePlayScene::Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) {
	engineContext.Validate();
	engineContext_ = &engineContext;
	sceneContext_ = &sceneContext;
	CameraManager *cameraManager = engineContext_->cameraManager;

	// NOTE: EngineContextからセットアップを取得し、SceneContextと責務を分離する
	MagEngine::SpriteSetup *spriteSetup = engineContext_->spriteSetup;
	MagEngine::Object3dSetup *object3dSetup = engineContext_->object3dSetup;
	MagEngine::ParticleSetup *particleSetup = engineContext_->particleSetup;
	MagEngine::SkyboxSetup *skyboxSetup = engineContext_->skyboxSetup;
	MagEngine::CloudSetup *cloudSetup = engineContext_->cloudSetup;

	//========================================
	// カメラ設定
	cameraManager->AddCamera("FollowCamera");
	cameraManager->GetCamera("FollowCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.3f, -8.0f}});

	//========================================
	// FollowCameraの初期化
	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize("FollowCamera", *engineContext_->cameraManager, *engineContext_->input);

	//========================================
	// DebugTextManagerにカメラを設定
	engineContext_->debugTextManager->SetCamera(cameraManager->GetCamera("FollowCamera"));

	// モデルの環境マップ設定
	engineContext_->modelManager->GetModelSetup()->SetEnvironmentTexture("overcast_soil_puresky_4k.dds");

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
	player_->Initialize(object3dSetup, "jet.obj", *engineContext_->input, *engineContext_->lineManager);
	// FollowCameraにプレイヤーを設定
	followCamera_->SetTarget(player_.get());
	// FollowCameraをメインカメラに設定
	cameraManager->SetCurrentCamera("FollowCamera");



	//========================================
	// 雲
	cloud_ = std::make_unique<Cloud>();
	cloud_->Initialize(cloudSetup);
	// NOTE: 敵弾収集用のバッファを再利用し、弾道穴の更新中に確保しない。
	enemyBulletsForCloud_.reserve(32);

	// 雲のサイズ設定（広い範囲に配置）
	cloud_->SetSize({500.0f, 100.0f, 500.0f});
	cloud_->SetEnabled(true);

	// 雲のTransform設定
	cloud_->GetTransform().translate = {0.0f, -35.0f, 250.0f};

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

#ifdef _DEBUG
	debugController_.Initialize(engineContext_->input, player_.get(), cloud_.get());
#endif
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
	
	// プレイヤーにTrailEffectManagerを設定（弾・ミサイルトレイル用）
	MagEngine::TrailEffectManager *trailEffectManager = engineContext_->trailEffectManager;
	
	enemyManager_->Initialize(object3dSetup, particle_.get(), particleSetup, trailEffectManager);
	// プレイヤー参照を設定
	enemyManager_->SetPlayer(player_.get());
	const StageLoadResult stageLoadResult = LoadDefaultStageDefinition();
	stageConfigurationPath_ = stageLoadResult.sourcePath;
	stageValidationError_ = stageLoadResult.errorMessage;
	if (!stageLoadResult.IsSuccess()) {
		throw std::runtime_error("GamePlayScene stage configuration error: " + stageValidationError_);
	}
	gameFlowController_.Initialize(stageLoadResult.stageDefinition);

	// プレイヤーにEnemyManagerを設定（ミサイル用）
	player_->SetEnemyManager(enemyManager_.get());

	// プレイヤーにTrailEffectManagerを設定（弾・ミサイルトレイル用）
	if (trailEffectManager) {
		player_->SetTrailEffectManager(trailEffectManager);
	}

	//========================================
	// 当たり判定（軽量システムで初期化）
	collisionManager_ = std::make_unique<CollisionManager>();
	collisionManager_->Initialize(*engineContext_->lineManager, 32.0f, 256); // セルサイズ32.0f、最大256オブジェクト
	collisionCoordinator_ = std::make_unique<GamePlayCollisionCoordinator>(*collisionManager_);

	//========================================
#ifdef _DEBUG
	// デバッグ表示はReleaseビルドへ持ち込まず、シーン終了時に名前で解除できるようにする。
	engineContext_->debugTextManager->AddText3D("Enemy", {5.0f, 1.0f, 5.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
	engineContext_->debugTextManager->AddText3D("Player", player_->GetPosition(), {0.0f, 1.0f, 0.0f, 1.0f});
#endif

	//========================================
	// UI管理の初期化
	uiManager_ = std::make_unique<UIManager>();
	uiManager_->Initialize(spriteSetup, object3dSetup, *engineContext_->input, *engineContext_->cameraManager, *engineContext_->lineManager, *engineContext_->textRenderer);

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
			BeginTransitionOut(1.0f);
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
			BeginTransitionOut(1.5f);
		});
	}
	isGameClear_ = false;
	phase_ = GamePlayPhase::Starting;
	hasSceneChangeRequested_ = false;

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

#ifdef _DEBUG
	RegisterEditorPanels();
#endif
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
	#ifdef _DEBUG
	if (engineContext_ && engineContext_->debugTextManager) {
		engineContext_->debugTextManager->RemoveText3D("Enemy");
		engineContext_->debugTextManager->RemoveText3D("Player");
	}
	debugController_.Finalize();
	#endif
	// リソースの適切なクリーンアップ
	// unique_ptrは自動的に破棄されますが、明示的な終了処理が
	// 必要なコンポーネントがあれば追加します
	if (collisionCoordinator_) {
		collisionCoordinator_.reset();
	}
	if (collisionManager_) {
		collisionManager_->ClearAll();
		collisionManager_.reset();
	}
	if (particle_) {
		particle_.reset();
	}
	if (enemyManager_) {
		gameFlowController_.Clear();
		// 理由：EnemyGroupはEnemyを非所有参照するため、Flow側のGroupを先に破棄してからEnemy所有を解放する。
		enemyManager_->Clear();
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
void GamePlayScene::Update(const FrameTime &frameTime) {
	assert(engineContext_);
	CameraManager *cameraManager = engineContext_->cameraManager;
	const float unscaledDeltaTime = frameTime.unscaledDeltaTime;

	//========================================
	// UI系の更新（メニュー状態確認用）
	if (uiManager_) {
		uiManager_->Update(player_.get(), unscaledDeltaTime);
	}

#ifdef _DEBUG
	// デバッグ入力はポーズ判定より前に処理し、F10でいつでも有効状態を切り替えられるようにする。
	debugController_.Update();
#endif

	UpdateStartPhase();

	ProcessMenuInput();

	//========================================
	// メニュー中はゲーム更新をスキップ
	if (IsPaused()) {
		return;
	}

	if (phase_ == GamePlayPhase::GameClearPresentation || phase_ == GamePlayPhase::GameOverPresentation) {
		UpdateSceneTransition(unscaledDeltaTime);
		UpdateTerminalPresentation();
		return;
	}
	if (phase_ == GamePlayPhase::TransitionOut) {
		UpdateSceneTransition(unscaledDeltaTime);
		UpdateTransitionOut();
		return;
	}

	//========================================
	// タイムスケール計算（ジャスト回避スロー効果）
	// NOTE: 衝突で成立したジャスト回避はPlayer更新後に確定するため、
	//       このフレームで取得する倍率は次フレームのゲーム進行へ反映される。
	gameTimeScale_ = 1.0f; // デフォルト: 通常速度
	if (player_) {
		gameTimeScale_ = player_->GetJustAvoidanceComponent()->GetGameTimeScale();
	}
	// NOTE: TimeScaleはGamePlayだけの概念のため、Engine共通FrameTimeではなくここで一度だけ適用する。
	const float timeScale = std::isfinite(gameTimeScale_) ? (std::max)(0.0f, gameTimeScale_) : 0.0f;
	const float gameplayDeltaTime = frameTime.unscaledDeltaTime * timeScale;
	currentFrameTime_ = {
		frameTime.rawDeltaTime,
		frameTime.unscaledDeltaTime,
		gameplayDeltaTime,
	};

	//========================================
	// 雲の更新
	if (cloud_) {
		cloud_->Update(*cameraManager->GetCurrentCamera(), gameplayDeltaTime);
	}

	UpdateSceneTransition(unscaledDeltaTime);

	//========================================
	// FollowCameraの更新
	if (followCamera_) {
		followCamera_->Update();
	}

	//========================================
	// プレイヤー
	if (player_) {
		player_->Update(unscaledDeltaTime, gameplayDeltaTime);

		// デバッグテキストは追加せず、初期化時に登録した項目だけを更新する。
#ifdef _DEBUG
		Vector3 playerPos = player_->GetPosition();
		playerPos.y += 2.0f; // プレイヤーの少し上に表示
		engineContext_->debugTextManager->UpdateText3D("Player", playerPos, {0.0f, 1.0f, 0.0f, 1.0f});
#endif

		// ジャスト回避成功時のカメラズーム演出を開始
		if (player_->IsJustAvoidanceSuccessThisFrame() && followCamera_) {
			float currentFov = followCamera_->GetCamera()->GetFovY();
			float targetFov = currentFov * 0.85f; // 視野角を15%狭める（ズームイン）
			float duration = 0.25f; // 0.25秒かけてズーム
			// FOVアニメーション中は新しいアニメーションを開始しない
			if (!followCamera_->IsFovAnimating()) {
				followCamera_->StartFovZoomAnimation(targetFov, duration);
			}

			// HUDにジャスト回避成功演出を通知
			if (uiManager_) {
				auto hud = uiManager_->GetHUD();
				if (hud) {
					float successRate = player_->GetLastJustAvoidanceSuccessRate();
					hud->PlayJustAvoidanceEffect(successRate);
				}
			}
		}
	}

	//========================================
	// 敵進行 / Wave / Clear判定
	if (enemyManager_) {
		gameFlowController_.Update(gameplayDeltaTime, *enemyManager_, player_.get());
		if (gameFlowController_.IsGameOver()) {
			RequestGameOver();
		} else if (gameFlowController_.IsCleared()) {
			RequestGameClear();
		}
	}

	UpdateCloudProjectileHoles(gameplayDeltaTime);

	if (!IsSimulationEnabled()) {
		if (particle_) {
			particle_->Update();
		}
		return;
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

	// 当たり判定の実行位置は維持し、型別の登録だけをCoordinatorへ集約する。
	if (collisionCoordinator_) {
		collisionCoordinator_->Execute(player_.get(), enemyManager_.get());
	}

#ifdef _DEBUG
	Input *input = engineContext_->input;
	//========================================
	// タイトルへのシーン遷移（デバッグ用）
	if (IsSimulationEnabled() && input->TriggerKey(DIK_RETURN)) {
		BeginTransitionOut(1.0f);
	}
#endif
}

void GamePlayScene::UpdateCloudProjectileHoles(float deltaTime) {
	if (!cloud_ || !player_ || !enemyManager_) {
		return;
	}

	cloudProjectileHoleTimer_ += deltaTime;
	if (cloudProjectileHoleTimer_ < 0.08f) {
		return;
	}
	cloudProjectileHoleTimer_ = 0.0f;

	// NOTE: 固定8本の雲穴を弾道へ優先配分し、プレイヤー弾と敵弾の両方を必ず対象にする。
	const auto addCylinderHole = [this](const Vector3 &position, const Vector3 &velocity, float radius, float length) {
		const float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
		if (speed <= 0.001f) {
			return;
		}
		const Vector3 direction = {velocity.x / speed, velocity.y / speed, velocity.z / speed};
		const Vector3 origin = {
			position.x - direction.x * (length * 0.25f),
			position.y - direction.y * (length * 0.25f),
			position.z - direction.z * (length * 0.25f),
		};
		cloud_->AddMovementHole(origin, direction, radius, length, 0.25f);
	};

	const auto &playerBullets = player_->GetBullets();
	for (auto bulletIt = playerBullets.rbegin(); bulletIt != playerBullets.rend() && bulletIt - playerBullets.rbegin() < 4; ++bulletIt) {
		if (*bulletIt && (*bulletIt)->IsAlive()) {
			// NOTE: 雲の最大対角より長く取り、弾道が雲の反対側まで貫通するようにする。
			addCylinderHole((*bulletIt)->GetPosition(), (*bulletIt)->GetVelocity(), 2.5f, 800.0f);
		}
	}

	enemyManager_->CollectEnemyBullets(enemyBulletsForCloud_);
	for (auto bulletIt = enemyBulletsForCloud_.rbegin(); bulletIt != enemyBulletsForCloud_.rend() && bulletIt - enemyBulletsForCloud_.rbegin() < 4; ++bulletIt) {
		if (*bulletIt && (*bulletIt)->IsAlive()) {
			addCylinderHole((*bulletIt)->GetPosition(), (*bulletIt)->GetVelocity(), 2.0f, 800.0f);
		}
	}
}

///=============================================================================
/// NOTE: SceneManagerのPreflightと初期化で同一のStage Loaderを通し、
///       不正な設定を既定値へ置換せずに拒否する。
StageLoadResult GamePlayScene::LoadDefaultStageDefinition() {
	return StageDefinitionLoader::Load(kDefaultStagePath);
}

void GamePlayScene::UpdateStartPhase() {
	if (phase_ != GamePlayPhase::Starting || hasUIDeploymentStarted_ || !uiManager_) {
		return;
	}

	StartAnimation *startAnimation = uiManager_->GetStartAnimation();
	if (!startAnimation || !startAnimation->IsDone()) {
		return;
	}

	hasUIDeploymentStarted_ = true;
	phase_ = GamePlayPhase::Playing;
	if (auto hud = uiManager_->GetHUD()) {
		hud->StartDeployAnimation(1.5f);
	}
	if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
		operationGuide->SetVisible(true);
		operationGuide->StartDeployAnimation(1.0f);
	}
}

void GamePlayScene::ProcessMenuInput() {
	if (!uiManager_ || !uiManager_->GetMenuUI() || !uiManager_->GetMenuUI()->IsOpen()) {
		return;
	}

	MenuUI *menuUI = uiManager_->GetMenuUI();
	if (!menuUI->IsButtonPressed()) {
		return;
	}

	const MenuButton selectedButton = menuUI->GetSelectedButton();
	menuUI->ResetButtonPressedFlag();
	if (selectedButton == MenuButton::ResumeGame) {
		menuUI->Close();
	} else if (selectedButton == MenuButton::OperationGuide) {
		menuUI->Close();
		if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
			operationGuide->SetVisible(true);
			operationGuide->StartDeployAnimation(1.0f);
		}
	} else if (selectedButton == MenuButton::ReturnToTitle) {
		menuUI->Close();
		BeginTransitionOut(1.0f);
	}
}

void GamePlayScene::UpdateTerminalPresentation() {
	if (followCamera_) {
		followCamera_->Update();
	}
	if (particle_) {
		particle_->Update();
	}
}

void GamePlayScene::UpdateTransitionOut() {
	UpdateTerminalPresentation();
}

void GamePlayScene::UpdateSceneTransition(float unscaledDeltaTime) {
	if (sceneTransition_) {
		sceneTransition_->Update(unscaledDeltaTime);
	}
}

bool GamePlayScene::IsPaused() const {
	return uiManager_ && uiManager_->GetMenuUI() && uiManager_->GetMenuUI()->IsOpen();
}

bool GamePlayScene::IsSimulationEnabled() const {
	return phase_ == GamePlayPhase::Starting || phase_ == GamePlayPhase::Playing;
}

void GamePlayScene::RequestGameClear() {
	if (!IsSimulationEnabled()) {
		return;
	}

	phase_ = GamePlayPhase::GameClearPresentation;
	isGameClear_ = true;
	if (!uiManager_) {
		return;
	}

	uiManager_->SetGameClear(true);
	if (auto gameClearAnimation = uiManager_->GetGameClearAnimation()) {
		gameClearAnimation->StartClearAnimation(1.0f, 2.0f, 3.0f, 1.0f);
	}
	if (auto hud = uiManager_->GetHUD()) {
		if (!hud->IsAnimating()) {
			hud->StartRetractAnimation(1.0f);
		}
	}
	if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
		operationGuide->StartRetractAnimation(0.6f);
	}
}

void GamePlayScene::RequestGameOver() {
	if (!IsSimulationEnabled()) {
		return;
	}

	phase_ = GamePlayPhase::GameOverPresentation;
	isGameOver_ = true;
	if (!uiManager_) {
		return;
	}

	uiManager_->SetGameOver(true);
	if (auto gameOverUI = uiManager_->GetGameOverUI()) {
		gameOverUI->Play(0.8f, 2.5f, 1.2f);
	}
	if (auto hud = uiManager_->GetHUD()) {
		if (!hud->IsAnimating()) {
			hud->StartRetractAnimation(1.0f);
		}
	}
	if (auto operationGuide = uiManager_->GetOperationGuideUI()) {
		operationGuide->StartRetractAnimation(0.6f);
	}
}

void GamePlayScene::BeginTransitionOut(float transitionDuration) {
	if (phase_ == GamePlayPhase::TransitionOut || !sceneTransition_ || sceneTransition_->IsTransitioning()) {
		return;
	}

	phase_ = GamePlayPhase::TransitionOut;
	sceneTransition_->StartClosing(TransitionType::Fade, transitionDuration);
	sceneTransition_->SetOnCompleteCallback([this]() {
		// SceneTransitionは完了状態を保持するため、Scene変更要求も一度だけに制限する。
		if (!hasSceneChangeRequested_) {
			hasSceneChangeRequested_ = true;
			SetSceneNo(SCENE::TITLE);
		}
	});
}

///=============================================================================
///                        描画対象の登録
void GamePlayScene::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	if(skybox_) {
		skybox_->RegisterRenderables(renderWorld);
	}
	if(player_) {
		player_->RegisterRenderables(renderWorld);
	}
	if(enemyManager_ && !isGameClear_) {
		enemyManager_->RegisterRenderables(renderWorld);
	}
	if (cloud_) {
		cloud_->RegisterRenderables(renderWorld);
	}
	if (particle_) {
		particle_->RegisterRenderables(renderWorld);
	}
	if (uiManager_) {
		uiManager_->RegisterRenderables(renderWorld);
	}
	if (sceneTransition_) {
		sceneTransition_->RegisterRenderables(renderWorld);
	}
}

#ifdef _DEBUG
///=============================================================================
///						Editor Panel登録
void GamePlayScene::RegisterEditorPanels() {
	// NOTE: Scene所有オブジェクトを参照するため、Scene切替時は必ずClearScenePanelsで登録を破棄する。
	engineContext_->editorUiSystem->RegisterPanel("GamePlay", MagEngine::EditorUiCategory::Scene, true, [this]() {
		DrawDebugUi();
	});
}

///=============================================================================
///						GamePlay Debug UI
void GamePlayScene::DrawDebugUi() {
	ImGui::Text("Raw Delta Time: %.4f", currentFrameTime_.rawDeltaTime);
	ImGui::Text("Unscaled Delta Time: %.4f", currentFrameTime_.unscaledDeltaTime);
	ImGui::Text("Gameplay Delta Time: %.4f", currentFrameTime_.gameplayDeltaTime);
	ImGui::Text("Time Scale: %.2f", gameTimeScale_);
	ImGui::Text("Phase: %s", GetGamePlayPhaseName(phase_));
	ImGui::Text("Paused: %s", IsPaused() ? "true" : "false");
	ImGui::Text("Simulation: %s", IsSimulationEnabled() && !IsPaused() ? "enabled" : "disabled");
	ImGui::Text("Collision: %s", IsSimulationEnabled() && !IsPaused() ? "enabled" : "disabled");
	ImGui::Text("Stage: %s", stageConfigurationPath_.c_str());
	ImGui::Text("Stage Validation: %s", stageValidationError_.empty() ? "Valid" : stageValidationError_.c_str());
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen) && followCamera_) {
		followCamera_->DrawImGui();
	}
	if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawPlayerDebugUi();
	}
	if (ImGui::CollapsingHeader("Enemy", ImGuiTreeNodeFlags_DefaultOpen)) {
		DrawEnemyDebugUi();
	}
	if (ImGui::CollapsingHeader("Cloud")) {
		DrawCloudDebugUi();
	}
	if (ImGui::CollapsingHeader("Collision")) {
		DrawCollisionDebugUi();
	}
	if (ImGui::CollapsingHeader("UI")) {
		DrawUiDebugUi();
	}
	if (ImGui::CollapsingHeader("Transition")) {
		DrawTransitionDebugUi();
	}
}

void GamePlayScene::DrawPlayerDebugUi() {
	if (!player_) {
		ImGui::TextDisabled("Player is not available.");
		return;
	}

	player_->DrawImGui();
	if (ImGui::TreeNode("Missiles")) {
		const auto &missiles = player_->GetMissiles();
		for (size_t i = 0; i < missiles.size(); ++i) {
			if (missiles[i] && missiles[i]->IsAlive()) {
				ImGui::PushID(static_cast<int>(i));
				missiles[i]->DrawImGui();
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}
}

void GamePlayScene::DrawEnemyDebugUi() {
	if (!enemyManager_) {
		ImGui::TextDisabled("EnemyManager is not available.");
		return;
	}

	const WaveController &waveController = gameFlowController_.GetWaveController();
	ImGui::Text("Game Flow: %s", ToString(gameFlowController_.GetState()));
	ImGui::Text("Wave: %zu / %zu", waveController.GetCurrentWaveIndex() + 1, waveController.GetTotalWaveCount());
	ImGui::Text("Wave ID: %s", waveController.GetCurrentWaveId().c_str());
	ImGui::Text("Wave State: %s", ToString(waveController.GetState()));
	ImGui::Text("Active Enemy Count: %zu", enemyManager_->GetActiveEnemyCount());
	ImGui::Text("Active Group Count: %zu", waveController.GetActiveGroupCount());
	ImGui::Text("Pending Spawn Groups: %zu", waveController.GetPendingSpawnGroupCount());
	ImGui::Text("Clear Delay Timer: %.2f", gameFlowController_.GetClearDelayTimer());
	const StageDefinition &stageDefinition = gameFlowController_.GetStageDefinition();
	ImGui::Text("Clear When All Waves Completed: %s", stageDefinition.clearWhenAllWavesCompleted ? "true" : "false");
	if (waveController.GetCurrentWaveIndex() < stageDefinition.waves.size()) {
		const WaveDefinition &waveDefinition = stageDefinition.waves[waveController.GetCurrentWaveIndex()];
		ImGui::Text("Wait For All Groups Finished: %s", waveDefinition.waitForAllGroupsFinished ? "true" : "false");
		ImGui::Text("Wait For All Enemies Removed: %s", waveDefinition.waitForAllEnemiesRemoved ? "true" : "false");
		for (const SpawnGroupDefinition &groupDefinition : waveDefinition.spawnGroups) {
			for (const EnemySpawnDefinition &enemyDefinition : groupDefinition.members) {
				ImGui::Text("%s: HP x%.2f / Speed x%.2f / Shot Offset %.2f",
						groupDefinition.groupId.c_str(),
						enemyDefinition.healthMultiplier,
						enemyDefinition.speedMultiplier,
						enemyDefinition.shotDelayOffset);
			}
		}
	}

	if (ImGui::TreeNode("Enemy Groups")) {
		for (const auto &group : waveController.GetGroups()) {
			if (!group) {
				continue;
			}
			ImGui::Text("Group %d Pattern %d State %d Members %u Active %u Finish %s",
						group->GetGroupId(),
						static_cast<int>(group->GetFormationPattern()),
						static_cast<int>(group->GetState()),
						static_cast<uint32_t>(group->GetMembers().size()),
						group->GetActiveMemberCount(),
						ToString(group->GetFinishReason()));
		}
		ImGui::TreePop();
	}

	enemyManager_->DrawImGui();
}

void GamePlayScene::DrawCloudDebugUi() {
	if (!cloud_) {
		ImGui::TextDisabled("Cloud is not available.");
		return;
	}

	cloud_->DrawImGui();
	ImGui::Separator();
	ImGui::Text("Bullet Hole System Test");
	ImGui::Text("F10: Toggle debug input");
	ImGui::Text("J: Add bullet hole at player");
	ImGui::Text("K: Add random bullet hole");
	ImGui::Text("L: Clear all bullet holes");

	auto &cloudParams = cloud_->GetMutableParams();
	ImGui::Text("Active Bullet Holes: %d", cloudParams.bulletHoleCount);
	ImGui::SliderFloat("Fade Start", &cloudParams.bulletHoleFadeStart, -2.0f, 2.0f);
	ImGui::SliderFloat("Fade End", &cloudParams.bulletHoleFadeEnd, 0.0f, 5.0f);
}

void GamePlayScene::DrawCollisionDebugUi() {
	ImGui::Text("Collision Manager");
	ImGui::TextDisabled("Detailed collision debug drawing is currently disabled.");
}

void GamePlayScene::DrawUiDebugUi() {
	if (uiManager_) {
		uiManager_->DrawImGui();
	}
}

void GamePlayScene::DrawTransitionDebugUi() {
	if (sceneTransition_) {
		sceneTransition_->DrawImGui();
	}
}
#endif

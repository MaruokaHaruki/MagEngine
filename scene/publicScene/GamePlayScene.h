/*********************************************************************
 * \file   GamePlayScene.h
 * \brief  ゲームプレイシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>
//========================================
// Engine
#include "Cloud.h"
#include "CloudSetup.h"
#include "Object3d.h"
#include "Object3dSetup.h"
#include "Particle.h"
#include "ParticleSetup.h"
#include "Skybox.h"
#include "SkyboxSetup.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include "gameplay/GamePlayCollisionCoordinator.h"
#include "stage/EnemyStageSystem.h"
#ifdef _DEBUG
#include "gameplay/GamePlayDebugController.h"
#endif

//========================================
// Game
#include "UIManager.h"

//========================================
// Forward declaration
class CollisionManager;
class FollowCamera;
class Skydome;
class Player;
class EnemyManager;
class EnemyBullet;
class SceneTransition;
class SceneContext;
namespace MagEngine {
	struct EngineContext;
}

enum class GamePlayPhase {
	Starting,
	Playing,
	GameClearPresentation,
	GameOverPresentation,
	TransitionOut,
};

///=============================================================================
///                         ゲームプレイシーンクラス
class GamePlayScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	GamePlayScene();
	~GamePlayScene() override;

	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) override;
	void Finalize() override;

	void Update(const FrameTime &frameTime) override;

	/// \brief 描画対象登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld) override;

	/// \brief 既定Stage設定を厳密に読み込む
	/// \note Scene切替前のPreflightと実初期化で同じ検証経路を使用するために公開する。
	static StageLoadResult LoadDefaultStageDefinition();

	///--------------------------------------------------------------
	///							静的メンバ関数
	///--------------------------------------------------------------
	///							入出力関数
public:

#ifdef _DEBUG
	void RegisterEditorPanels() override;
#endif

private:
	class PhaseStateBase;
	class StartingPhaseState;
	class PlayingPhaseState;
	class GameClearPresentationPhaseState;
	class GameOverPresentationPhaseState;
	class TransitionOutPhaseState;

	/// \brief 現在のフェーズ状態へ更新処理を委譲する
	/// \param frameTime 今フレームの時間情報
	void UpdatePhaseState(const FrameTime &frameTime);
	/// \brief 状態更新完了後に保留したフェーズ遷移を適用する
	/// @note Update中に状態オブジェクト自身を破棄しないため、委譲後に呼び出す。
	void ApplyPendingPhaseState();
	/// \brief 指定したフェーズへの遷移を要求する
	/// \param nextPhase 遷移先のフェーズ
	/// @note enumはImGui表示とシミュレーション可否の識別に残し、振る舞いの選択は状態オブジェクトへ委譲する。
	void RequestPhaseState(GamePlayPhase nextPhase);
	/// \brief 開始・プレイ中に共通のゲームシミュレーションを更新する
	/// \param frameTime 今フレームの時間情報
	void UpdateSimulation(const FrameTime &frameTime);
	void UpdateStartPhase();
	void ProcessMenuInput();
	void UpdateTerminalPresentation();
	void UpdateTransitionOut();
	void UpdateSceneTransition(float unscaledDeltaTime);
	void UpdateCloudProjectileHoles(float deltaTime);
	bool IsPaused() const;
	bool IsSimulationEnabled() const;
	void RequestGameClear();
	void RequestGameOver();
	void BeginTransitionOut(float transitionDuration);

	void DrawDebugUi();
	void DrawPlayerDebugUi();
	void DrawEnemyDebugUi();
	void DrawCloudDebugUi();
	void DrawCollisionDebugUi();
	void DrawUiDebugUi();
	void DrawTransitionDebugUi();

	///--------------------------------------------------------------
	///							メンバ変数
private:
	struct GamePlayFrameTime {
		float rawDeltaTime = 0.0f;
		float unscaledDeltaTime = 0.0f;
		float gameplayDeltaTime = 0.0f;
	};

	//========================================
	// EngineContext
	const MagEngine::EngineContext *engineContext_ = nullptr;
	SceneContext *sceneContext_ = nullptr;

	//========================================
	// 当たり判定
	std::unique_ptr<CollisionManager> collisionManager_;
	std::unique_ptr<GamePlayCollisionCoordinator> collisionCoordinator_;

	//========================================
	// カメラ
	std::unique_ptr<FollowCamera> followCamera_;

	//=========================================
	// スカイドーム
	std::unique_ptr<Skydome> skydome_;

	//========================================
	// プレイヤー
	std::unique_ptr<Player> player_;

	//========================================
	// 敵
	std::unique_ptr<EnemyManager> enemyManager_;
	GameFlowController gameFlowController_;

	//========================================
	// スプライト
	std::unique_ptr<MagEngine::Sprite> moveSprite_;

	//========================================
	// パーティクル
	std::unique_ptr<MagEngine::Particle> particle_;

	//========================================
	// 雲
	std::unique_ptr<MagEngine::Cloud> cloud_;
	std::vector<EnemyBullet *> enemyBulletsForCloud_;
	float cloudProjectileHoleTimer_ = 0.0f;

	//=========================================
	// Skybox
	std::unique_ptr<MagEngine::Skybox> skybox_;

	//========================================
	// トランジション
	std::unique_ptr<SceneTransition> sceneTransition_;

	//========================================
	// UI管理
	std::unique_ptr<UIManager> uiManager_;

#ifdef _DEBUG
	GamePlayDebugController debugController_;
#endif

	//========================================
	// ゲーム状態
	GamePlayPhase phase_ = GamePlayPhase::Starting; // ImGui表示と共通判定用の識別子。振る舞いはphaseStateObject_が保持する。
	std::unique_ptr<PhaseStateBase> phaseStateObject_;
	std::unique_ptr<PhaseStateBase> pendingPhaseStateObject_;
	bool isGameOver_ = false;
	bool isGameClear_ = false;
	bool hasSceneChangeRequested_ = false;
	bool hasUIDeploymentStarted_ = false; // UI展開開始フラグ

	//========================================
	// タイムスケール（ジャスト回避スロー効果用）
	float gameTimeScale_ = 1.0f; // デフォルト: 1.0x（通常速度）
	GamePlayFrameTime currentFrameTime_{};
	std::string stageConfigurationPath_;
	std::string stageValidationError_;

};

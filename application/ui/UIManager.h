#pragma once
#include "GameClearAnimation.h"
#include "GameOverUI.h"
#include "HUD.h"
#include "LockOnHUD.h"
#include "MenuUI.h"
#include "Object3dSetup.h"
#include "OperationGuideUI.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include "StartAnimation.h"
#include <memory>

namespace MagEngine {
	class Input;
	class CameraManager;
	class LineManager;
	class RenderWorld;
}

///=============================================================================
///                        UI管理クラス
class UIManager {
public:
	/// \brief 初期化
	void Initialize(MagEngine::SpriteSetup *spriteSetup,
					MagEngine::Object3dSetup *object3dSetup,
					MagEngine::Input &input,
					MagEngine::CameraManager &cameraManager,
					MagEngine::LineManager &lineManager);

	/// \brief 終了処理
	void Finalize();

	/// \brief 更新
	void Update(const class Player *player, float unscaledDeltaTime);

	/// \brief Sprite描画対象を登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        UI要素のアクセッサ
	/// \brief GameOverUI を取得
	GameOverUI *GetGameOverUI() {
		return gameOverUI_.get();
	}

	/// \brief GameClearAnimation を取得
	GameClearAnimation *GetGameClearAnimation() {
		return gameClearAnimation_.get();
	}

	/// \brief OperationGuideUI を取得
	OperationGuideUI *GetOperationGuideUI() {
		return operationGuideUI_.get();
	}

	/// \brief StartAnimation を取得
	StartAnimation *GetStartAnimation() {
		return startAnimation_.get();
	}

	/// \brief HUD を取得
	HUD *GetHUD() {
		return hud_.get();
	}

	/// \brief MenuUI を取得
	MenuUI *GetMenuUI() {
		return menuUI_.get();
	}

	/// \brief LockOnHUD を取得
	LockOnHUD *GetLockOnHUD() {
		return lockOnHUD_.get();
	}

	///--------------------------------------------------------------
	///                        ゲーム状態管理
	/// \brief ゲームオーバー状態を設定
	void SetGameOver(bool isGameOver) {
		isGameOver_ = isGameOver;
	}

	/// \brief ゲームクリア状態を設定
	void SetGameClear(bool isGameClear) {
		isGameClear_ = isGameClear;
	}

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト設定
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	MagEngine::CameraManager *cameraManager_ = nullptr;

	// UI要素
	std::unique_ptr<GameOverUI> gameOverUI_;
	std::unique_ptr<GameClearAnimation> gameClearAnimation_;
	std::unique_ptr<OperationGuideUI> operationGuideUI_;
	std::unique_ptr<StartAnimation> startAnimation_;
	std::unique_ptr<HUD> hud_;
	std::unique_ptr<MenuUI> menuUI_;
	std::unique_ptr<LockOnHUD> lockOnHUD_;

	// ゲーム状態
	bool isGameOver_ = false;
	bool isGameClear_ = false;
};

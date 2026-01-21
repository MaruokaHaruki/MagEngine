#pragma once
#include "GameClearAnimation.h"
#include "GameOverUI.h"
#include "HUD.h"
#include "Object3dSetup.h"
#include "OperationGuideUI.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include "StartAnimation.h"
#include <memory>

///=============================================================================
///                        UI管理クラス
class UIManager {
public:
	/// \brief 初期化
	void Initialize(MagEngine::SpriteSetup *spriteSetup,
					MagEngine::Object3dSetup *object3dSetup);

	/// \brief 終了処理
	void Finalize();

	/// \brief 更新
	void Update(const class Player *player);

	/// \brief 2D描画
	void Draw();

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

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト設定
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;

	// UI要素
	std::unique_ptr<GameOverUI> gameOverUI_;
	std::unique_ptr<GameClearAnimation> gameClearAnimation_;
	std::unique_ptr<OperationGuideUI> operationGuideUI_;
	std::unique_ptr<StartAnimation> startAnimation_;
	std::unique_ptr<HUD> hud_;
};

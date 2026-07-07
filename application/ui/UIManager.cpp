#include "UIManager.h"
#include "CameraManager.h"
#include "Player.h"
#include "engine/render/pass/RenderWorld.h"

///=============================================================================
///                        初期化
void UIManager::Initialize(MagEngine::SpriteSetup *spriteSetup,
						   MagEngine::Object3dSetup *object3dSetup,
						   MagEngine::Input &input,
						   MagEngine::CameraManager &cameraManager,
						   MagEngine::LineManager &lineManager) {
	spriteSetup_ = spriteSetup;
	cameraManager_ = &cameraManager;

	// GameOverUI の初期化
	gameOverUI_ = std::make_unique<GameOverUI>();
	gameOverUI_->Initialize(spriteSetup_);

	// GameClearAnimation の初期化
	gameClearAnimation_ = std::make_unique<GameClearAnimation>();
	gameClearAnimation_->Initialize(spriteSetup_);

	// OperationGuideUI の初期化
	operationGuideUI_ = std::make_unique<OperationGuideUI>();
	operationGuideUI_->Initialize(spriteSetup_, input);

	// StartAnimation の初期化
	startAnimation_ = std::make_unique<StartAnimation>();
	startAnimation_->Initialize(spriteSetup_);

	// HUD の初期化
	hud_ = std::make_unique<HUD>();
	hud_->Initialize(cameraManager, lineManager);

	// MenuUI の初期化
	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(spriteSetup_, input);

	// LockOnHUD の初期化
	lockOnHUD_ = std::make_unique<LockOnHUD>();
	lockOnHUD_->SetLineManager(lineManager);
}

///=============================================================================
///                        終了処理
void UIManager::Finalize() {
	if (gameOverUI_) {
		gameOverUI_->Finalize();
	}
	if (gameClearAnimation_) {
		gameClearAnimation_->Finalize();
	}
	if (operationGuideUI_) {
		operationGuideUI_->Finalize();
	}
	if (startAnimation_) {
		startAnimation_->Finalize();
	}
	if (menuUI_) {
		menuUI_->Finalize();
	}
	if (lockOnHUD_) {
		lockOnHUD_->Finalize();
	}
}

///=============================================================================
///                        更新
void UIManager::Update(const Player *player) {
	if (gameOverUI_) {
		gameOverUI_->Update();
	}
	if (gameClearAnimation_) {
		gameClearAnimation_->Update();
	}
	if (operationGuideUI_) {
		operationGuideUI_->Update();
	}
	if (startAnimation_) {
		startAnimation_->Update();
	}
	if (hud_ && player) {
		hud_->Update(player);
	}
	if (menuUI_) {
		menuUI_->Update();
	}
	if (lockOnHUD_ && player) {
		MagEngine::Camera *camera = cameraManager_ ? cameraManager_->GetCamera("FollowCamera") : nullptr;
		if (camera) {
			lockOnHUD_->Update(camera);
		}
	}
}

///=============================================================================
///                        Sprite描画対象登録
void UIManager::RegisterRenderables(MagEngine::RenderWorld &renderWorld) {
	// NOTE: メニュー表示中は旧Drawと同じくゲーム中UIを隠し、メニューだけを登録する。
	if (menuUI_ && menuUI_->IsOpen()) {
		menuUI_->RegisterRenderables(renderWorld);
		return;
	}

	if (gameOverUI_) {
		gameOverUI_->RegisterRenderables(renderWorld);
	}
	if (gameClearAnimation_) {
		gameClearAnimation_->RegisterRenderables(renderWorld);
	}
	// ゲームオーバー/クリア時は操作ガイドUIを描画しない
	if (operationGuideUI_ && !isGameOver_ && !isGameClear_) {
		operationGuideUI_->RegisterRenderables(renderWorld);
	}
	if (startAnimation_) {
		startAnimation_->RegisterRenderables(renderWorld);
	}
	// NOTE: HUD/LockOnHUDはLineManagerへHUDモードで蓄積し、LineRenderPassへ委譲する。
	if (hud_) {
		hud_->Draw();
	}
	if (lockOnHUD_) {
		lockOnHUD_->Draw();
	}
	if (menuUI_) {
		menuUI_->RegisterRenderables(renderWorld);
	}
}

///=============================================================================
///                        ImGui描画
void UIManager::DrawImGui() {
#ifdef _DEBUG
	// if (gameOverUI_) {
	//	gameOverUI_->DrawImGui();
	// }
	// if (gameClearAnimation_) {
	//	gameClearAnimation_->DrawImGui();
	// }
	// if (operationGuideUI_) {
	//	operationGuideUI_->DrawImGui();
	// }
	// if (startAnimation_) {
	//	startAnimation_->DrawImGui();
	// }
	if (hud_) {
		hud_->DrawImGui();
	}
	// if (menuUI_) {
	//	menuUI_->DrawImGui();
	// }
	// if (lockOnHUD_) {
	//	lockOnHUD_->DrawImGui();
	// }
#endif
}

#include "UIManager.h"
#include "Player.h"

///=============================================================================
///                        初期化
void UIManager::Initialize(MagEngine::SpriteSetup *spriteSetup,
						   MagEngine::Object3dSetup *object3dSetup) {
	spriteSetup_ = spriteSetup;

	// GameOverUI の初期化
	gameOverUI_ = std::make_unique<GameOverUI>();
	gameOverUI_->Initialize(spriteSetup_);

	// GameClearAnimation の初期化
	gameClearAnimation_ = std::make_unique<GameClearAnimation>();
	gameClearAnimation_->Initialize(spriteSetup_);

	// OperationGuideUI の初期化
	operationGuideUI_ = std::make_unique<OperationGuideUI>();
	operationGuideUI_->Initialize(spriteSetup_);

	// StartAnimation の初期化
	startAnimation_ = std::make_unique<StartAnimation>();
	startAnimation_->Initialize(spriteSetup_);

	// HUD の初期化
	hud_ = std::make_unique<HUD>();
	hud_->Initialize();

	// MenuUI の初期化
	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(spriteSetup_);
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
}

///=============================================================================
///                        描画
void UIManager::Draw() {
	// メニューが開いている場合、メニューのみ描画
	if (menuUI_ && menuUI_->IsOpen()) {
		menuUI_->Draw();
		return;
	}

	// メニューが閉じている場合、通常のUI描画
	if (gameOverUI_) {
		gameOverUI_->Draw();
	}
	if (gameClearAnimation_) {
		gameClearAnimation_->Draw();
	}
	if (operationGuideUI_) {
		operationGuideUI_->Draw();
	}
	if (startAnimation_) {
		startAnimation_->Draw();
	}
	if (hud_) {
		hud_->Draw();
	}
	if (menuUI_) {
		menuUI_->Draw();
	}
}

///=============================================================================
///                        ImGui描画
void UIManager::DrawImGui() {
#ifdef _DEBUG
	if (gameOverUI_) {
		gameOverUI_->DrawImGui();
	}
	if (gameClearAnimation_) {
		gameClearAnimation_->DrawImGui();
	}
	if (operationGuideUI_) {
		operationGuideUI_->DrawImGui();
	}
	if (startAnimation_) {
		startAnimation_->DrawImGui();
	}
	if (hud_) {
		hud_->DrawImGui();
	}
	if (menuUI_) {
		menuUI_->DrawImGui();
	}
#endif
}

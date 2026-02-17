#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GameOverUI.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void GameOverUI::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = GameOverState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// 背景スプライトの作成
	backgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.dds");
	backgroundSprite_->SetSize({screenWidth_, screenHeight_});
	backgroundSprite_->SetPosition({0.0f, 0.0f});
	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});

	// テキストスプライトの作成
	textSprite_ = std::make_unique<MagEngine::Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetAnchorPoint({0.5f, 0.5f});
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	textSprite_->SetSize(textSize_);
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

///=============================================================================
///                        終了処理
void GameOverUI::Finalize() {
	backgroundSprite_.reset();
	textSprite_.reset();
}

///=============================================================================
///                        更新
void GameOverUI::Update() {
	if (state_ == GameOverState::Idle || state_ == GameOverState::Done) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;
	elapsedTime_ += deltaTime;
	UpdateShowing();

	// スプライト更新
	backgroundSprite_->Update();
	textSprite_->Update();
}

///=============================================================================
///                        表示状態の更新
void GameOverUI::UpdateShowing() {
	float totalDuration = fadeDuration_ + displayDuration_;
	float rawProgress = std::clamp(elapsedTime_ / totalDuration, 0.0f, 1.0f);

	// フェーズ分け
	if (elapsedTime_ < fadeDuration_) {
		// フェードインフェーズ
		float fadeProgress = elapsedTime_ / fadeDuration_;
		progress_ = EaseInOut(fadeProgress);

		// 背景を表示
		Vector4 bgColor = backgroundColor_;
		bgColor.w = progress_ * 0.8f;
		backgroundSprite_->SetColor(bgColor);

		// テキストをフェードイン＆拡大
		float scale = 0.5f + progress_ * 0.5f;
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, progress_});

	} else {
		// 表示維持フェーズ
		progress_ = 1.0f;

		// 背景を維持
		Vector4 bgColor = backgroundColor_;
		bgColor.w = 0.8f;
		backgroundSprite_->SetColor(bgColor);

		// テキストにパルス効果を追加
		float pulse = 0.95f + 0.05f * std::sin((elapsedTime_ - fadeDuration_) * 2.0f);
		textSprite_->SetSize({textSize_.x * pulse, textSize_.y * pulse});
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}

	// 完了チェック
	if (rawProgress >= 1.0f && state_ != GameOverState::Done) {
		state_ = GameOverState::Done;
		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

///=============================================================================
///                        アニメーション制御
void GameOverUI::StartGameOver(float fadeDuration, float displayDuration) {
	state_ = GameOverState::Showing;
	fadeDuration_ = fadeDuration;
	displayDuration_ = displayDuration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// 初期状態設定
	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	textSprite_->SetSize({textSize_.x * 0.5f, textSize_.y * 0.5f});
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
}

void GameOverUI::Cancel() {
	state_ = GameOverState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

///=============================================================================
///                        描画
void GameOverUI::Draw() {
	if (state_ == GameOverState::Idle) {
		return;
	}

	backgroundSprite_->Draw();
	textSprite_->Draw();
}

///=============================================================================
///                        ヘルパー関数
float GameOverUI::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

///=============================================================================
///                        ImGui描画
void GameOverUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Game Over UI");

	// 状態表示
	const char *stateNames[] = {"Idle", "Showing", "Done"}; // Completed を Done に変更
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);

	ImGui::Separator();

	// タイミング設定
	ImGui::SliderFloat("Fade Duration", &fadeDuration_, 0.5f, 5.0f);
	ImGui::SliderFloat("Display Duration", &displayDuration_, 1.0f, 10.0f);

	// 表示設定
	ImGui::ColorEdit4("Background Color", &backgroundColor_.x);
	ImGui::InputFloat2("Text Size", &textSize_.x);

	ImGui::Separator();

	// テスト用ボタン
	if (ImGui::Button("Start Game Over")) {
		StartGameOver(fadeDuration_, displayDuration_);
	}

	if (ImGui::Button("Cancel")) {
		Cancel();
	}

	ImGui::End();
#endif
}

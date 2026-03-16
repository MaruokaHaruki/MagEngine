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

	InitializeSprites();
}

///=============================================================================
///                        スプライト初期化
void GameOverUI::InitializeSprites() {
	if (!spriteSetup_)
		return;

	// 背景スプライト
	backgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.dds");
	backgroundSprite_->SetSize({SCREEN_WIDTH, SCREEN_HEIGHT});
	backgroundSprite_->SetPosition({0.0f, 0.0f});
	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});

	// テキストスプライト
	textSprite_ = std::make_unique<MagEngine::Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetAnchorPoint({0.5f, 0.5f});
	textSprite_->SetPosition({SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f});
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

	constexpr float DELTA_TIME = 1.0f / 60.0f;
	elapsedTime_ += DELTA_TIME;
	UpdateAnimation();

	backgroundSprite_->Update();
	textSprite_->Update();
}

///=============================================================================
///                        アニメーション更新
void GameOverUI::UpdateAnimation() {
	float totalDuration = fadeDuration_ + displayDuration_;
	float rawProgress = std::clamp(elapsedTime_ / totalDuration, 0.0f, 1.0f);

	if (elapsedTime_ < fadeDuration_) {
		// フェードインフェーズ
		float fadeProgress = elapsedTime_ / fadeDuration_;
		progress_ = EaseInOut(fadeProgress);

		Vector4 bgColor = backgroundColor_;
		bgColor.w = progress_ * DEFAULT_BACKGROUND_ALPHA;
		backgroundSprite_->SetColor(bgColor);

		float scale = 0.5f + progress_ * 0.5f;
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, progress_});
	} else {
		// 表示維持フェーズ
		progress_ = 1.0f;

		Vector4 bgColor = backgroundColor_;
		bgColor.w = DEFAULT_BACKGROUND_ALPHA;
		backgroundSprite_->SetColor(bgColor);

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
void GameOverUI::Play(float fadeDuration, float displayDuration) {
	state_ = GameOverState::Showing;
	fadeDuration_ = fadeDuration;
	displayDuration_ = displayDuration;
	ResetAnimation();

	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	textSprite_->SetSize({textSize_.x * 0.5f, textSize_.y * 0.5f});
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	textSprite_->SetPosition({SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f});
}

void GameOverUI::Stop() {
	state_ = GameOverState::Idle;
	ResetAnimation();

	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

///=============================================================================
///                        アニメーション初期化
void GameOverUI::ResetAnimation() {
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;
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
///                        イージング関数
float GameOverUI::EaseInOut(float t) const {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

///=============================================================================
///                        ImGui描画
void GameOverUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Game Over UI");

	const char *stateNames[] = {"Idle", "Showing", "Done"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);

	ImGui::Separator();

	ImGui::SliderFloat("Fade Duration", &fadeDuration_, 0.5f, 5.0f);
	ImGui::SliderFloat("Display Duration", &displayDuration_, 1.0f, 10.0f);

	ImGui::ColorEdit4("Background Color", &backgroundColor_.x);
	ImGui::InputFloat2("Text Size", &textSize_.x);

	ImGui::Separator();

	if (ImGui::Button("Play")) {
		Play(fadeDuration_, displayDuration_);
	}

	if (ImGui::Button("Stop")) {
		Stop();
	}

	ImGui::End();
#endif
}

#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GameOverUI.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void GameOverUI::Initialize(SpriteSetup *spriteSetup) {
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
	backgroundSprite_ = std::make_unique<Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.png");
	backgroundSprite_->SetSize({screenWidth_, screenHeight_});
	backgroundSprite_->SetPosition({0.0f, 0.0f});
	backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f}); // 初期は透明

	// テキストスプライトの作成
	textSprite_ = std::make_unique<Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetAnchorPoint({0.5f, 0.5f}); // 中心を基準点に設定
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	textSprite_->SetSize(textSize_);				 // 初期サイズを設定
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f}); // 初期は透明
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
	if (state_ == GameOverState::Idle || state_ == GameOverState::Completed) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f; // 60FPS想定
	elapsedTime_ += deltaTime;

	UpdateShowing();

	// スプライト更新
	if (backgroundSprite_) {
		backgroundSprite_->Update();
	}
	if (textSprite_) {
		textSprite_->Update();
	}
}

///=============================================================================
///                        表示状態の更新
void GameOverUI::UpdateShowing() {
	float totalDuration = fadeDuration_ + displayDuration_;
	float rawProgress = elapsedTime_ / totalDuration;

	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}

	// フェーズ分け
	if (elapsedTime_ < fadeDuration_) {
		// フェードインフェーズ
		float fadeProgress = elapsedTime_ / fadeDuration_;
		progress_ = EaseInOut(fadeProgress);

		// 背景を徐々に表示
		Vector4 bgColor = backgroundColor_;
		bgColor.w = progress_ * 0.8f; // 最大80%の不透明度
		backgroundSprite_->SetColor(bgColor);

		// テキストをフェードイン + 拡大（アンカーポイント考慮）
		float textAlpha = progress_;
		float scale = 0.5f + progress_ * 0.5f;

		// サイズを直接設定（アンカーポイントが中心なので問題なし）
		Vector2 scaledSize = {
			textSize_.x * scale,
			textSize_.y * scale};

		textSprite_->SetSize(scaledSize);
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, textAlpha});

	} else {
		// 表示維持フェーズ
		progress_ = 1.0f;

		// 背景を維持
		Vector4 bgColor = backgroundColor_;
		bgColor.w = 1.8f;
		backgroundSprite_->SetColor(bgColor);

		// テキストを維持（少しパルス効果）
		float pulseTime = elapsedTime_ - fadeDuration_;
		float pulse = 0.9f + 0.1f * sinf(pulseTime * 2.0f);

		// パルス効果も正しくサイズ調整
		Vector2 pulseSize = {
			textSize_.x * pulse,
			textSize_.y * pulse};

		textSprite_->SetSize(pulseSize);
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, pulse});
	}

	// 完了チェック
	if (rawProgress >= 1.0f) {
		state_ = GameOverState::Completed;

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
	if (backgroundSprite_) {
		backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	}
	if (textSprite_) {
		// 初期サイズを小さく設定（0.5倍）
		Vector2 initialSize = {
			textSize_.x * 0.5f,
			textSize_.y * 0.5f};
		textSprite_->SetSize(initialSize);
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

		// 位置を再設定（念のため）
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}
}

void GameOverUI::Cancel() {
	state_ = GameOverState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 全て非表示に
	if (backgroundSprite_) {
		backgroundSprite_->SetColor({backgroundColor_.x, backgroundColor_.y, backgroundColor_.z, 0.0f});
	}
	if (textSprite_) {
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}
}

void GameOverUI::Reset() {
	Cancel();
}

///=============================================================================
///                        描画
void GameOverUI::Draw() {
	if (state_ == GameOverState::Idle) {
		return;
	}

	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}
	if (textSprite_) {
		textSprite_->Draw();
	}
}

///=============================================================================
///                        イージング関数
float GameOverUI::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

///=============================================================================
///                        ImGui描画
void GameOverUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Game Over UI");

	// 状態表示
	const char *stateNames[] = {"Idle", "Showing", "Completed"};
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

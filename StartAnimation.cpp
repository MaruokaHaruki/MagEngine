#include "StartAnimation.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void StartAnimation::Initialize(SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = StartAnimationState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// 上部バーの作成
	topBar_ = std::make_unique<Sprite>();
	topBar_->Initialize(spriteSetup_, "white1x1.png");
	topBar_->SetColor(barColor_);

	// 下部バーの作成
	bottomBar_ = std::make_unique<Sprite>();
	bottomBar_->Initialize(spriteSetup_, "white1x1.png");
	bottomBar_->SetColor(barColor_);

	// テキストスプライトの作成
	textSprite_ = std::make_unique<Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetSize(textSize_);
	textSprite_->SetAnchorPoint({0.5f, 0.5f});
}

///=============================================================================
///                        終了処理
void StartAnimation::Finalize() {
	topBar_.reset();
	bottomBar_.reset();
	textSprite_.reset();
}

///=============================================================================
///                        更新
void StartAnimation::Update() {
	if (state_ == StartAnimationState::Idle || state_ == StartAnimationState::Completed) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f; // 60FPS想定
	elapsedTime_ += deltaTime;

	switch (state_) {
	case StartAnimationState::Opening:
		UpdateOpening();
		break;
	case StartAnimationState::Showing:
		UpdateShowing();
		break;
	case StartAnimationState::Closing:
		UpdateClosing();
		break;
	}

	// スプライト更新
	if (topBar_) {
		topBar_->Update();
	}
	if (bottomBar_) {
		bottomBar_->Update();
	}
	if (textSprite_) {
		textSprite_->Update();
	}
}

///=============================================================================
///                        各状態の更新処理
void StartAnimation::UpdateOpening() {
	float rawProgress = elapsedTime_ / openDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = EaseOut(rawProgress);

	float barHeight = screenHeight_ * barHeightRatio_;

	if (isReversed_) {
		// 逆再生：バーが引っ込む + フェードアウト
		float effectiveProgress = 1.0f - progress_;
		float topY = -barHeight * (1.0f - effectiveProgress);
		float bottomY = screenHeight_ - barHeight + barHeight * (1.0f - effectiveProgress);

		Vector4 color = barColor_;
		color.w = effectiveProgress;

		topBar_->SetPosition({0.0f, topY});
		topBar_->SetSize({screenWidth_, barHeight});
		topBar_->SetColor(color);

		bottomBar_->SetPosition({0.0f, bottomY});
		bottomBar_->SetSize({screenWidth_, barHeight});
		bottomBar_->SetColor(color);

		// テキストもフェードアウト + 縮小
		Vector4 textColor = {1.0f, 1.0f, 1.0f, effectiveProgress * 0.5f};
		float scale = 0.5f + effectiveProgress * 0.5f;
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor(textColor);
	} else {
		// 通常：バーが登場
		float topY = -barHeight + barHeight * progress_;
		float bottomY = screenHeight_ - barHeight * progress_;

		topBar_->SetPosition({0.0f, topY});
		topBar_->SetSize({screenWidth_, barHeight});
		topBar_->SetColor(barColor_);

		bottomBar_->SetPosition({0.0f, bottomY});
		bottomBar_->SetSize({screenWidth_, barHeight});
		bottomBar_->SetColor(barColor_);

		// テキストは非表示
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}

	if (rawProgress >= 1.0f) {
		state_ = StartAnimationState::Showing;
		elapsedTime_ = 0.0f;
		progress_ = 0.0f;
	}
}

void StartAnimation::UpdateShowing() {
	float rawProgress = elapsedTime_ / showDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = rawProgress;

	float barHeight = screenHeight_ * barHeightRatio_;

	// バーは固定位置
	topBar_->SetPosition({0.0f, 0.0f});
	topBar_->SetSize({screenWidth_, barHeight});
	topBar_->SetColor(barColor_);

	bottomBar_->SetPosition({0.0f, screenHeight_ - barHeight});
	bottomBar_->SetSize({screenWidth_, barHeight});
	bottomBar_->SetColor(barColor_);

	if (!isReversed_) {
		// 通常：テキストをフェードイン + 拡大
		float textProgress = std::min(1.0f, progress_ * 2.0f);
		float textAlpha = EaseOut(textProgress);
		float scale = 0.5f + textAlpha * 0.5f;

		Vector4 textColor = {1.0f, 1.0f, 1.0f, textAlpha};
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor(textColor);
	} else {
		// 逆再生：バーのみ表示
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}

	if (rawProgress >= 1.0f) {
		state_ = StartAnimationState::Closing;
		elapsedTime_ = 0.0f;
		progress_ = 0.0f;
	}
}

void StartAnimation::UpdateClosing() {
	float rawProgress = elapsedTime_ / closeDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = EaseInOut(rawProgress);

	float barHeight = screenHeight_ * barHeightRatio_;

	if (isReversed_) {
		// 逆再生：バーが登場
		float topY = -barHeight + barHeight * progress_;
		float bottomY = screenHeight_ - barHeight * progress_;

		topBar_->SetPosition({0.0f, topY});
		topBar_->SetSize({screenWidth_, barHeight});
		topBar_->SetColor(barColor_);

		bottomBar_->SetPosition({0.0f, bottomY});
		bottomBar_->SetSize({screenWidth_, barHeight});
		bottomBar_->SetColor(barColor_);

		// テキストをフェードイン + 拡大
		float textAlpha = progress_;
		float scale = 0.5f + textAlpha * 0.5f;

		Vector4 textColor = {1.0f, 1.0f, 1.0f, textAlpha};
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor(textColor);
	} else {
		// 通常：バーが引っ込む + フェードアウト
		float effectiveProgress = 1.0f - progress_;
		float topY = -barHeight * (1.0f - effectiveProgress);
		float bottomY = screenHeight_ - barHeight + barHeight * (1.0f - effectiveProgress);

		Vector4 color = barColor_;
		color.w = effectiveProgress;

		topBar_->SetPosition({0.0f, topY});
		topBar_->SetSize({screenWidth_, barHeight});
		topBar_->SetColor(color);

		bottomBar_->SetPosition({0.0f, bottomY});
		bottomBar_->SetSize({screenWidth_, barHeight});
		bottomBar_->SetColor(color);

		// テキストもフェードアウト + 縮小
		Vector4 textColor = {1.0f, 1.0f, 1.0f, effectiveProgress};
		float scale = 0.5f + effectiveProgress * 0.5f;
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
		textSprite_->SetColor(textColor);
	}

	if (rawProgress >= 1.0f) {
		state_ = StartAnimationState::Completed;

		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

///=============================================================================
///                        アニメーション制御
void StartAnimation::StartOpening(float showDuration, float openDuration, float closeDuration) {
	state_ = StartAnimationState::Opening;
	openDuration_ = openDuration;
	showDuration_ = showDuration;
	closeDuration_ = closeDuration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	isReversed_ = false;

	// 初期状態設定
	if (topBar_) {
		topBar_->SetColor(barColor_);
	}
	if (bottomBar_) {
		bottomBar_->SetColor(barColor_);
	}
	if (textSprite_) {
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}
}

void StartAnimation::StartClosing(float showDuration, float openDuration, float closeDuration) {
	state_ = StartAnimationState::Opening; // 逆再生も Opening から開始
	openDuration_ = openDuration;
	showDuration_ = showDuration;
	closeDuration_ = closeDuration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	isReversed_ = true;

	// 初期状態設定（逆再生なので完全表示から）
	float barHeight = screenHeight_ * barHeightRatio_;
	if (topBar_) {
		topBar_->SetPosition({0.0f, 0.0f});
		topBar_->SetSize({screenWidth_, barHeight});
		topBar_->SetColor(barColor_);
	}
	if (bottomBar_) {
		bottomBar_->SetPosition({0.0f, screenHeight_ - barHeight});
		bottomBar_->SetSize({screenWidth_, barHeight});
		bottomBar_->SetColor(barColor_);
	}
	if (textSprite_) {
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize(textSize_);
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	}
}

void StartAnimation::Cancel() {
	state_ = StartAnimationState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 全て非表示に
	if (topBar_) {
		Vector4 color = barColor_;
		color.w = 0.0f;
		topBar_->SetColor(color);
	}
	if (bottomBar_) {
		Vector4 color = barColor_;
		color.w = 0.0f;
		bottomBar_->SetColor(color);
	}
	if (textSprite_) {
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}
}

void StartAnimation::Reset() {
	Cancel();
}

///=============================================================================
///                        描画
void StartAnimation::Draw() {
	if (state_ == StartAnimationState::Idle) {
		return;
	}

	if (topBar_) {
		topBar_->Draw();
	}
	if (bottomBar_) {
		bottomBar_->Draw();
	}
	if (textSprite_) {
		textSprite_->Draw();
	}
}

///=============================================================================
///                        イージング関数
float StartAnimation::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float StartAnimation::EaseOut(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

///=============================================================================
///                        ImGui描画
void StartAnimation::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Start Animation");

	// 状態表示
	const char *stateNames[] = {"Idle", "Opening", "Showing", "Closing", "Completed"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);
	ImGui::Text("Mode: %s", isReversed_ ? "Reversed (Closing)" : "Normal (Opening)");

	ImGui::Separator();

	// タイミング設定
	ImGui::SliderFloat("Open Duration", &openDuration_, 0.1f, 3.0f);
	ImGui::SliderFloat("Show Duration", &showDuration_, 0.5f, 5.0f);
	ImGui::SliderFloat("Close Duration", &closeDuration_, 0.1f, 3.0f);

	// 表示設定
	ImGui::ColorEdit4("Bar Color", &barColor_.x);
	ImGui::SliderFloat("Bar Height Ratio", &barHeightRatio_, 0.05f, 0.3f);
	ImGui::InputFloat2("Text Size", &textSize_.x);

	ImGui::Separator();

	// テスト用ボタン
	if (ImGui::Button("Start Opening Animation")) {
		StartOpening(showDuration_, openDuration_, closeDuration_);
	}

	if (ImGui::Button("Start Closing Animation")) {
		StartClosing(showDuration_, openDuration_, closeDuration_);
	}

	if (ImGui::Button("Cancel")) {
		Cancel();
	}

	ImGui::End();
#endif
}

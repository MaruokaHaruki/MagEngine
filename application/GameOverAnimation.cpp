/*********************************************************************
 * \file   GameOverAnimation.cpp
 * \brief  ゲームオーバー演出実装
 *
 * \author Harukichimaru
 * \date   March 2026
 * \note   敗北時のテキスト表示とフェードアウト演出
 *********************************************************************/
#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GameOverAnimation.h"
#include <algorithm>
#include <cmath>

//=============================================================================
// 初期化
void GameOverAnimation::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = GameOverAnimationState::Idle;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// テキストスプライト生成
	if (spriteSetup_) {
		// 画面サイズを取得
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());

		// テキストスプライト（赤いテキスト）
		textSprite_ = std::make_unique<MagEngine::Sprite>();
		textSprite_->Initialize(spriteSetup_, "white1x1.dds");
		textSprite_->SetSize({textSize_.x, textSize_.y});
		textSprite_->SetAnchorPoint({0.5f, 0.5f});
		textSprite_->SetColor(textColor_);

		// 背景フェードスプライト
		fadeBackgroundSprite_ = std::make_unique<MagEngine::Sprite>();
		fadeBackgroundSprite_->Initialize(spriteSetup_, "white1x1.dds");
		fadeBackgroundSprite_->SetSize({screenWidth_, screenHeight_});
		fadeBackgroundSprite_->SetAnchorPoint({0.5f, 0.5f});
		fadeBackgroundSprite_->SetColor(fadeBackgroundColor_);
	}
}

//=============================================================================
// 終了処理
void GameOverAnimation::Finalize() {
	// Sprite には Finalize がないため、ここは処理なし
	textSprite_.reset();
	fadeBackgroundSprite_.reset();
}

//=============================================================================
// 更新
void GameOverAnimation::Update() {
	if (state_ == GameOverAnimationState::Idle || state_ == GameOverAnimationState::Done) {
		return;
	}

	elapsedTime_ += 1.0f / 60.0f; // 60FPS固定

	switch (state_) {
	case GameOverAnimationState::Appearing:
		UpdateAppearing();
		break;
	case GameOverAnimationState::Displaying:
		UpdateDisplaying();
		break;
	case GameOverAnimationState::Fading:
		UpdateFading();
		break;
	default:
		break;
	}

	// スプライト更新
	if (textSprite_) {
		textSprite_->Update();
	}
	if (fadeBackgroundSprite_) {
		fadeBackgroundSprite_->Update();
	}
}

//=============================================================================
// テキスト出現中の更新
void GameOverAnimation::UpdateAppearing() {
	progress_ = std::min(elapsedTime_ / appearDuration_, 1.0f);

	// テキストのアルファを増加させる（イーズイン）
	float alpha = EaseInOut(progress_);
	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = alpha;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}

	// 出現完了
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Displaying;
		elapsedTime_ = 0.0f;
	}
}

//=============================================================================
// テキスト表示中の更新
void GameOverAnimation::UpdateDisplaying() {
	progress_ = std::min(elapsedTime_ / displayDuration_, 1.0f);

	// 表示を継続
	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = 1.0f;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}

	// 表示完了
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Fading;
		elapsedTime_ = 0.0f;
	}
}

//=============================================================================
// フェードアウト中の更新
void GameOverAnimation::UpdateFading() {
	progress_ = std::min(elapsedTime_ / fadeDuration_, 1.0f);

	// テキストと背景をフェードアウト
	float alpha = 1.0f - EaseOut(progress_);

	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = alpha;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}
	if (fadeBackgroundSprite_) {
		MagMath::Vector4 bgColor = fadeBackgroundColor_;
		bgColor.w = alpha * fadeBackgroundColor_.w;
		fadeBackgroundSprite_->SetColor(bgColor);
		fadeBackgroundSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}

	// フェード完了
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Done;
		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

//=============================================================================
// ゲームオーバー演出開始
void GameOverAnimation::StartGameOverAnimation(
	float appearDuration,
	float displayDuration,
	float fadeDuration) {

	appearDuration_ = appearDuration;
	displayDuration_ = displayDuration;
	fadeDuration_ = fadeDuration;

	state_ = GameOverAnimationState::Appearing;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// テキストを表示状態に
	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = 0.0f;
		textSprite_->SetColor(color);
	}
	if (fadeBackgroundSprite_) {
		MagMath::Vector4 bgColor = fadeBackgroundColor_;
		bgColor.w = 0.0f;
		fadeBackgroundSprite_->SetColor(bgColor);
	}
}

//=============================================================================
// リセット
void GameOverAnimation::Reset() {
	state_ = GameOverAnimationState::Idle;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = 0.0f;
		textSprite_->SetColor(color);
	}
	if (fadeBackgroundSprite_) {
		MagMath::Vector4 bgColor = fadeBackgroundColor_;
		bgColor.w = 0.0f;
		fadeBackgroundSprite_->SetColor(bgColor);
	}
}

//=============================================================================
// ImGui描画（デバッグ用）
void GameOverAnimation::DrawImGui() {
	// デバッグ情報表示
	// ImGui実装は省略可能
}

//=============================================================================
// 描画
void GameOverAnimation::Draw() {
	if (state_ == GameOverAnimationState::Idle) {
		return;
	}

	// 背景をまず描画
	if (fadeBackgroundSprite_) {
		fadeBackgroundSprite_->Draw();
	}

	// テキストを描画
	if (textSprite_) {
		textSprite_->Draw();
	}
}

//=============================================================================
// イーズイン・アウト関数
float GameOverAnimation::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float GameOverAnimation::EaseOut(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float GameOverAnimation::EaseIn(float t) {
	return t * t;
}

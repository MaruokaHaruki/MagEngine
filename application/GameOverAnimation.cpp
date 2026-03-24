/*********************************************************************
 * \file   GameOverAnimation.cpp
 * \brief  Game Over animation implementation (enhanced version with luxurious effects)
 *
 * \author Harukichimaru
 * \date   March 2026
 * \note   Multiple visual effects for stylish animation
 *********************************************************************/
#include "GameOverAnimation.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

//=============================================================================
// Initialize
void GameOverAnimation::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = GameOverAnimationState::Idle;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// Initialize particles
	particles_.resize(kMaxParticles);
	for (auto &p : particles_) {
		p.active = false;
	}

	if (spriteSetup_) {
		// Get screen size
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());

		// === Main text sprite (GAME OVER) ===
		textSprite_ = std::make_unique<MagEngine::Sprite>();
		textSprite_->Initialize(spriteSetup_, "white1x1.dds");
		textSprite_->SetSize({textSize_.x, textSize_.y});
		textSprite_->SetAnchorPoint({0.5f, 0.5f});
		textSprite_->SetColor(textColor_);

		// === Glow effect sprite (background light) ===
		glowSprite_ = std::make_unique<MagEngine::Sprite>();
		glowSprite_->Initialize(spriteSetup_, "white1x1.dds");
		glowSprite_->SetSize({textSize_.x * 1.2f, textSize_.y * 1.2f});
		glowSprite_->SetAnchorPoint({0.5f, 0.5f});
		glowSprite_->SetColor({1.0f, 0.5f, 0.0f, 0.0f}); // Orange glow

		// === Top border ===
		borderSprite1_ = std::make_unique<MagEngine::Sprite>();
		borderSprite1_->Initialize(spriteSetup_, "white1x1.dds");
		borderSprite1_->SetSize({screenWidth_, 8.0f});
		borderSprite1_->SetAnchorPoint({0.5f, 0.5f});
		borderSprite1_->SetColor({1.0f, 0.3f, 0.0f, 0.0f}); // Orange

		// === Bottom border ===
		borderSprite2_ = std::make_unique<MagEngine::Sprite>();
		borderSprite2_->Initialize(spriteSetup_, "white1x1.dds");
		borderSprite2_->SetSize({screenWidth_, 8.0f});
		borderSprite2_->SetAnchorPoint({0.5f, 0.5f});
		borderSprite2_->SetColor({1.0f, 0.3f, 0.0f, 0.0f}); // Orange

		// === Background fade ===
		fadeBackgroundSprite_ = std::make_unique<MagEngine::Sprite>();
		fadeBackgroundSprite_->Initialize(spriteSetup_, "white1x1.dds");
		fadeBackgroundSprite_->SetSize({screenWidth_, screenHeight_});
		fadeBackgroundSprite_->SetAnchorPoint({0.5f, 0.5f});
		fadeBackgroundSprite_->SetColor(fadeBackgroundColor_);
	}
}

//=============================================================================
// Finalize
void GameOverAnimation::Finalize() {
	textSprite_.reset();
	fadeBackgroundSprite_.reset();
	glowSprite_.reset();
	borderSprite1_.reset();
	borderSprite2_.reset();
	particles_.clear();
}

//=============================================================================
// Update
void GameOverAnimation::Update() {
	if (state_ == GameOverAnimationState::Idle || state_ == GameOverAnimationState::Done) {
		return;
	}

	elapsedTime_ += 1.0f / 60.0f; // 60FPS fixed

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

	UpdateParticles();

	// Update sprites
	if (textSprite_) {
		textSprite_->Update();
	}
	if (glowSprite_) {
		glowSprite_->Update();
	}
	if (borderSprite1_) {
		borderSprite1_->Update();
	}
	if (borderSprite2_) {
		borderSprite2_->Update();
	}
	if (fadeBackgroundSprite_) {
		fadeBackgroundSprite_->Update();
	}
}

//=============================================================================
// Update text appearing (explosion & slide-in)
void GameOverAnimation::UpdateAppearing() {
	progress_ = (elapsedTime_ / appearDuration_ < 1.0f) ? (elapsedTime_ / appearDuration_) : 1.0f;

	// EaseOutBounce effect for scale change
	float scale = EaseOutBounce(progress_) * 0.5f + 0.5f; // 0.5 ~ 1.0

	// Display text
	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = EaseOut(progress_); // Fade in alpha
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
	}

	// Glow effect
	if (glowSprite_) {
		float glowScale = (1.0f - progress_) * 1.5f + 0.8f;
		MagMath::Vector4 glowColor = {1.0f, 0.5f, 0.0f, (1.0f - progress_) * 0.3f};
		glowSprite_->SetColor(glowColor);
		glowSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		glowSprite_->SetSize({textSize_.x * 1.3f * glowScale, textSize_.y * 1.3f * glowScale});
	}

	// Border effect (expanding from top to bottom)
	if (borderSprite1_) {
		float borderAlpha = EaseOut(progress_);
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, borderAlpha * 0.8f};
		borderSprite1_->SetColor(borderColor);
		borderSprite1_->SetPosition({
			screenWidth_ / 2.0f,
			(screenHeight_ / 2.0f - 150.0f) * progress_
		});
	}

	if (borderSprite2_) {
		float borderAlpha = EaseOut(progress_);
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, borderAlpha * 0.8f};
		borderSprite2_->SetColor(borderColor);
		borderSprite2_->SetPosition({
			screenWidth_ / 2.0f,
			screenHeight_ - (screenHeight_ / 2.0f - 150.0f) * progress_
		});
	}

	// Generate particles
	if (progress_ < 0.5f) {
		GenerateParticles();
	}

	// Appearing complete
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Displaying;
		elapsedTime_ = 0.0f;
	}
}

//=============================================================================
// Update text displaying (pulsing effect)
void GameOverAnimation::UpdateDisplaying() {
	progress_ = (elapsedTime_ / displayDuration_ < 1.0f) ? (elapsedTime_ / displayDuration_) : 1.0f;

	// Pulsing effect (periodic scale change)
	float pulse = std::sin(elapsedTime_ * 3.0f) * 0.05f + 1.0f; // 0.95 ~ 1.05

	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = 1.0f;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * pulse, textSize_.y * pulse});
	}

	// Glow effect (always slightly glowing)
	if (glowSprite_) {
		float glowIntensity = 0.2f + std::sin(elapsedTime_ * 2.0f) * 0.1f;
		MagMath::Vector4 glowColor = {1.0f, 0.5f, 0.0f, glowIntensity};
		glowSprite_->SetColor(glowColor);
		glowSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		glowSprite_->SetSize({textSize_.x * 1.3f, textSize_.y * 1.3f});
	}

	// Borders always displayed
	if (borderSprite1_) {
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, 0.8f};
		borderSprite1_->SetColor(borderColor);
		borderSprite1_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f - 150.0f});
	}

	if (borderSprite2_) {
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, 0.8f};
		borderSprite2_->SetColor(borderColor);
		borderSprite2_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f + 150.0f});
	}

	// Displaying complete
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Fading;
		elapsedTime_ = 0.0f;
	}
}

//=============================================================================
// Update fading out
void GameOverAnimation::UpdateFading() {
	progress_ = (elapsedTime_ / fadeDuration_ < 1.0f) ? (elapsedTime_ / fadeDuration_) : 1.0f;

	// Fade out text
	float alpha = 1.0f - EaseOut(progress_);

	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = alpha;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}

	if (glowSprite_) {
		MagMath::Vector4 glowColor = {1.0f, 0.5f, 0.0f, alpha * 0.2f};
		glowSprite_->SetColor(glowColor);
		glowSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	}

	// Borders also fade out
	if (borderSprite1_) {
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, alpha * 0.8f};
		borderSprite1_->SetColor(borderColor);
		borderSprite1_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f - 150.0f});
	}

	if (borderSprite2_) {
		MagMath::Vector4 borderColor = {1.0f, 0.3f, 0.0f, alpha * 0.8f};
		borderSprite2_->SetColor(borderColor);
		borderSprite2_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f + 150.0f});
	}

	if (fadeBackgroundSprite_) {
		MagMath::Vector4 bgColor = fadeBackgroundColor_;
		bgColor.w = alpha * fadeBackgroundColor_.w;
		fadeBackgroundSprite_->SetColor(bgColor);
		fadeBackgroundSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		fadeBackgroundSprite_->SetSize({screenWidth_, screenHeight_});
	}

	// Fading complete
	if (progress_ >= 1.0f) {
		state_ = GameOverAnimationState::Done;
		elapsedTime_ = 0.0f;
		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

//=============================================================================
// Draw
void GameOverAnimation::Draw() {
	if (state_ == GameOverAnimationState::Idle || state_ == GameOverAnimationState::Done) {
		return;
	}

	// Draw background fade
	if (fadeBackgroundSprite_) {
		fadeBackgroundSprite_->Draw();
	}

	// Draw borders
	if (borderSprite1_) {
		borderSprite1_->Draw();
	}
	if (borderSprite2_) {
		borderSprite2_->Draw();
	}

	// Draw glow
	if (glowSprite_) {
		glowSprite_->Draw();
	}

	// Draw main text
	if (textSprite_) {
		textSprite_->Draw();
	}

	// Draw particles
	for (auto &p : particles_) {
		if (p.active) {
			// Particle rendering (simplified - just a small quad)
			// In a real implementation, you'd render actual particle sprites
		}
	}
}

//=============================================================================
// ImGui drawing (for debugging)
void GameOverAnimation::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("GameOverAnimation Debug");
	ImGui::Text("State: %d", static_cast<int>(state_));
	ImGui::Text("Elapsed Time: %.2f", elapsedTime_);
	ImGui::Text("Progress: %.2f%%", progress_ * 100.0f);
	ImGui::SliderFloat("Text Color R", &textColor_.x, 0.0f, 1.0f);
	ImGui::SliderFloat("Text Color G", &textColor_.y, 0.0f, 1.0f);
	ImGui::SliderFloat("Text Color B", &textColor_.z, 0.0f, 1.0f);
	ImGui::End();
#endif
}

//=============================================================================
// Start game over animation
void GameOverAnimation::StartGameOverAnimation(
	float appearDuration,
	float displayDuration,
	float fadeDuration) {
	state_ = GameOverAnimationState::Appearing;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	appearDuration_ = appearDuration;
	displayDuration_ = displayDuration;
	fadeDuration_ = fadeDuration;
}

//=============================================================================
// Reset
void GameOverAnimation::Reset() {
	state_ = GameOverAnimationState::Idle;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	particles_.clear();
	particles_.resize(kMaxParticles);
	for (auto &p : particles_) {
		p.active = false;
	}
}

//=============================================================================
// Update particles
void GameOverAnimation::UpdateParticles() {
	const float deltaTime = 1.0f / 60.0f;
	for (auto &p : particles_) {
		if (p.active) {
			p.lifetime -= deltaTime;
			if (p.lifetime <= 0.0f) {
				p.active = false;
			} else {
				p.position.x += p.velocity.x * deltaTime;
				p.position.y += p.velocity.y * deltaTime;
				p.velocity.y -= 9.8f * deltaTime; // Gravity
				p.scale -= 0.02f;
				p.color.w = p.lifetime / p.maxLifetime;
			}
		}
	}
}

//=============================================================================
// Generate particles
void GameOverAnimation::GenerateParticles() {
	for (int i = 0; i < kMaxParticles; ++i) {
		if (!particles_[i].active) {
			particles_[i].position = {screenWidth_ / 2.0f, screenHeight_ / 2.0f};
			float angle = (static_cast<float>(i) / kMaxParticles) * 6.28f; // 2*PI
			float speed = 200.0f + (std::rand() % 100);
			particles_[i].velocity = {
				std::cos(angle) * speed,
				std::sin(angle) * speed
			};
			particles_[i].lifetime = 1.0f;
			particles_[i].maxLifetime = 1.0f;
			particles_[i].scale = 5.0f;
			particles_[i].color = {1.0f, 0.5f, 0.0f, 1.0f};
			particles_[i].active = true;
			break;
		}
	}
}

//=============================================================================
// Easing functions
float GameOverAnimation::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float GameOverAnimation::EaseOut(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float GameOverAnimation::EaseIn(float t) {
	return t * t;
}

float GameOverAnimation::EaseOutBounce(float t) {
	float n1 = 7.5625f;
	float d1 = 2.75f;

	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		t -= 1.5f / d1;
		return n1 * t * t + 0.75f;
	} else if (t < 2.5f / d1) {
		t -= 2.25f / d1;
		return n1 * t * t + 0.9375f;
	} else {
		t -= 2.625f / d1;
		return n1 * t * t + 0.984375f;
	}
}

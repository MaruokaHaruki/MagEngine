#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GameOverUI.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

///=============================================================================
// Initialize
void GameOverUI::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = GameOverState::Idle;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// Initialize particles
	particles_.resize(kMaxParticles);
	for (auto &p : particles_) {
		p.active = false;
	}

	InitializeSprites();
}

///=============================================================================
// Initialize sprites
void GameOverUI::InitializeSprites() {
	if (!spriteSetup_)
		return;

	// Get screen size
	screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
	screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());

	// === Main text sprite (GAME OVER) ===
	textSprite_ = std::make_unique<MagEngine::Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetSize({textSize_.x, textSize_.y});
	textSprite_->SetAnchorPoint({0.5f, 0.5f});
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	textSprite_->SetColor(textColor_);

	// === Glow effect sprite ===
	glowSprite_ = std::make_unique<MagEngine::Sprite>();
	glowSprite_->Initialize(spriteSetup_, "white1x1.dds");
	glowSprite_->SetSize({textSize_.x * 1.2f, textSize_.y * 1.2f});
	glowSprite_->SetAnchorPoint({0.5f, 0.5f});
	glowSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	glowSprite_->SetColor({1.0f, 0.5f, 0.0f, 0.0f}); // Orange glow

	// === Top border ===
	borderSprite1_ = std::make_unique<MagEngine::Sprite>();
	borderSprite1_->Initialize(spriteSetup_, "white1x1.dds");
	borderSprite1_->SetSize({screenWidth_, 8.0f});
	borderSprite1_->SetAnchorPoint({0.5f, 0.5f});
	borderSprite1_->SetPosition({screenWidth_ / 2.0f, 60.0f});
	borderSprite1_->SetColor({1.0f, 0.3f, 0.0f, 0.0f}); // Orange

	// === Bottom border ===
	borderSprite2_ = std::make_unique<MagEngine::Sprite>();
	borderSprite2_->Initialize(spriteSetup_, "white1x1.dds");
	borderSprite2_->SetSize({screenWidth_, 8.0f});
	borderSprite2_->SetAnchorPoint({0.5f, 0.5f});
	borderSprite2_->SetPosition({screenWidth_ / 2.0f, screenHeight_ - 60.0f});
	borderSprite2_->SetColor({1.0f, 0.3f, 0.0f, 0.0f}); // Orange

	// === Background fade ===
	fadeBackgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	fadeBackgroundSprite_->Initialize(spriteSetup_, "white1x1.dds");
	fadeBackgroundSprite_->SetSize({screenWidth_, screenHeight_});
	fadeBackgroundSprite_->SetAnchorPoint({0.5f, 0.5f});
	fadeBackgroundSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	fadeBackgroundSprite_->SetColor(fadeBackgroundColor_);
}

///=============================================================================
// Finalize
void GameOverUI::Finalize() {
	textSprite_.reset();
	fadeBackgroundSprite_.reset();
	glowSprite_.reset();
	borderSprite1_.reset();
	borderSprite2_.reset();
	particles_.clear();
}

///=============================================================================
// Update
void GameOverUI::Update() {
	if (state_ == GameOverState::Idle || state_ == GameOverState::Done) {
		return;
	}

	elapsedTime_ += 1.0f / 60.0f; // 60FPS fixed

	switch (state_) {
	case GameOverState::Appearing:
		UpdateAppearing();
		break;
	case GameOverState::Displaying:
		UpdateDisplaying();
		break;
	case GameOverState::Fading:
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

///=============================================================================
// Update text appearing
void GameOverUI::UpdateAppearing() {
	progress_ = (elapsedTime_ / appearDuration_ < 1.0f) ? (elapsedTime_ / appearDuration_) : 1.0f;

	// EaseOutBounce effect
	float scale = EaseOutBounce(progress_) * 0.5f + 0.5f; // 0.5 ~ 1.0

	// Display text
	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = EaseOut(progress_);
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
		state_ = GameOverState::Displaying;
		elapsedTime_ = 0.0f;
	}
}

///=============================================================================
// Update text displaying
void GameOverUI::UpdateDisplaying() {
	progress_ = (elapsedTime_ / displayDuration_ < 1.0f) ? (elapsedTime_ / displayDuration_) : 1.0f;

	// Pulsing effect
	float pulse = std::sin(elapsedTime_ * 3.0f) * 0.05f + 1.0f; // 0.95 ~ 1.05

	if (textSprite_) {
		MagMath::Vector4 color = textColor_;
		color.w = 1.0f;
		textSprite_->SetColor(color);
		textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
		textSprite_->SetSize({textSize_.x * pulse, textSize_.y * pulse});
	}

	// Glow effect
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
		state_ = GameOverState::Fading;
		elapsedTime_ = 0.0f;
	}
}

///=============================================================================
// Update fading out
void GameOverUI::UpdateFading() {
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
		state_ = GameOverState::Done;
		elapsedTime_ = 0.0f;
		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

///=============================================================================
// Draw
void GameOverUI::Draw() {
	if (state_ == GameOverState::Idle || state_ == GameOverState::Done) {
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
}

///=============================================================================
// Update particles
void GameOverUI::UpdateParticles() {
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

///=============================================================================
// Generate particles
void GameOverUI::GenerateParticles() {
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

///=============================================================================
// Animation control
void GameOverUI::Play(float appearDuration, float displayDuration, float fadeDuration) {
	state_ = GameOverState::Appearing;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	appearDuration_ = appearDuration;
	displayDuration_ = displayDuration;
	fadeDuration_ = fadeDuration;
}

void GameOverUI::Stop() {
	state_ = GameOverState::Idle;
	ResetAnimation();
}

///=============================================================================
// Reset animation
void GameOverUI::ResetAnimation() {
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;
	particles_.clear();
	particles_.resize(kMaxParticles);
	for (auto &p : particles_) {
		p.active = false;
	}
}

///=============================================================================
// Easing functions
float GameOverUI::EaseInOut(float t) const {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float GameOverUI::EaseOut(float t) const {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float GameOverUI::EaseIn(float t) const {
	return t * t;
}

float GameOverUI::EaseOutBounce(float t) const {
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

///=============================================================================
// ImGui drawing
void GameOverUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Game Over UI");

	const char *stateNames[] = {"Idle", "Appearing", "Displaying", "Fading", "Done"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);
	ImGui::Text("Elapsed Time: %.2f", elapsedTime_);

	ImGui::Separator();

	ImGui::SliderFloat("Appear Duration", &appearDuration_, 0.1f, 2.0f);
	ImGui::SliderFloat("Display Duration", &displayDuration_, 0.5f, 5.0f);
	ImGui::SliderFloat("Fade Duration", &fadeDuration_, 0.5f, 3.0f);

	ImGui::ColorEdit4("Text Color", &textColor_.x);
	ImGui::InputFloat2("Text Size", &textSize_.x);
	ImGui::ColorEdit4("Background Color", &fadeBackgroundColor_.x);

	ImGui::Separator();

	if (ImGui::Button("Play")) {
		Play(appearDuration_, displayDuration_, fadeDuration_);
	}

	ImGui::SameLine();

	if (ImGui::Button("Stop")) {
		Stop();
	}

	ImGui::End();
#endif
}

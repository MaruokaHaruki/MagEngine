#define _USE_MATH_DEFINES
#define NOMINMAX
#include "MenuUI.h"
#include "ImguiSetup.h"
#include "Input.h"
#include <Xinput.h>
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void MenuUI::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// 背景パネルの作成（半透明の暗い背景）
	backgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.png");
	backgroundSprite_->SetSize({screenWidth_, screenHeight_});
	backgroundSprite_->SetPosition({screenWidth_ * 0.5f, screenHeight_ * 0.5f});
	backgroundSprite_->SetAnchorPoint({0.5f, 0.5f});
	backgroundSprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});

	// タイトルスプライトの作成
	titleSprite_ = std::make_unique<MagEngine::Sprite>();
	titleSprite_->Initialize(spriteSetup_, "WolfOne_Pause.png");
	titleSprite_->SetSize({600.0f, 80.0f});
	titleSprite_->SetPosition({screenWidth_ * 0.5f, screenHeight_ * 0.15f});
	titleSprite_->SetAnchorPoint({0.5f, 0.5f});
	titleSprite_->SetColor({0.1f, 0.3f, 0.6f, 0.0f});

	// ボタンの初期化
	InitializeButtons();
}

///=============================================================================
///                        ボタンの初期化
void MenuUI::InitializeButtons() {
	//========================================
	// メニュー中央配置設定
	float centerX = screenWidth_ * 0.5f;
	float centerY = screenHeight_ * 0.5f;
	float buttonWidth = 300.0f;
	float buttonHeight = 50.0f;
	float spacing = 80.0f; // 縦方向の間隔

	//========================================
	// ゲームに戻る（最上部）
	auto &resumeButton = buttons_[MenuButton::ResumeGame];
	resumeButton.sprite = std::make_unique<MagEngine::Sprite>();
	resumeButton.sprite->Initialize(spriteSetup_, "white1x1.png");
	resumeButton.basePosition = {centerX, centerY - spacing};
	resumeButton.baseSize = {buttonWidth, buttonHeight};
	resumeButton.normalColor = {0.2f, 0.5f, 0.9f, 0.0f};
	resumeButton.highlightColor = {0.3f, 0.8f, 1.0f, 0.0f};
	resumeButton.currentScale = 1.0f;
	resumeButton.targetScale = 1.0f;
	resumeButton.isSelected = true;
	resumeButton.isPressed = false;
	resumeButton.pulseTime = 0.0f;
	resumeButton.labelText = "Resume Game";
	resumeButton.sprite->SetAnchorPoint({0.5f, 0.5f});
	resumeButton.sprite->SetPosition(resumeButton.basePosition);
	resumeButton.sprite->SetSize(resumeButton.baseSize);
	resumeButton.sprite->SetColor(resumeButton.normalColor);

	//========================================
	// テキストスプライトの初期化
	resumeButton.textSprite = std::make_unique<MagEngine::Sprite>();
	resumeButton.textSprite->Initialize(spriteSetup_, "WolfOne_Resume.png");
	resumeButton.textPosition = resumeButton.basePosition;
	resumeButton.textSize = {200.0f, 30.0f};
	resumeButton.textAlpha = 0.0f;
	resumeButton.textTargetAlpha = 0.0f;
	resumeButton.textSprite->SetAnchorPoint({0.5f, 0.5f});
	resumeButton.textSprite->SetSize(resumeButton.textSize);
	resumeButton.textSprite->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	//========================================
	// === 操作説明（中央）===
	auto &operationButton = buttons_[MenuButton::OperationGuide];
	operationButton.sprite = std::make_unique<MagEngine::Sprite>();
	operationButton.sprite->Initialize(spriteSetup_, "white1x1.png");
	operationButton.basePosition = {centerX, centerY};
	operationButton.baseSize = {buttonWidth, buttonHeight};
	operationButton.normalColor = {0.3f, 0.6f, 0.9f, 0.0f};
	operationButton.highlightColor = {0.5f, 0.9f, 1.0f, 0.0f};
	operationButton.currentScale = 1.0f;
	operationButton.targetScale = 1.0f;
	operationButton.isSelected = false;
	operationButton.isPressed = false;
	operationButton.pulseTime = 0.0f;
	operationButton.labelText = "Operation Guide";
	operationButton.sprite->SetAnchorPoint({0.5f, 0.5f});
	operationButton.sprite->SetPosition(operationButton.basePosition);
	operationButton.sprite->SetSize(operationButton.baseSize);
	operationButton.sprite->SetColor(operationButton.normalColor);

	// テキストスプライトの初期化
	operationButton.textSprite = std::make_unique<MagEngine::Sprite>();
	operationButton.textSprite->Initialize(spriteSetup_, "WolfOne_Controls.png");
	operationButton.textPosition = operationButton.basePosition;
	operationButton.textSize = {200.0f, 30.0f};
	operationButton.textAlpha = 0.0f;
	operationButton.textTargetAlpha = 0.0f;
	operationButton.textSprite->SetAnchorPoint({0.5f, 0.5f});
	operationButton.textSprite->SetSize(operationButton.textSize);
	operationButton.textSprite->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	// === タイトルに戻る（最下部）===
	auto &returnButton = buttons_[MenuButton::ReturnToTitle];
	returnButton.sprite = std::make_unique<MagEngine::Sprite>();
	returnButton.sprite->Initialize(spriteSetup_, "white1x1.png");
	returnButton.basePosition = {centerX, centerY + spacing};
	returnButton.baseSize = {buttonWidth, buttonHeight};
	returnButton.normalColor = {0.8f, 0.2f, 0.2f, 0.0f};
	returnButton.highlightColor = {1.0f, 0.4f, 0.4f, 0.0f};
	returnButton.currentScale = 1.0f;
	returnButton.targetScale = 1.0f;
	returnButton.isSelected = false;
	returnButton.isPressed = false;
	returnButton.pulseTime = 0.0f;
	returnButton.labelText = "Return to Title";
	returnButton.sprite->SetAnchorPoint({0.5f, 0.5f});
	returnButton.sprite->SetPosition(returnButton.basePosition);
	returnButton.sprite->SetSize(returnButton.baseSize);
	returnButton.sprite->SetColor(returnButton.normalColor);

	// テキストスプライトの初期化
	returnButton.textSprite = std::make_unique<MagEngine::Sprite>();
	returnButton.textSprite->Initialize(spriteSetup_, "WolfOne_ReturntoTitle.png");
	returnButton.textPosition = returnButton.basePosition;
	returnButton.textSize = {200.0f, 30.0f};
	returnButton.textAlpha = 0.0f;
	returnButton.textTargetAlpha = 0.0f;
	returnButton.textSprite->SetAnchorPoint({0.5f, 0.5f});
	returnButton.textSprite->SetSize(returnButton.textSize);
	returnButton.textSprite->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
}

///=============================================================================
///                        終了処理
void MenuUI::Finalize() {
	backgroundSprite_.reset();
	titleSprite_.reset();
	buttons_.clear();
}

///=============================================================================
///                        更新
void MenuUI::Update() {
	const float deltaTime = 1.0f / 60.0f; // 60FPS想定

	Input *input = Input::GetInstance();

	// ESCキーまたはパッドのメニューボタンでメニューを開く・閉じる（トリガーで一回だけ）
	if (input->TriggerKey(DIK_ESCAPE) || input->TriggerButton(XINPUT_GAMEPAD_START)) {
		if (isOpen_) {
			Close();
		} else {
			Open();
		}
	}

	// フェード制御
	if (isOpen_) {
		targetFadeAlpha_ = 0.7f;
	} else {
		targetFadeAlpha_ = 0.0f;
	}

	fadeAlpha_ += (targetFadeAlpha_ - fadeAlpha_) * fadeSpeed_ * deltaTime;

	// メニューが閉じている場合は更新をスキップ
	if (fadeAlpha_ < 0.01f) {
		return;
	}

	// ボタン選択の更新
	UpdateButtonSelection();

	// アニメーションの更新
	UpdateButtonAnimations(deltaTime);

	// スプライト更新
	if (backgroundSprite_) {
		backgroundSprite_->SetColor({0.0f, 0.0f, 0.0f, fadeAlpha_ * 0.6f});
		backgroundSprite_->Update();
	}

	if (titleSprite_) {
		titleSprite_->SetColor({0.1f, 0.3f, 0.6f, fadeAlpha_ * 0.9f});
		titleSprite_->Update();
	}

	for (auto &[button, info] : buttons_) {
		if (info.sprite) {
			info.sprite->Update();
		}
		if (info.textSprite) {
			info.textSprite->Update();
		}
	}
}

///=============================================================================
///                        ボタン選択の更新
void MenuUI::UpdateButtonSelection() {
	if (fadeAlpha_ < 0.5f) {
		return;
	}

	Input *input = Input::GetInstance();

	// 上下キーでボタン選択
	static float inputCooldown = 0.0f;
	inputCooldown -= 1.0f / 60.0f;

	float stickY = input->GetLeftStickY();
	bool moveUp = (stickY > 0.5f);
	bool moveDown = (stickY < -0.5f);

	// D-Padの上下も対応
	moveUp = moveUp || input->PushButton(XINPUT_GAMEPAD_DPAD_UP);
	moveDown = moveDown || input->PushButton(XINPUT_GAMEPAD_DPAD_DOWN);

	if (inputCooldown < 0.0f) {
		if (moveUp) {
			selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(MenuButton::Max)) % static_cast<int>(MenuButton::Max);
			inputCooldown = 0.2f; // 200ms のクールダウン
		} else if (moveDown) {
			selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(MenuButton::Max);
			inputCooldown = 0.2f;
		}
	}

	// ボタンの選択状態を更新
	for (int i = 0; i < static_cast<int>(MenuButton::Max); ++i) {
		buttons_[static_cast<MenuButton>(i)].isSelected = (i == selectedIndex_);
	}

	// Aボタンで決定（トリガーで一回だけ）
	if (input->TriggerButton(XINPUT_GAMEPAD_A)) {
		isButtonPressed_ = true;
	}
}

///=============================================================================
///                        ボタンアニメーションの更新
void MenuUI::UpdateButtonAnimations(float deltaTime) {
	// グロー効果の更新
	glowIntensity_ += deltaTime * glowPulseSpeed_;

	for (auto &[button, info] : buttons_) {
		// ターゲットスケールの設定
		if (info.isSelected) {
			info.targetScale = selectedScale_;
			info.textTargetAlpha = 1.0f;
			info.pulseTime += deltaTime * 10.0f;
		} else {
			info.targetScale = normalScale_;
			info.textTargetAlpha = 0.6f;
			info.pulseTime = 0.0f;
		}

		// スケールの補間
		float lerpSpeed = info.isSelected ? selectAnimationSpeed_ : pressAnimationSpeed_;
		info.currentScale += (info.targetScale - info.currentScale) * lerpSpeed * deltaTime;

		// スプライトに適用
		if (info.sprite) {
			// サイズの更新
			Vector2 scaledSize = {
				info.baseSize.x * info.currentScale,
				info.baseSize.y * info.currentScale};
			info.sprite->SetSize(scaledSize);

			// 位置の更新
			info.sprite->SetPosition(info.basePosition);

			// 色の更新
			Vector4 currentColor;
			if (info.isSelected) {
				// 選択中はハイライトカラーでパルス効果
				float pulse = 0.7f + 0.3f * std::abs(std::sin(info.pulseTime));
				float glow = 1.0f + 0.3f * std::abs(std::sin(glowIntensity_));
				currentColor = {
					info.highlightColor.x * pulse * glow * fadeAlpha_,
					info.highlightColor.y * pulse * glow * fadeAlpha_,
					info.highlightColor.z * pulse * glow * fadeAlpha_,
					fadeAlpha_ * 0.8f};
			} else {
				// 未選択は通常カラー
				currentColor = {
					info.normalColor.x * fadeAlpha_,
					info.normalColor.y * fadeAlpha_,
					info.normalColor.z * fadeAlpha_,
					fadeAlpha_ * 0.6f};
			}
			info.sprite->SetColor(currentColor);
		}

		// テキストスプライトの更新
		if (info.textSprite) {
			info.textAlpha += (info.textTargetAlpha - info.textAlpha) * 0.1f;

			// テキスト位置の更新
			info.textSprite->SetPosition(info.textPosition);

			// テキスト色の更新
			Vector4 textColor;
			if (info.isSelected) {
				// 選択中は明るく表示
				textColor = {1.0f, 1.0f, 1.0f, info.textAlpha * fadeAlpha_};
			} else {
				textColor = {0.8f, 0.8f, 0.8f, info.textAlpha * fadeAlpha_};
			}
			info.textSprite->SetColor(textColor);
		}
	}
}

///=============================================================================
///                        描画
void MenuUI::Draw() {
	if (fadeAlpha_ < 0.01f) {
		return;
	}

	// 背景パネルの描画
	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}

	// タイトルの描画
	if (titleSprite_) {
		titleSprite_->Draw();
	}

	// ボタンの描画
	for (auto &[button, info] : buttons_) {
		if (info.sprite) {
			info.sprite->Draw();
		}
		// テキストラベルの描画
		if (info.textSprite) {
			info.textSprite->Draw();
		}
	}
}

///=============================================================================
///                        イージング関数
float MenuUI::EaseOutElastic(float t) {
	const float c4 = (2.0f * static_cast<float>(M_PI)) / 3.0f;
	return t == 0.0f   ? 0.0f
		   : t == 1.0f ? 1.0f
					   : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float MenuUI::EaseInOutQuad(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

///=============================================================================
///                        ImGui描画
void MenuUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Menu UI");

	ImGui::Text("Menu Open: %s", isOpen_ ? "YES" : "NO");
	ImGui::Text("Selected Index: %d", selectedIndex_);
	ImGui::Text("Fade Alpha: %.2f", fadeAlpha_);
	ImGui::Text("Button Pressed: %s", isButtonPressed_ ? "YES" : "NO");

	ImGui::Separator();

	ImGui::SliderFloat("Fade Speed", &fadeSpeed_, 0.1f, 10.0f);
	ImGui::SliderFloat("Select Scale", &selectedScale_, 1.0f, 2.0f);
	ImGui::SliderFloat("Select Animation Speed", &selectAnimationSpeed_, 1.0f, 20.0f);

	ImGui::Separator();

	ImGui::Text("Button Info:");
	for (const auto &[button, info] : buttons_) {
		ImGui::Text("%s: %s (Scale: %.2f)",
					info.labelText.c_str(),
					info.isSelected ? "SELECTED" : "---",
					info.currentScale);
	}

	ImGui::End();
#endif
}

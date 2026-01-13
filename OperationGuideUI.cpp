#define _USE_MATH_DEFINES
#define NOMINMAX
#include "OperationGuideUI.h"
#include "ImguiSetup.h"
#include <Xinput.h>
#include <algorithm>
#include <cmath>
#include "Input.h"
using namespace MagEngine;

///=============================================================================
///                        初期化
void OperationGuideUI::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// 背景パネルの作成
	backgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.png");
	backgroundSprite_->SetSize({400.0f, 300.0f});
	backgroundSprite_->SetPosition(guideBasePosition_);
	backgroundSprite_->SetColor({0.1f, 0.1f, 0.15f, opacity_ * 0.7f}); // 半透明の暗い背景

	// ボタンの初期化
	InitializeButtons();
}

///=============================================================================
///                        ボタンの初期化
void OperationGuideUI::InitializeButtons() {
	// ベースカラー設定
	Vector4 normalColor = {0.7f, 0.7f, 0.7f, opacity_};
	Vector4 pressedColor = {0.2f, 1.0f, 0.3f, opacity_}; // 緑色のハイライト

	// レイアウト設定（背景パネル内での相対位置）
	float baseX = guideBasePosition_.x + 30.0f;
	float baseY = guideBasePosition_.y + 30.0f;
	float buttonSize = 40.0f;
	float spacing = 55.0f;

	// === 左スティック ===
	auto &leftStick = buttons_[ControllerButton::LeftStick];
	leftStick.sprite = std::make_unique<MagEngine::Sprite>();
	leftStick.sprite->Initialize(spriteSetup_, "white1x1.png");
	leftStick.basePosition = {baseX, baseY};
	leftStick.baseSize = {buttonSize, buttonSize};
	leftStick.normalColor = normalColor;
	leftStick.pressedColor = pressedColor;
	leftStick.currentScale = 1.0f;
	leftStick.targetScale = 1.0f;
	leftStick.isPressed = false;
	leftStick.pulseTime = 0.0f;
	leftStick.labelText = "L-Stick: Move";
	leftStick.sprite->SetAnchorPoint({0.5f, 0.5f});
	leftStick.sprite->SetPosition(leftStick.basePosition);
	leftStick.sprite->SetSize(leftStick.baseSize);
	leftStick.sprite->SetColor(leftStick.normalColor);

	// === Aボタン（ブースト/バレルロール） ===
	auto &buttonA = buttons_[ControllerButton::ButtonA];
	buttonA.sprite = std::make_unique<MagEngine::Sprite>();
	buttonA.sprite->Initialize(spriteSetup_, "white1x1.png");
	buttonA.basePosition = {baseX + spacing * 3, baseY};
	buttonA.baseSize = {buttonSize, buttonSize};
	buttonA.normalColor = {0.2f, 0.8f, 0.3f, opacity_}; // 緑色
	buttonA.pressedColor = {0.3f, 1.0f, 0.4f, opacity_};
	buttonA.currentScale = 1.0f;
	buttonA.targetScale = 1.0f;
	buttonA.isPressed = false;
	buttonA.pulseTime = 0.0f;
	buttonA.labelText = "A: Boost/Roll";
	buttonA.sprite->SetAnchorPoint({0.5f, 0.5f});
	buttonA.sprite->SetPosition(buttonA.basePosition);
	buttonA.sprite->SetSize(buttonA.baseSize);
	buttonA.sprite->SetColor(buttonA.normalColor);

	// === Bボタン（ミサイル） ===
	auto &buttonB = buttons_[ControllerButton::ButtonB];
	buttonB.sprite = std::make_unique<MagEngine::Sprite>();
	buttonB.sprite->Initialize(spriteSetup_, "white1x1.png");
	buttonB.basePosition = {baseX + spacing * 4, baseY};
	buttonB.baseSize = {buttonSize, buttonSize};
	buttonB.normalColor = {0.9f, 0.2f, 0.2f, opacity_}; // 赤色
	buttonB.pressedColor = {1.0f, 0.3f, 0.3f, opacity_};
	buttonB.currentScale = 1.0f;
	buttonB.targetScale = 1.0f;
	buttonB.isPressed = false;
	buttonB.pulseTime = 0.0f;
	buttonB.labelText = "B: Missile";
	buttonB.sprite->SetAnchorPoint({0.5f, 0.5f});
	buttonB.sprite->SetPosition(buttonB.basePosition);
	buttonB.sprite->SetSize(buttonB.baseSize);
	buttonB.sprite->SetColor(buttonB.normalColor);

	// === Yボタン（ロックオン） ===
	auto &buttonY = buttons_[ControllerButton::ButtonY];
	buttonY.sprite = std::make_unique<MagEngine::Sprite>();
	buttonY.sprite->Initialize(spriteSetup_, "white1x1.png");
	buttonY.basePosition = {baseX + spacing * 5, baseY};
	buttonY.baseSize = {buttonSize, buttonSize};
	buttonY.normalColor = {0.9f, 0.9f, 0.2f, opacity_}; // 黄色
	buttonY.pressedColor = {1.0f, 1.0f, 0.3f, opacity_};
	buttonY.currentScale = 1.0f;
	buttonY.targetScale = 1.0f;
	buttonY.isPressed = false;
	buttonY.pulseTime = 0.0f;
	buttonY.labelText = "Y: Lock-On";
	buttonY.sprite->SetAnchorPoint({0.5f, 0.5f});
	buttonY.sprite->SetPosition(buttonY.basePosition);
	buttonY.sprite->SetSize(buttonY.baseSize);
	buttonY.sprite->SetColor(buttonY.normalColor);

	// === RTボタン（射撃） ===
	auto &buttonRT = buttons_[ControllerButton::RT];
	buttonRT.sprite = std::make_unique<MagEngine::Sprite>();
	buttonRT.sprite->Initialize(spriteSetup_, "white1x1.png");
	buttonRT.basePosition = {baseX + spacing * 6, baseY};
	buttonRT.baseSize = {buttonSize, buttonSize};
	buttonRT.normalColor = {0.8f, 0.5f, 0.2f, opacity_}; // オレンジ色
	buttonRT.pressedColor = {1.0f, 0.6f, 0.2f, opacity_};
	buttonRT.currentScale = 1.0f;
	buttonRT.targetScale = 1.0f;
	buttonRT.isPressed = false;
	buttonRT.pulseTime = 0.0f;
	buttonRT.labelText = "RT: Shoot";
	buttonRT.sprite->SetAnchorPoint({0.5f, 0.5f});
	buttonRT.sprite->SetPosition(buttonRT.basePosition);
	buttonRT.sprite->SetSize(buttonRT.baseSize);
	buttonRT.sprite->SetColor(buttonRT.normalColor);

	// === 方向キー（上下左右） ===
	float dpadBaseX = baseX + spacing * 1.5f;
	float dpadBaseY = baseY + spacing * 1.5f;
	float dpadSize = 30.0f;

	// 上
	auto &dpadUp = buttons_[ControllerButton::DPadUp];
	dpadUp.sprite = std::make_unique<MagEngine::Sprite>();
	dpadUp.sprite->Initialize(spriteSetup_, "white1x1.png");
	dpadUp.basePosition = {dpadBaseX, dpadBaseY - 35.0f};
	dpadUp.baseSize = {dpadSize, dpadSize};
	dpadUp.normalColor = normalColor;
	dpadUp.pressedColor = pressedColor;
	dpadUp.currentScale = 1.0f;
	dpadUp.targetScale = 1.0f;
	dpadUp.isPressed = false;
	dpadUp.pulseTime = 0.0f;
	dpadUp.labelText = "D-Pad";
	dpadUp.sprite->SetAnchorPoint({0.5f, 0.5f});
	dpadUp.sprite->SetPosition(dpadUp.basePosition);
	dpadUp.sprite->SetSize(dpadUp.baseSize);
	dpadUp.sprite->SetColor(dpadUp.normalColor);

	// 下
	auto &dpadDown = buttons_[ControllerButton::DPadDown];
	dpadDown.sprite = std::make_unique<MagEngine::Sprite>();
	dpadDown.sprite->Initialize(spriteSetup_, "white1x1.png");
	dpadDown.basePosition = {dpadBaseX, dpadBaseY + 35.0f};
	dpadDown.baseSize = {dpadSize, dpadSize};
	dpadDown.normalColor = normalColor;
	dpadDown.pressedColor = pressedColor;
	dpadDown.currentScale = 1.0f;
	dpadDown.targetScale = 1.0f;
	dpadDown.isPressed = false;
	dpadDown.pulseTime = 0.0f;
	dpadDown.sprite->SetAnchorPoint({0.5f, 0.5f});
	dpadDown.sprite->SetPosition(dpadDown.basePosition);
	dpadDown.sprite->SetSize(dpadDown.baseSize);
	dpadDown.sprite->SetColor(dpadDown.normalColor);

	// 左
	auto &dpadLeft = buttons_[ControllerButton::DPadLeft];
	dpadLeft.sprite = std::make_unique<MagEngine::Sprite>();
	dpadLeft.sprite->Initialize(spriteSetup_, "white1x1.png");
	dpadLeft.basePosition = {dpadBaseX - 35.0f, dpadBaseY};
	dpadLeft.baseSize = {dpadSize, dpadSize};
	dpadLeft.normalColor = normalColor;
	dpadLeft.pressedColor = pressedColor;
	dpadLeft.currentScale = 1.0f;
	dpadLeft.targetScale = 1.0f;
	dpadLeft.isPressed = false;
	dpadLeft.pulseTime = 0.0f;
	dpadLeft.sprite->SetAnchorPoint({0.5f, 0.5f});
	dpadLeft.sprite->SetPosition(dpadLeft.basePosition);
	dpadLeft.sprite->SetSize(dpadLeft.baseSize);
	dpadLeft.sprite->SetColor(dpadLeft.normalColor);

	// 右
	auto &dpadRight = buttons_[ControllerButton::DPadRight];
	dpadRight.sprite = std::make_unique<MagEngine::Sprite>();
	dpadRight.sprite->Initialize(spriteSetup_, "white1x1.png");
	dpadRight.basePosition = {dpadBaseX + 35.0f, dpadBaseY};
	dpadRight.baseSize = {dpadSize, dpadSize};
	dpadRight.normalColor = normalColor;
	dpadRight.pressedColor = pressedColor;
	dpadRight.currentScale = 1.0f;
	dpadRight.targetScale = 1.0f;
	dpadRight.isPressed = false;
	dpadRight.pulseTime = 0.0f;
	dpadRight.sprite->SetAnchorPoint({0.5f, 0.5f});
	dpadRight.sprite->SetPosition(dpadRight.basePosition);
	dpadRight.sprite->SetSize(dpadRight.baseSize);
	dpadRight.sprite->SetColor(dpadRight.normalColor);
}

///=============================================================================
///                        終了処理
void OperationGuideUI::Finalize() {
	backgroundSprite_.reset();
	buttons_.clear();
}

///=============================================================================
///                        更新
void OperationGuideUI::Update() {
	if (!isVisible_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f; // 60FPS想定

	// ボタン状態の更新
	UpdateButtonStates();

	// アニメーションの更新
	UpdateButtonAnimations(deltaTime);

	// スプライト更新
	if (backgroundSprite_) {
		backgroundSprite_->Update();
	}

	for (auto &[button, info] : buttons_) {
		if (info.sprite) {
			info.sprite->Update();
		}
	}
}

///=============================================================================
///                        ボタン状態の更新
void OperationGuideUI::UpdateButtonStates() {
	Input *input = Input::GetInstance();

	// 左スティック（移動入力があれば押されている判定）
	float stickX = input->GetLeftStickX();
	float stickY = input->GetLeftStickY();
	bool stickMoved = (std::abs(stickX) > 0.1f || std::abs(stickY) > 0.1f);
	buttons_[ControllerButton::LeftStick].isPressed = stickMoved;

	// Aボタン
	buttons_[ControllerButton::ButtonA].isPressed = input->PushButton(XINPUT_GAMEPAD_A);

	// Bボタン
	buttons_[ControllerButton::ButtonB].isPressed = input->PushButton(XINPUT_GAMEPAD_B);

	// Yボタン
	buttons_[ControllerButton::ButtonY].isPressed = input->PushButton(XINPUT_GAMEPAD_Y);

	// RTボタン（右トリガー）
	float rightTrigger = input->GetRightTrigger();
	buttons_[ControllerButton::RT].isPressed = (rightTrigger > 0.1f);

	// 方向キー
	buttons_[ControllerButton::DPadUp].isPressed = input->PushButton(XINPUT_GAMEPAD_DPAD_UP);
	buttons_[ControllerButton::DPadDown].isPressed = input->PushButton(XINPUT_GAMEPAD_DPAD_DOWN);
	buttons_[ControllerButton::DPadLeft].isPressed = input->PushButton(XINPUT_GAMEPAD_DPAD_LEFT);
	buttons_[ControllerButton::DPadRight].isPressed = input->PushButton(XINPUT_GAMEPAD_DPAD_RIGHT);
}

///=============================================================================
///                        ボタンアニメーションの更新
void OperationGuideUI::UpdateButtonAnimations(float deltaTime) {
	for (auto &[button, info] : buttons_) {
		// ターゲットスケールの設定
		if (info.isPressed) {
			info.targetScale = pressedScale_;
			info.pulseTime += deltaTime * 10.0f; // パルス効果用
		} else {
			info.targetScale = normalScale_;
			info.pulseTime = 0.0f;
		}

		// スケールの補間
		float lerpSpeed = info.isPressed ? pressAnimationSpeed_ : releaseAnimationSpeed_;
		info.currentScale += (info.targetScale - info.currentScale) * lerpSpeed * deltaTime;

		// スプライトに適用
		if (info.sprite) {
			// サイズの更新
			Vector2 scaledSize = {
				info.baseSize.x * info.currentScale,
				info.baseSize.y * info.currentScale};
			info.sprite->SetSize(scaledSize);

			// 色の更新（押されている時は押下色に変化）
			Vector4 currentColor;
			if (info.isPressed) {
				// パルス効果を追加
				float pulse = 0.8f + 0.2f * std::abs(sinf(info.pulseTime));
				currentColor = {
					info.pressedColor.x * pulse,
					info.pressedColor.y * pulse,
					info.pressedColor.z * pulse,
					info.pressedColor.w};
			} else {
				currentColor = info.normalColor;
			}
			info.sprite->SetColor(currentColor);
		}
	}
}

///=============================================================================
///                        描画
void OperationGuideUI::Draw() {
	if (!isVisible_) {
		return;
	}

	// 背景パネルの描画
	if (backgroundSprite_) {
		backgroundSprite_->Draw();
	}

	// ボタンの描画
	for (auto &[button, info] : buttons_) {
		if (info.sprite) {
			info.sprite->Draw();
		}
	}
}

///=============================================================================
///                        イージング関数
float OperationGuideUI::EaseOutElastic(float t) {
	const float c4 = (2.0f * static_cast<float>(M_PI)) / 3.0f;
	return t == 0.0f   ? 0.0f
		   : t == 1.0f ? 1.0f
					   : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float OperationGuideUI::EaseInOutQuad(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

///=============================================================================
///                        ImGui描画
void OperationGuideUI::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Operation Guide UI");

	// 表示設定
	ImGui::Checkbox("Visible", &isVisible_);
	ImGui::SliderFloat("Opacity", &opacity_, 0.0f, 1.0f);

	ImGui::Separator();

	// アニメーション設定
	ImGui::SliderFloat("Press Scale", &pressedScale_, 1.0f, 2.0f);
	ImGui::SliderFloat("Press Speed", &pressAnimationSpeed_, 1.0f, 20.0f);
	ImGui::SliderFloat("Release Speed", &releaseAnimationSpeed_, 1.0f, 20.0f);

	ImGui::Separator();

	// 位置設定
	ImGui::InputFloat2("Guide Position", &guideBasePosition_.x);

	ImGui::Separator();

	// ボタン状態表示
	ImGui::Text("Button States:");
	for (const auto &[button, info] : buttons_) {
		ImGui::Text("%s: %s",
					info.labelText.c_str(),
					info.isPressed ? "PRESSED" : "---");
	}

	ImGui::End();
#endif
}

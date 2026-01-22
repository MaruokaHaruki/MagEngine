#define _USE_MATH_DEFINES
#define NOMINMAX
#include "OperationGuideUI.h"
#include "ImguiSetup.h"
#include "Input.h"
#include <Xinput.h>
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void OperationGuideUI::Initialize(MagEngine::SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;

	// 画面サイズを取得
	if(spriteSetup_) {
		screenWidth_ = static_cast<float>( spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth() );
		screenHeight_ = static_cast<float>( spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight() );
	}

	// 画面左端に配置
	guideBasePosition_ = { 0.0f, screenHeight_ * 0.5f - 20.0f };

	// 背景パネルの作成（ミリタリー風の濃い背景）
	backgroundSprite_ = std::make_unique<MagEngine::Sprite>();
	backgroundSprite_->Initialize(spriteSetup_, "white1x1.png");
	backgroundSprite_->SetSize({ 256.0f, 420.0f }); // 縦長のパネル
	backgroundSprite_->SetPosition(guideBasePosition_);
	backgroundSprite_->SetColor({ 0.05f, 0.08f, 0.12f, opacity_ * 0.85f }); // ダークブルー系

	// ボタンの初期化
	InitializeButtons();
}

///=============================================================================
///                        ボタンの初期化
void OperationGuideUI::InitializeButtons() {
	// ベースカラー設定（HUD風のサイバーブルー系）
	Vector4 normalColor = { 0.2f, 0.5f, 0.8f, opacity_ * 0.7f };
	Vector4 pressedColor = { 0.1f, 0.9f, 1.0f, opacity_ }; // シアンのハイライト

	// レイアウト設定（縦配置、画面左端）
	float baseX = guideBasePosition_.x + 90.0f;
	float baseY = guideBasePosition_.y + 40.0f;
	float buttonSize = 50.0f;
	float spacing = 70.0f; // 縦方向の間隔

	// === 左スティック（最上部、大きめ） ===
	auto &leftStick = buttons_[ControllerButton::LeftStick];
	leftStick.sprite = std::make_unique<MagEngine::Sprite>();
	leftStick.sprite->Initialize(spriteSetup_, "xbox_ls.png");
	leftStick.basePosition = { baseX, baseY };
	leftStick.baseSize = { 60.0f, 60.0f }; // 大きめ
	leftStick.normalColor = { 0.15f, 0.4f, 0.7f, opacity_ * 0.8f };
	leftStick.pressedColor = { 0.2f, 0.8f, 1.0f, opacity_ };
	leftStick.currentScale = 1.0f;
	leftStick.targetScale = 1.0f;
	leftStick.isPressed = false;
	leftStick.pulseTime = 0.0f;
	leftStick.labelText = "L-Stick: Move";
	leftStick.textLabelPosition = { baseX + 50.0f, baseY }; // ボタンの右側
	leftStick.sprite->SetAnchorPoint({ 0.5f, 0.5f });
	leftStick.sprite->SetPosition(leftStick.basePosition);
	leftStick.sprite->SetSize(leftStick.baseSize);
	leftStick.sprite->SetColor(leftStick.normalColor);

	// テキストスプライトの初期化
	leftStick.textSprite = std::make_unique<MagEngine::Sprite>();
	leftStick.textSprite->Initialize(spriteSetup_, "WolfOne_ControlStick.png");
	leftStick.textBasePosition = { baseX + 60.0f, baseY };
	leftStick.textSize = { 80.0f, 20.0f };
	leftStick.textAlpha = 0.0f;
	leftStick.textTargetAlpha = 0.7f;
	leftStick.textSlideOffset = -20.0f;
	leftStick.textTargetOffset = 0.0f;
	leftStick.textSprite->SetAnchorPoint({ 0.0f, 0.5f });
	leftStick.textSprite->SetSize(leftStick.textSize);
	leftStick.textSprite->SetColor({ 0.8f, 0.9f, 1.0f, 0.0f });

	// === RTボタン（射撃）- 2番目 ===
	auto &buttonRT = buttons_[ControllerButton::RT];
	buttonRT.sprite = std::make_unique<MagEngine::Sprite>();
	buttonRT.sprite->Initialize(spriteSetup_, "xbox_rt.png");
	buttonRT.basePosition = { baseX, baseY + spacing };
	buttonRT.baseSize = { buttonSize, buttonSize };
	buttonRT.normalColor = { 0.9f, 0.3f, 0.1f, opacity_ * 0.8f }; // 攻撃的な赤
	buttonRT.pressedColor = { 1.0f, 0.5f, 0.0f, opacity_ };
	buttonRT.currentScale = 1.0f;
	buttonRT.targetScale = 1.0f;
	buttonRT.isPressed = false;
	buttonRT.pulseTime = 0.0f;
	buttonRT.labelText = "RT: Shoot";
	buttonRT.textLabelPosition = { baseX + 45.0f, baseY + spacing }; // ボタンの右側
	buttonRT.sprite->SetAnchorPoint({ 0.5f, 0.5f });
	buttonRT.sprite->SetPosition(buttonRT.basePosition);
	buttonRT.sprite->SetSize(buttonRT.baseSize);
	buttonRT.sprite->SetColor(buttonRT.normalColor);

	// テキストスプライトの初期化
	buttonRT.textSprite = std::make_unique<MagEngine::Sprite>();
	buttonRT.textSprite->Initialize(spriteSetup_, "WolfOne_MachineGun.png");
	buttonRT.textBasePosition = { baseX + 55.0f, baseY + spacing };
	buttonRT.textSize = { 80.0f, 20.0f };
	buttonRT.textAlpha = 0.0f;
	buttonRT.textTargetAlpha = 0.7f;
	buttonRT.textSlideOffset = -20.0f;
	buttonRT.textTargetOffset = 0.0f;
	buttonRT.textSprite->SetAnchorPoint({ 0.0f, 0.5f });
	buttonRT.textSprite->SetSize(buttonRT.textSize);
	buttonRT.textSprite->SetColor({ 1.0f, 0.9f, 0.8f, 0.0f });

	// === Bボタン（ミサイル）- 3番目 ===
	auto &buttonB = buttons_[ControllerButton::ButtonB];
	buttonB.sprite = std::make_unique<MagEngine::Sprite>();
	buttonB.sprite->Initialize(spriteSetup_, "xbox_button_color_b.png");
	buttonB.basePosition = { baseX, baseY + spacing * 2 };
	buttonB.baseSize = { buttonSize, buttonSize };
	buttonB.normalColor = { 0.8f, 0.1f, 0.1f, opacity_ * 0.8f }; // 深紅
	buttonB.pressedColor = { 1.0f, 0.2f, 0.2f, opacity_ };
	buttonB.currentScale = 1.0f;
	buttonB.targetScale = 1.0f;
	buttonB.isPressed = false;
	buttonB.pulseTime = 0.0f;
	buttonB.labelText = "B: Missile";
	buttonB.textLabelPosition = { baseX + 45.0f, baseY + spacing * 2 }; // ボタンの右側
	buttonB.sprite->SetAnchorPoint({ 0.5f, 0.5f });
	buttonB.sprite->SetPosition(buttonB.basePosition);
	buttonB.sprite->SetSize(buttonB.baseSize);
	buttonB.sprite->SetColor(buttonB.normalColor);

	// テキストスプライトの初期化
	buttonB.textSprite = std::make_unique<MagEngine::Sprite>();
	buttonB.textSprite->Initialize(spriteSetup_, "WolfOne_Missile.png");
	buttonB.textBasePosition = { baseX + 55.0f, baseY + spacing * 2 };
	buttonB.textSize = { 80.0f, 20.0f };
	buttonB.textAlpha = 0.0f;
	buttonB.textTargetAlpha = 0.7f;
	buttonB.textSlideOffset = -20.0f;
	buttonB.textTargetOffset = 0.0f;
	buttonB.textSprite->SetAnchorPoint({ 0.0f, 0.5f });
	buttonB.textSprite->SetSize(buttonB.textSize);
	buttonB.textSprite->SetColor({ 1.0f, 0.8f, 0.8f, 0.0f });

	// === Aボタン（ブースト/バレルロール）- 4番目 ===
	auto &buttonA = buttons_[ControllerButton::ButtonA];
	buttonA.sprite = std::make_unique<MagEngine::Sprite>();
	buttonA.sprite->Initialize(spriteSetup_, "xbox_button_color_a.png");
	buttonA.basePosition = { baseX, baseY + spacing * 3 };
	buttonA.baseSize = { buttonSize, buttonSize };
	buttonA.normalColor = { 0.1f, 0.8f, 0.3f, opacity_ * 0.8f }; // 緑
	buttonA.pressedColor = { 0.2f, 1.0f, 0.4f, opacity_ };
	buttonA.currentScale = 1.0f;
	buttonA.targetScale = 1.0f;
	buttonA.isPressed = false;
	buttonA.pulseTime = 0.0f;
	buttonA.labelText = "A: Boost/Roll";
	buttonA.textLabelPosition = { baseX + 45.0f, baseY + spacing * 3 }; // ボタンの右側
	buttonA.sprite->SetAnchorPoint({ 0.5f, 0.5f });
	buttonA.sprite->SetPosition(buttonA.basePosition);
	buttonA.sprite->SetSize(buttonA.baseSize);
	buttonA.sprite->SetColor(buttonA.normalColor);

	// テキストスプライトの初期化
	buttonA.textSprite = std::make_unique<MagEngine::Sprite>();
	buttonA.textSprite->Initialize(spriteSetup_, "WolfOne_Dodge.png");
	buttonA.textBasePosition = { baseX + 55.0f, baseY + spacing * 3 };
	buttonA.textSize = { 80.0f, 20.0f };
	buttonA.textAlpha = 0.0f;
	buttonA.textTargetAlpha = 0.7f;
	buttonA.textSlideOffset = -20.0f;
	buttonA.textTargetOffset = 0.0f;
	buttonA.textSprite->SetAnchorPoint({ 0.0f, 0.5f });
	buttonA.textSprite->SetSize(buttonA.textSize);
	buttonA.textSprite->SetColor({ 0.8f, 1.0f, 0.8f, 0.0f });

	// === Yボタン（ロックオン）- 5番目 ===
	auto &buttonY = buttons_[ControllerButton::ButtonY];
	buttonY.sprite = std::make_unique<MagEngine::Sprite>();
	buttonY.sprite->Initialize(spriteSetup_, "xbox_button_color_y.png");
	buttonY.basePosition = { baseX, baseY + spacing * 4 };
	buttonY.baseSize = { buttonSize, buttonSize };
	buttonY.normalColor = { 0.9f, 0.8f, 0.1f, opacity_ * 0.8f }; // ゴールド
	buttonY.pressedColor = { 1.0f, 0.95f, 0.2f, opacity_ };
	buttonY.currentScale = 1.0f;
	buttonY.targetScale = 1.0f;
	buttonY.isPressed = false;
	buttonY.pulseTime = 0.0f;
	buttonY.labelText = "Y: Lock-On";
	buttonY.textLabelPosition = { baseX + 45.0f, baseY + spacing * 4 }; // ボタンの右側
	buttonY.sprite->SetAnchorPoint({ 0.5f, 0.5f });
	buttonY.sprite->SetPosition(buttonY.basePosition);
	buttonY.sprite->SetSize(buttonY.baseSize);
	buttonY.sprite->SetColor(buttonY.normalColor);

	// テキストスプライトの初期化
	buttonY.textSprite = std::make_unique<MagEngine::Sprite>();
	buttonY.textSprite->Initialize(spriteSetup_, "WolfOne_Test.png");
	buttonY.textBasePosition = { baseX + 55.0f, baseY + spacing * 4 };
	buttonY.textSize = { 80.0f, 20.0f };
	buttonY.textAlpha = 0.0f;
	buttonY.textTargetAlpha = 0.7f;
	buttonY.textSlideOffset = -20.0f;
	buttonY.textTargetOffset = 0.0f;
	buttonY.textSprite->SetAnchorPoint({ 0.0f, 0.5f });
	buttonY.textSprite->SetSize(buttonY.textSize);
	buttonY.textSprite->SetColor({ 1.0f, 1.0f, 0.8f, 0.0f });
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
	if(!isVisible_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f; // 60FPS想定

	// ボタン状態の更新
	UpdateButtonStates();

	// アニメーションの更新
	UpdateButtonAnimations(deltaTime);

	// スプライト更新
	if(backgroundSprite_) {
		backgroundSprite_->Update();
	}

	for(auto &[button, info] : buttons_) {
		if(info.sprite) {
			info.sprite->Update();
		}
		if(info.textSprite) {
			info.textSprite->Update();
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
	bool stickMoved = ( std::abs(stickX) > 0.1f || std::abs(stickY) > 0.1f );
	buttons_[ControllerButton::LeftStick].isPressed = stickMoved;

	// スティックの傾きをオフセットに変換
	leftStickOffset_.x = stickX * stickMoveRange_;
	leftStickOffset_.y = -stickY * stickMoveRange_; // Y軸反転

	// Aボタン
	buttons_[ControllerButton::ButtonA].isPressed = input->PushButton(XINPUT_GAMEPAD_A);

	// Bボタン
	buttons_[ControllerButton::ButtonB].isPressed = input->PushButton(XINPUT_GAMEPAD_B);

	// Yボタン
	buttons_[ControllerButton::ButtonY].isPressed = input->PushButton(XINPUT_GAMEPAD_Y);

	// RTボタン（右トリガー）
	float rightTrigger = input->GetRightTrigger();
	buttons_[ControllerButton::RT].isPressed = ( rightTrigger > 0.1f );
}

///=============================================================================
///                        ボタンアニメーションの更新
void OperationGuideUI::UpdateButtonAnimations(float deltaTime) {
	// スティックオフセットの滑らかな補間
	currentStickOffset_.x += ( leftStickOffset_.x - currentStickOffset_.x ) * stickMoveSmoothing_;
	currentStickOffset_.y += ( leftStickOffset_.y - currentStickOffset_.y ) * stickMoveSmoothing_;

	// グロー効果の更新
	glowIntensity_ += deltaTime * glowPulseSpeed_;

	for(auto &[button, info] : buttons_) {
		// ターゲットスケールの設定
		if(info.isPressed) {
			info.targetScale = pressedScale_;
			info.pulseTime += deltaTime * 10.0f;
			// テキストを強調表示
			info.textTargetAlpha = 1.0f;
			info.textTargetOffset = 5.0f;
		} else {
			info.targetScale = normalScale_;
			info.pulseTime = 0.0f;
			// テキストを通常表示
			info.textTargetAlpha = 0.7f;
			info.textTargetOffset = 0.0f;
		}

		// スケールの補間
		float lerpSpeed = info.isPressed ? pressAnimationSpeed_ : releaseAnimationSpeed_;
		info.currentScale += ( info.targetScale - info.currentScale ) * lerpSpeed * deltaTime;

		// スプライトに適用
		if(info.sprite) {
			// サイズの更新
			Vector2 scaledSize = {
				info.baseSize.x * info.currentScale,
				info.baseSize.y * info.currentScale };
			info.sprite->SetSize(scaledSize);

			// 位置の更新（左スティックのみ傾きに連動）
			Vector2 finalPosition = info.basePosition;
			if(button == ControllerButton::LeftStick) {
				finalPosition.x += currentStickOffset_.x;
				finalPosition.y += currentStickOffset_.y;
			}
			info.sprite->SetPosition(finalPosition);

			// 色の更新（押されている時は押下色に変化 + グロー効果）
			Vector4 currentColor;
			if(info.isPressed) {
				// 強力なパルス効果とグロー
				float pulse = 0.7f + 0.3f * std::abs(sinf(info.pulseTime));
				float glow = 1.0f + 0.4f * std::abs(sinf(glowIntensity_));
				currentColor = {
					info.pressedColor.x * pulse * glow,
					info.pressedColor.y * pulse * glow,
					info.pressedColor.z * pulse * glow,
					info.pressedColor.w };
			} else {
				// 通常時も微妙にパルス
				float subtlePulse = 0.85f + 0.15f * std::abs(sinf(glowIntensity_ * 0.5f));
				currentColor = {
					info.normalColor.x * subtlePulse,
					info.normalColor.y * subtlePulse,
					info.normalColor.z * subtlePulse,
					info.normalColor.w };
			}
			info.sprite->SetColor(currentColor);
		}

		// テキストスプライトの更新
		if(info.textSprite) {
			// アルファ値のスムーズな補間
			info.textAlpha += ( info.textTargetAlpha - info.textAlpha ) * 0.1f;

			// スライドオフセットのスムーズな補間
			info.textSlideOffset += ( info.textTargetOffset - info.textSlideOffset ) * 0.15f;

			// テキスト位置の更新
			Vector2 textPos = info.textBasePosition;
			textPos.x += info.textSlideOffset;
			info.textSprite->SetPosition(textPos);

			// テキスト色の更新（押下時にグロー効果）
			Vector4 textColor;
			if(info.isPressed) {
				// 押下時は明るくパルス
				float textGlow = 0.9f + 0.1f * std::abs(sinf(info.pulseTime * 2.0f));
				textColor = {
					1.0f * textGlow,
					1.0f * textGlow,
					1.0f * textGlow,
					info.textAlpha };
			} else {
				// 通常時は控えめに
				textColor = {
					0.8f,
					0.9f,
					1.0f,
					info.textAlpha };
			}
			info.textSprite->SetColor(textColor);
		}
	}
}

///=============================================================================
///                        描画
void OperationGuideUI::Draw() {
	if(!isVisible_) {
		return;
	}

	// 背景パネルの描画
	if(backgroundSprite_) {
		//backgroundSprite_->Draw();
	}

	// ボタンの描画
	for(auto &[button, info] : buttons_) {
		if(info.sprite) {
			info.sprite->Draw();
		}
		// テキストラベルの描画
		if(info.textSprite) {
			info.textSprite->Draw();
		}
	}
}

///=============================================================================
///                        イージング関数
float OperationGuideUI::EaseOutElastic(float t) {
	const float c4 = ( 2.0f * static_cast<float>( M_PI ) ) / 3.0f;
	return t == 0.0f ? 0.0f
		: t == 1.0f ? 1.0f
		: std::pow(2.0f, -10.0f * t) * std::sin(( t * 10.0f - 0.75f ) * c4) + 1.0f;
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

	// スティック移動設定
	ImGui::SliderFloat("Stick Move Range", &stickMoveRange_, 0.0f, 50.0f);
	ImGui::SliderFloat("Stick Smoothing", &stickMoveSmoothing_, 0.01f, 0.5f);
	ImGui::SliderFloat("Glow Pulse Speed", &glowPulseSpeed_, 1.0f, 10.0f);

	ImGui::Separator();

	// 位置設定
	ImGui::InputFloat2("Guide Position", &guideBasePosition_.x);

	ImGui::Separator();

	// ボタン状態表示
	ImGui::Text("Button States:");
	for(const auto &[button, info] : buttons_) {
		ImGui::Text("%s: %s",
			info.labelText.c_str(),
			info.isPressed ? "PRESSED" : "---");
	}

	ImGui::End();
#endif
}

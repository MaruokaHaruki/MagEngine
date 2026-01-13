#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Input.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include <map>
#include <memory>
#include <string>

///=============================================================================
///                        コントローラーボタン種類
enum class ControllerButton {
	LeftStick,
	RightStick,
	ButtonA,
	ButtonB,
	ButtonX,
	ButtonY,
	LB,
	RB,
	LT,
	RT,
	DPadUp,
	DPadDown,
	DPadLeft,
	DPadRight,
	Max
};

///=============================================================================
///                        ボタン表示情報
struct ButtonDisplayInfo {
	std::unique_ptr<MagEngine::Sprite> sprite;
	Vector2 basePosition;
	Vector2 baseSize;
	Vector4 normalColor;
	Vector4 pressedColor;
	float currentScale;
	float targetScale;
	bool isPressed;
	float pulseTime;
	std::string labelText;	   // ボタンの説明テキスト
	Vector2 textLabelPosition; // テキストラベル表示位置

	// テキストラベル用スプライト
	std::unique_ptr<MagEngine::Sprite> textSprite;
	Vector2 textBasePosition;
	Vector2 textSize;
	float textAlpha;		// テキストの不透明度
	float textTargetAlpha;	// 目標不透明度
	float textSlideOffset;	// スライドアニメーション用オフセット
	float textTargetOffset; // 目標オフセット
};

///=============================================================================
///                        操作ガイドUIクラス
class OperationGuideUI {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::SpriteSetup *spriteSetup);

	/// \brief 終了処理
	void Finalize();

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        表示制御
	/// \brief 表示/非表示の切り替え
	void SetVisible(bool visible) {
		isVisible_ = visible;
	}

	/// \brief 表示状態を取得
	bool IsVisible() const {
		return isVisible_;
	}

	/// \brief 不透明度を設定
	void SetOpacity(float opacity) {
		opacity_ = opacity;
	}

	/// \brief ガイド位置を設定（画面の隅に配置）
	void SetGuidePosition(const Vector2 &position) {
		guideBasePosition_ = position;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	void InitializeButtons();
	void UpdateButtonStates();
	void UpdateButtonAnimations(float deltaTime);
	float EaseOutElastic(float t);
	float EaseInOutQuad(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> backgroundSprite_ = nullptr; // 背景パネル

	// ボタン情報マップ
	std::map<ControllerButton, ButtonDisplayInfo> buttons_;

	// 表示設定
	bool isVisible_ = true;
	float opacity_ = 0.8f;
	Vector2 guideBasePosition_ = {16.0f, 600.0f}; // デフォルトは左下寄り

	// アニメーション設定
	float pressAnimationSpeed_ = 8.0f;
	float releaseAnimationSpeed_ = 6.0f;
	float pressedScale_ = 1.3f;
	float normalScale_ = 1.0f;

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// スティック傾き連動
	Vector2 leftStickOffset_ = {0.0f, 0.0f};
	Vector2 currentStickOffset_ = {0.0f, 0.0f};
	float stickMoveRange_ = 15.0f;	   // スティックUIの最大移動範囲
	float stickMoveSmoothing_ = 0.15f; // 移動の滑らかさ

	// グロー効果
	float glowIntensity_ = 0.0f;
	float glowPulseSpeed_ = 5.0f;
};

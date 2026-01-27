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
///                        メニューボタン種類
enum class MenuButton {
	ResumeGame,		// ゲームに戻る
	OperationGuide, // 操作説明
	ReturnToTitle,	// タイトルに戻る
	Max
};

///=============================================================================
///                        メニューボタン表示情報
struct MenuButtonDisplayInfo {
	std::unique_ptr<MagEngine::Sprite> sprite;
	Vector2 basePosition;
	Vector2 baseSize;
	Vector4 normalColor;
	Vector4 highlightColor;
	float currentScale;
	float targetScale;
	bool isSelected;
	bool isPressed;
	float pulseTime;
	std::string labelText;

	// テキストラベル用スプライト
	std::unique_ptr<MagEngine::Sprite> textSprite;
	Vector2 textPosition;
	Vector2 textSize;
	float textAlpha;
	float textTargetAlpha;
};

///=============================================================================
///                        ゲーム中メニュークラス
class MenuUI {
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
	/// \brief メニューを開く
	void Open() {
		isOpen_ = true;
		selectedIndex_ = 0;
	}

	/// \brief メニューを閉じる
	void Close() {
		isOpen_ = false;
	}

	/// \brief メニューが開いているか
	bool IsOpen() const {
		return isOpen_;
	}

	/// \brief 選択されたボタンを取得
	MenuButton GetSelectedButton() const {
		return static_cast<MenuButton>(selectedIndex_);
	}

	/// \brief ボタンが選択されたか確認
	bool IsButtonPressed() const {
		return isButtonPressed_;
	}

	/// \brief ボタン押下フラグをリセット
	void ResetButtonPressedFlag() {
		isButtonPressed_ = false;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	void InitializeButtons();
	void UpdateButtonSelection();
	void UpdateButtonAnimations(float deltaTime);
	float EaseOutElastic(float t);
	float EaseInOutQuad(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> backgroundSprite_ = nullptr; // 背景パネル
	std::unique_ptr<MagEngine::Sprite> titleSprite_ = nullptr;		// タイトル

	// ボタン情報マップ
	std::map<MenuButton, MenuButtonDisplayInfo> buttons_;

	// メニュー状態
	bool isOpen_ = false;
	int selectedIndex_ = 0;
	bool isButtonPressed_ = false;

	// アニメーション設定
	float selectAnimationSpeed_ = 8.0f;
	float pressAnimationSpeed_ = 10.0f;
	float selectedScale_ = 1.2f;
	float normalScale_ = 1.0f;

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// フェードイン・アウト
	float fadeAlpha_ = 0.0f;
	float targetFadeAlpha_ = 0.0f;
	float fadeSpeed_ = 3.0f;

	// グロー効果
	float glowIntensity_ = 0.0f;
	float glowPulseSpeed_ = 5.0f;
};

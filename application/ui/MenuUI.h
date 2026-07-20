#pragma once
#include "MagMath.h"
using Vector2 = MagMath::Vector2;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <map>
#include <memory>
#include <string>

namespace MagEngine {
	class Input;
	class RenderWorld;
}

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
	/// \brief メニュースプライトと入力参照を初期化する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	/// \param input メニュー操作に使用する入力
	void Initialize(MagEngine::SpriteSetup *spriteSetup, MagEngine::Input &input);

	/// \brief 終了処理
	void Finalize();

	/// \brief 開閉状態に応じて選択入力とボタンアニメーションを更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 描画
	void Draw();

	/// \brief 開いているメニューのスプライトを描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        表示制御
	/// \brief メニューを開き、先頭ボタンを選択状態にする
	void Open() {
		isOpen_ = true;
		selectedIndex_ = 0;
	}

	/// \brief メニューを閉じる
	void Close() {
		isOpen_ = false;
	}

	/// \brief メニューが開いているかを取得する
	/// \return 開いている場合はtrue、それ以外はfalse
	bool IsOpen() const {
		return isOpen_;
	}

	/// \brief 現在選択されているボタン種別を取得する
	/// \return 選択中のMenuButton
	MenuButton GetSelectedButton() const {
		return static_cast<MenuButton>(selectedIndex_);
	}

	/// \brief 決定入力によりボタンが押されたかを取得する
	/// \return 押下フラグが立っている場合はtrue、それ以外はfalse
	bool IsButtonPressed() const {
		return isButtonPressed_;
	}

	/// \brief 消費済みのボタン押下フラグをリセットする
	void ResetButtonPressedFlag() {
		isButtonPressed_ = false;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	/// \brief 各メニューボタンのスプライトと初期表示値を生成する
	void InitializeButtons();
	/// \brief 入力に応じて選択中のボタンと決定フラグを更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void UpdateButtonSelection(float unscaledDeltaTime);
	/// \brief 選択・押下状態に対応するボタン表示を補間する
	/// \param deltaTime 前フレームからの経過時間（秒）
	void UpdateButtonAnimations(float deltaTime);
	/// \brief 弾性を伴う減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOutElastic(float t);
	/// \brief 加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOutQuad(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	MagEngine::Input *input_ = nullptr;
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

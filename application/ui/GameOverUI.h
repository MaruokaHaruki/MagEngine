#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>

///=============================================================================
///                        ゲームオーバーアニメーション状態
enum class GameOverState {
	Idle,	 // アイドル状態
	Showing, // テキスト表示中
	Done,	 // 完了
};

///=============================================================================
///                        ゲームオーバーUIクラス
class GameOverUI {
	// デフォルト設定値
	static constexpr float DEFAULT_FADE_DURATION = 2.0f;
	static constexpr float DEFAULT_DISPLAY_DURATION = 3.0f;
	static constexpr float DEFAULT_BACKGROUND_ALPHA = 0.8f;
	static constexpr float SCREEN_WIDTH = 1280.0f;
	static constexpr float SCREEN_HEIGHT = 720.0f;

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

	// ========== アニメーション制御 ==========
	/// \brief ゲームオーバー演出を開始
	void Play(float fadeDuration = DEFAULT_FADE_DURATION,
			  float displayDuration = DEFAULT_DISPLAY_DURATION);

	/// \brief アニメーションのキャンセル
	void Stop();

	// ========== 状態取得 ==========
	/// \brief アニメーション中かどうか
	bool IsPlaying() const {
		return state_ == GameOverState::Showing;
	}

	/// \brief 完了したかどうか
	bool IsDone() const {
		return state_ == GameOverState::Done;
	}

	// ========== UI設定 ==========
	/// \brief 背景色を設定
	void SetBackgroundColor(const Vector4 &color) {
		backgroundColor_ = color;
	}

	/// \brief テキストテクスチャを設定
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief テキストサイズを設定
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief 完了時コールバックを設定
	void SetOnComplete(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

private:
	void UpdateAnimation();
	void InitializeSprites();
	void ResetAnimation();
	float EaseInOut(float t) const;

	// ========== スプライト ==========
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> backgroundSprite_;
	std::unique_ptr<MagEngine::Sprite> textSprite_;

	// ========== アニメーション状態 ==========
	GameOverState state_ = GameOverState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;
	float fadeDuration_ = DEFAULT_FADE_DURATION;
	float displayDuration_ = DEFAULT_DISPLAY_DURATION;

	// ========== UI設定 ==========
	Vector4 backgroundColor_ = {0.0f, 0.0f, 0.0f, 1.0f};
	std::string textTexture_ = "WolfOne_GameOver.dds";
	Vector2 textSize_ = {512.0f, 64.0f};

	// ========== コールバック ==========
	std::function<void()> onCompleteCallback_;
};

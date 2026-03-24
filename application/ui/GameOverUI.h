#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

///=============================================================================
///                        ゲームオーバーアニメーション状態
enum class GameOverState {
	Idle,	   // アイドル状態
	Appearing, // テキスト出現中
	Displaying, // テキスト表示中
	Fading,	   // フェードアウト中
	Done,	   // 完了
};

///=============================================================================
///                        パーティクル構造体
struct GameOverParticle {
	Vector2 position;
	Vector2 velocity;
	float lifetime;
	float maxLifetime;
	float scale;
	Vector4 color;
	bool active;
};

///=============================================================================
///                        ゲームオーバーUIクラス
class GameOverUI {
	// デフォルト設定値
	static constexpr float DEFAULT_APPEAR_DURATION = 0.8f;
	static constexpr float DEFAULT_DISPLAY_DURATION = 2.5f;
	static constexpr float DEFAULT_FADE_DURATION = 1.2f;
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
	void Play(float appearDuration = DEFAULT_APPEAR_DURATION,
			  float displayDuration = DEFAULT_DISPLAY_DURATION,
			  float fadeDuration = DEFAULT_FADE_DURATION);

	/// \brief アニメーションのキャンセル
	void Stop();

	// ========== 状態取得 ==========
	/// \brief アニメーション中かどうか
	bool IsPlaying() const {
		return state_ != GameOverState::Idle && state_ != GameOverState::Done;
	}

	/// \brief 完了したかどうか
	bool IsDone() const {
		return state_ == GameOverState::Done;
	}

	// ========== UI設定 ==========
	/// \brief テキストテクスチャを設定
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
		if (spriteSetup_) {
			InitializeSprites(); // テクスチャ設定後、スプライトを再初期化
		}
	}

	/// \brief テキストサイズを設定
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief テキストカラーを設定
	void SetTextColor(const Vector4 &color) {
		textColor_ = color;
	}

	/// \brief 背景フェードの色を設定
	void SetFadeBackgroundColor(const Vector4 &color) {
		fadeBackgroundColor_ = color;
	}

	/// \brief 完了時コールバックを設定
	void SetOnComplete(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

private:
	void UpdateAppearing();
	void UpdateDisplaying();
	void UpdateFading();
	void UpdateParticles();
	void GenerateParticles();
	void InitializeSprites();
	void ResetAnimation();

	float EaseInOut(float t) const;
	float EaseOut(float t) const;
	float EaseIn(float t) const;
	float EaseOutBounce(float t) const;

	// ========== スプライト ==========
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> fadeBackgroundSprite_;
	std::unique_ptr<MagEngine::Sprite> textSprite_;
	std::unique_ptr<MagEngine::Sprite> glowSprite_;		// グロー効果
	std::unique_ptr<MagEngine::Sprite> borderSprite1_;	// 上部ボーダー
	std::unique_ptr<MagEngine::Sprite> borderSprite2_;	// 下部ボーダー

	// パーティクル
	std::vector<GameOverParticle> particles_;
	static constexpr int kMaxParticles = 30;

	// ========== アニメーション状態 ==========
	GameOverState state_ = GameOverState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float appearDuration_ = DEFAULT_APPEAR_DURATION;
	float displayDuration_ = DEFAULT_DISPLAY_DURATION;
	float fadeDuration_ = DEFAULT_FADE_DURATION;

	// 表示設定
	Vector4 textColor_ = {1.0f, 0.2f, 0.2f, 1.0f};			// 鮮やかな赤
	Vector2 textSize_ = {1000.0f, 250.0f};
	std::string textTexture_ = "white1x1.dds";
	Vector4 fadeBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.7f}; // 濃い黒

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_;
};

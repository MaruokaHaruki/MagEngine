#pragma once
#include "MagMath.h"
using Vector2 = MagMath::Vector2;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace MagEngine {
	class RenderWorld;
}

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
	/// \brief ゲームオーバー表示に必要なスプライトを初期化する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	void Initialize(MagEngine::SpriteSetup *spriteSetup);

	/// \brief 保持するスプライトとパーティクルを解放する
	void Finalize();

	/// \brief 演出状態とパーティクルを時間停止の影響なしで更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 演出用スプライトの表示状態を更新する
	void Draw();

	/// \brief 演出用スプライトをフレーム描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 演出パラメータを調整するImGuiを描画する
	void DrawImGui();

	// ========== アニメーション制御 ==========
	/// \brief ゲームオーバー演出を出現・表示・フェードの順で開始する
	/// \param appearDuration テキスト出現時間（秒）
	/// \param displayDuration テキスト表示時間（秒）
	/// \param fadeDuration フェードアウト時間（秒）
	void Play(float appearDuration = DEFAULT_APPEAR_DURATION,
			  float displayDuration = DEFAULT_DISPLAY_DURATION,
			  float fadeDuration = DEFAULT_FADE_DURATION);

	/// \brief 実行中のゲームオーバー演出を停止する
	void Stop();

	// ========== 状態取得 ==========
	/// \brief ゲームオーバー演出が進行中かを判定する
	/// \return 待機・完了以外の状態の場合はtrue、それ以外はfalse
	bool IsPlaying() const {
		return state_ != GameOverState::Idle && state_ != GameOverState::Done;
	}

	/// \brief ゲームオーバー演出が完了したかを判定する
	/// \return 完了状態の場合はtrue、それ以外はfalse
	bool IsDone() const {
		return state_ == GameOverState::Done;
	}

	// ========== UI設定 ==========
	/// \brief テキスト用テクスチャを設定し、初期化済みならスプライトを作り直す
	/// \param textureFilePath 読み込むテクスチャファイルのパス
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
		if (spriteSetup_) {
			InitializeSprites(); // テクスチャ設定後、スプライトを再初期化
		}
	}

	/// \brief テキストスプライトの表示サイズを設定する
	/// \param size スプライトの幅・高さ
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief テキストの表示色を設定する
	/// \param color RGBA形式の表示色
	void SetTextColor(const Vector4 &color) {
		textColor_ = color;
	}

	/// \brief 背景フェードの表示色を設定する
	/// \param color RGBA形式の表示色
	void SetFadeBackgroundColor(const Vector4 &color) {
		fadeBackgroundColor_ = color;
	}

	/// \brief 演出完了時に呼び出すコールバックを設定する
	/// \param callback 呼び出す処理
	void SetOnComplete(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

private:
	/// \brief テキスト出現フェーズを更新する
	void UpdateAppearing();
	/// \brief テキスト表示フェーズを更新する
	void UpdateDisplaying();
	/// \brief フェードアウトフェーズを更新する
	void UpdateFading();
	/// \brief 発生済みパーティクルを更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void UpdateParticles(float unscaledDeltaTime);
	/// \brief 出現演出に使用するパーティクルを生成する
	void GenerateParticles();
	/// \brief 現在の表示設定で演出用スプライトを生成する
	void InitializeSprites();
	/// \brief 演出状態を待機状態へ戻す
	void ResetAnimation();

	/// \brief 加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOut(float t) const;
	/// \brief 減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOut(float t) const;
	/// \brief 加速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseIn(float t) const;
	/// \brief 跳ね返りを伴う減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
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

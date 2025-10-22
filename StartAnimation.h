#pragma once
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>

///=============================================================================
///                        アニメーション状態
enum class StartAnimationState {
	Idle,	   // アイドル状態
	Opening,   // バーが登場中
	Showing,   // バーとテキストを表示中
	Closing,   // バーが退場中
	Completed, // 完了
};

///=============================================================================
///                        スタートアニメーションクラス
class StartAnimation {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief 初期化
	void Initialize(SpriteSetup *spriteSetup);

	/// \brief 終了処理
	void Finalize();

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        アニメーション制御
	/// \brief 開始演出を開始
	/// \param showDuration 表示時間（秒）
	/// \param openDuration バー登場時間（秒）
	/// \param closeDuration バー退場時間（秒）
	void StartOpening(float showDuration = 2.0f, float openDuration = 1.0f, float closeDuration = 1.0f);

	/// \brief 終了演出を開始（逆再生）
	/// \param showDuration 表示時間（秒）
	/// \param openDuration バー登場時間（秒）
	/// \param closeDuration バー退場時間（秒）
	void StartClosing(float showDuration = 2.0f, float openDuration = 1.0f, float closeDuration = 1.0f);

	/// \brief アニメーションのキャンセル
	void Cancel();

	/// \brief アニメーションのリセット
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief アニメーション中かどうか
	bool IsAnimating() const {
		return state_ != StartAnimationState::Idle && state_ != StartAnimationState::Completed;
	}

	/// \brief 完了したかどうか
	bool IsCompleted() const {
		return state_ == StartAnimationState::Completed;
	}

	/// \brief 現在の状態を取得
	StartAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief バーの色を設定
	void SetBarColor(const Vector4 &color) {
		barColor_ = color;
	}

	/// \brief テキストスプライトのテクスチャを設定
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief バーの高さを設定（画面高さに対する比率 0.0〜1.0）
	void SetBarHeightRatio(float ratio) {
		barHeightRatio_ = ratio;
	}

	/// \brief テキストのサイズを設定
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief 完了時のコールバックを設定
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	void UpdateOpening();
	void UpdateShowing();
	void UpdateClosing();

	float EaseInOut(float t);
	float EaseOut(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<Sprite> topBar_ = nullptr;	   // 上部バー
	std::unique_ptr<Sprite> bottomBar_ = nullptr;  // 下部バー
	std::unique_ptr<Sprite> textSprite_ = nullptr; // 中央テキスト

	// アニメーション状態
	StartAnimationState state_ = StartAnimationState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float openDuration_ = 1.0f;	 // バー登場時間
	float showDuration_ = 2.0f;	 // 表示時間
	float closeDuration_ = 1.0f; // バー退場時間

	// 表示設定
	Vector4 barColor_ = {0.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは黒
	std::string textTexture_ = "white1x1.png";
	float barHeightRatio_ = 0.15f; // 画面高さの15%
	Vector2 textSize_ = {400.0f, 100.0f};

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;

	// 逆再生フラグ
	bool isReversed_ = false;
};

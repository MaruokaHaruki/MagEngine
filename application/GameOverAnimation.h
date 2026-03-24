/*********************************************************************
 * \file   GameOverAnimation.h
 * \brief  ゲームオーバー演出管理
 *
 * \author Harukichimaru
 * \date   March 2026
 * \note   敗北時のテキスト表示とフェードアウト演出を制御
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <memory>
#include <string>
#include <functional>

///=============================================================================
///                        ゲームオーバー演出状態
enum class GameOverAnimationState {
	Idle,	   // 待機中
	Appearing, // テキスト出現中
	Displaying, // テキスト表示中
	Fading,	   // フェードアウト中
	Done,	   // 完了
};

///=============================================================================
///                        ゲームオーバーアニメーションクラス
class GameOverAnimation {
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

	/// \brief ImGui描画（デバッグ用）
	void DrawImGui();

	///--------------------------------------------------------------
	///                        アニメーション制御
	/// \brief ゲームオーバー演出を開始
	/// \param appearDuration テキスト出現時間（秒）
	/// \param displayDuration テキスト表示時間（秒）
	/// \param fadeDuration フェードアウト時間（秒）
	void StartGameOverAnimation(
		float appearDuration = 0.5f,
		float displayDuration = 2.0f,
		float fadeDuration = 1.0f);

	/// \brief アニメーションのリセット
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief アニメーション中かどうか
	bool IsAnimating() const {
		return state_ != GameOverAnimationState::Idle && state_ != GameOverAnimationState::Done;
	}

	/// \brief 完了したかどうか
	bool IsDone() const {
		return state_ == GameOverAnimationState::Done;
	}

	/// \brief 現在の状態を取得
	GameOverAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief テキストの色を設定
	void SetTextColor(const Vector4 &color) {
		textColor_ = color;
	}

	/// \brief テキストのサイズを設定
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief テキストスプライトのテクスチャを設定
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief 背景フェードの色を設定
	void SetFadeBackgroundColor(const Vector4 &color) {
		fadeBackgroundColor_ = color;
	}

	/// \brief 完了時のコールバックを設定
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	void UpdateAppearing();
	void UpdateDisplaying();
	void UpdateFading();

	float EaseInOut(float t);
	float EaseOut(float t);
	float EaseIn(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> textSprite_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> fadeBackgroundSprite_ = nullptr;

	// アニメーション状態
	GameOverAnimationState state_ = GameOverAnimationState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float appearDuration_ = 0.5f;
	float displayDuration_ = 2.0f;
	float fadeDuration_ = 1.0f;

	// 表示設定
	Vector4 textColor_ = {1.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは赤
	Vector2 textSize_ = {800.0f, 200.0f};
	std::string textTexture_ = "white1x1.png"; // 白いテクスチャ（色で染めて使用）
	Vector4 fadeBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.5f}; // 半透明の黒

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;
};

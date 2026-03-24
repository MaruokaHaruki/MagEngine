/*********************************************************************
 * \file   GameOverAnimation.h
 * \brief  ゲームオーバー演出管理（改良版：豪華な演出）
 *
 * \author Harukichimaru
 * \date   March 2026
 * \note   敗北時のテキスト表示とフェードアウト演出を制御
 *         複数のエフェクトで、かっこいいおしゃれな演出を実現
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <memory>
#include <string>
#include <functional>
#include <vector>

///=============================================================================
///                        ゲームオーバー演出状態
enum class GameOverAnimationState {
	Idle,	   // 待機中
	Appearing, // テキスト出現中（爆発＆スライドイン）
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
		float appearDuration = 0.8f,
		float displayDuration = 2.5f,
		float fadeDuration = 1.2f);

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

	void UpdateParticles();
	void GenerateParticles();

	float EaseInOut(float t);
	float EaseOut(float t);
	float EaseIn(float t);
	float EaseOutBounce(float t);

	///--------------------------------------------------------------
	///                        パーティクル構造体
	struct Particle {
		Vector2 position;
		Vector2 velocity;
		float lifetime;
		float maxLifetime;
		float scale;
		Vector4 color;
		bool active;
	};

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> textSprite_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> fadeBackgroundSprite_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> glowSprite_ = nullptr;	  // グロー効果
	std::unique_ptr<MagEngine::Sprite> borderSprite1_ = nullptr; // 上部ボーダー
	std::unique_ptr<MagEngine::Sprite> borderSprite2_ = nullptr; // 下部ボーダー

	// パーティクル
	std::vector<Particle> particles_;
	static constexpr int kMaxParticles = 30;

	// アニメーション状態
	GameOverAnimationState state_ = GameOverAnimationState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float appearDuration_ = 0.8f;
	float displayDuration_ = 2.5f;
	float fadeDuration_ = 1.2f;

	// 表示設定
	Vector4 textColor_ = {1.0f, 0.2f, 0.2f, 1.0f};			  // 鮮やかな赤
	Vector2 textSize_ = {1000.0f, 250.0f};
	std::string textTexture_ = "white1x1.dds";
	Vector4 fadeBackgroundColor_ = {0.0f, 0.0f, 0.0f, 0.7f}; // 濃い黒

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;
};

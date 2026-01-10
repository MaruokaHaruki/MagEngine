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
	Done,	 // 完了（Completed から Done に変更）
};

///=============================================================================
///                        ゲームオーバーUIクラス
class GameOverUI {
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
	/// \brief ゲームオーバー演出を開始
	/// \param fadeDuration フェード時間（秒）
	/// \param displayDuration 表示時間（秒）
	void StartGameOver(float fadeDuration = 2.0f, float displayDuration = 3.0f);

	/// \brief アニメーションのキャンセル
	void Cancel();

	/// \brief アニメーションのリセット
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief アニメーション中かどうか
	bool IsAnimating() const {
		return state_ != GameOverState::Idle && state_ != GameOverState::Done;
	}

	/// \brief 完了したかどうか
	bool IsDone() const { // IsCompleted から IsDone に変更
		return state_ == GameOverState::Done;
	}

	///--------------------------------------------------------------
	///                        設定
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

	/// \brief 完了時のコールバックを設定
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	void UpdateShowing();
	float EaseInOut(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<Sprite> backgroundSprite_ = nullptr; // 背景
	std::unique_ptr<Sprite> textSprite_ = nullptr;		 // テキスト

	// アニメーション状態
	GameOverState state_ = GameOverState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float fadeDuration_ = 2.0f;	   // フェード時間
	float displayDuration_ = 3.0f; // 表示時間

	// 表示設定
	Vector4 backgroundColor_ = {0.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは黒
	std::string textTexture_ = "WolfOne_GameOver.png";
	Vector2 textSize_ = {512.0f, 64.0f};

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;
};

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
using Vector2 = MagMath::Vector2;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace MagEngine {
	class RenderWorld;
}

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
	/// \brief ゲームオーバー演出で使用するスプライトを生成する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	void Initialize(MagEngine::SpriteSetup *spriteSetup);

	/// \brief 保持するスプライトとパーティクルを解放する
	void Finalize();

	/// \brief 演出状態とパーティクルを時間停止の影響なしで更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 演出用スプライトの表示状態を更新する
	void Draw();

	/// \brief 演出用スプライトをフレームの描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 演出パラメータを調整するデバッグImGuiを描画する
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

	/// \brief 演出を待機状態へ戻し、全表示要素を初期状態へ戻す
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief 演出が待機・完了以外の状態かを判定する
	/// \return 演出を進行中の場合はtrue、待機または完了済みの場合はfalse
	bool IsAnimating() const {
		return state_ != GameOverAnimationState::Idle && state_ != GameOverAnimationState::Done;
	}

	/// \brief 演出が完了状態へ到達したかを判定する
	/// \return 完了状態の場合はtrue、それ以外はfalse
	bool IsDone() const {
		return state_ == GameOverAnimationState::Done;
	}

	/// \brief 現在のゲームオーバー演出状態を取得する
	/// \return 進行中の演出状態
	GameOverAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief ゲームオーバーテキストの表示色を設定する
	/// \param color RGBA形式の表示色
	void SetTextColor(const Vector4 &color) {
		textColor_ = color;
	}

	/// \brief ゲームオーバーテキストの表示サイズを設定する
	/// \param size スプライトの幅・高さ
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief ゲームオーバーテキストに使用するテクスチャパスを設定する
	/// \param textureFilePath 読み込むテクスチャファイルのパス
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief フェード背景の表示色を設定する
	/// \param color RGBA形式の表示色
	void SetFadeBackgroundColor(const Vector4 &color) {
		fadeBackgroundColor_ = color;
	}

	/// \brief 演出完了時に呼び出すコールバックを設定する
	/// \param callback 呼び出す処理
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	/// \brief テキスト出現とパーティクル発生を行うフェーズを更新する
	void UpdateAppearing();
	/// \brief テキストを表示し続けるフェーズを更新する
	void UpdateDisplaying();
	/// \brief 表示要素をフェードアウトして完了するフェーズを更新する
	void UpdateFading();

	/// \brief 発生済みパーティクルの寿命と表示状態を更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void UpdateParticles(float unscaledDeltaTime);
	/// \brief 出現演出で使用するパーティクルを初期化する
	void GenerateParticles();

	/// \brief 加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOut(float t);
	/// \brief 減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOut(float t);
	/// \brief 加速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseIn(float t);
	/// \brief 跳ね返りを伴う減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
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

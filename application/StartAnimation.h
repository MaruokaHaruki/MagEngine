#pragma once
#include "MagMath.h"
using Vector2 = MagMath::Vector2;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>

namespace MagEngine {
	class RenderWorld;
}

///=============================================================================
///                        アニメーション状態
enum class StartAnimationState {
	Idle,	 // アイドル状態
	Opening, // バーが登場中
	Showing, // バーとテキストを表示中
	Closing, // バーが退場中
	Done,	 // 完了（Completed から Done に変更）
};

///=============================================================================
///                        スタートアニメーションクラス
class StartAnimation {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief 開始演出で使用するスプライトを生成する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	void Initialize(MagEngine::SpriteSetup *spriteSetup);

	/// \brief 保持するスプライトと演出状態を解放する
	void Finalize();

	/// \brief 現在の開始演出状態を時間停止の影響なしで更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 演出用スプライトの表示状態を更新する
	void Draw();

	/// \brief 演出用スプライトをフレーム描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 演出パラメータを調整するImGuiを描画する
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

	/// \brief 演出を中断して待機状態へ戻す
	void Cancel();

	/// \brief スプライトを初期表示状態に戻し、演出を待機状態へ戻す
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief 開始演出が進行中かを判定する
	/// \return 待機・完了以外の状態の場合はtrue、それ以外はfalse
	bool IsAnimating() const {
		return state_ != StartAnimationState::Idle && state_ != StartAnimationState::Done;
	}

	/// \brief 開始演出が完了したかを判定する
	/// \return 完了状態の場合はtrue、それ以外はfalse
	bool IsDone() const { // IsCompleted から IsDone に変更
		return state_ == StartAnimationState::Done;
	}

	/// \brief 現在の開始演出状態を取得する
	/// \return 進行中の演出状態
	StartAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief 開始演出の上下バーの表示色を設定する
	/// \param color RGBA形式の表示色
	void SetBarColor(const Vector4 &color) {
		barColor_ = color;
	}

	/// \brief 中央テキストに使用するテクスチャパスを設定する
	/// \param textureFilePath 読み込むテクスチャファイルのパス
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief 上下バーの高さ比率を設定する
	/// \param ratio 画面高に対する比率
	void SetBarHeightRatio(float ratio) {
		barHeightRatio_ = ratio;
	}

	/// \brief 中央テキストの表示サイズを設定する
	/// \param size スプライトの幅・高さ
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief 演出完了時に呼び出すコールバックを設定する
	/// \param callback 呼び出す処理
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	/// \brief バーを展開するフェーズを更新する
	void UpdateOpening();
	/// \brief バーとテキストを表示するフェーズを更新する
	void UpdateShowing();
	/// \brief バーを収束させるフェーズを更新する
	void UpdateClosing();

	/// \brief 加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOut(float t);
	/// \brief 減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOut(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> topBar_ = nullptr;	   // 上部バー
	std::unique_ptr<MagEngine::Sprite> bottomBar_ = nullptr;  // 下部バー
	std::unique_ptr<MagEngine::Sprite> textSprite_ = nullptr; // 中央テキスト

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
	std::string textTexture_ = "WolfOne_Engage.dds";
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

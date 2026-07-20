#pragma once
#include "MagMath.h"
using Vector2 = MagMath::Vector2;
using Vector3 = MagMath::Vector3;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>
#include <string>

namespace MagEngine {
	class RenderWorld;
}

// 前方宣言
class FollowCamera;
class Player;

///=============================================================================
///                        クリアアニメーション状態
enum class GameClearAnimationState {
	Idle,	  // 待機中
	Opening,  // シネスコ展開中
	Showing,  // テキスト表示中
	CameraUp, // カメラ上昇演出中
	Closing,  // シネスコ収束中
	Done,	  // 完了
};

///=============================================================================
///                        ゲームクリアアニメーションクラス
class GameClearAnimation {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief クリア演出で使用するスプライトを生成する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	void Initialize(MagEngine::SpriteSetup *spriteSetup);

	/// \brief 保持するスプライトと演出状態を解放する
	void Finalize();

	/// \brief 現在の演出状態を進め、状態ごとの表示・カメラ演出を更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 保持するスプライトの表示状態を更新する
	void Draw();

	/// \brief 保持するスプライトをフレームの描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 演出パラメータを調整するImGuiを描画する
	void DrawImGui();

	///--------------------------------------------------------------
	///                        アニメーション制御
	/// \brief クリア演出を開始
	/// \param openDuration シネスコ展開時間（秒）
	/// \param showDuration テキスト表示時間（秒）
	/// \param cameraUpDuration カメラ上昇時間（秒）
	/// \param closeDuration シネスコ収束時間（秒）
	void StartClearAnimation(
		float openDuration = 1.0f,
		float showDuration = 2.0f,
		float cameraUpDuration = 3.0f,
		float closeDuration = 1.0f);

	/// \brief 実行中の演出を中断し、完了コールバックを呼び出す
	void Cancel();

	/// \brief 演出を待機状態へ戻し、各スプライトを非表示にする
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief 演出が待機・完了以外の状態かを判定する
	/// \return 演出を進行中の場合はtrue、待機または完了済みの場合はfalse
	bool IsAnimating() const {
		return state_ != GameClearAnimationState::Idle && state_ != GameClearAnimationState::Done;
	}

	/// \brief 演出が完了状態へ到達したかを判定する
	/// \return 完了状態の場合はtrue、それ以外はfalse
	bool IsDone() const {
		return state_ == GameClearAnimationState::Done;
	}

	/// \brief 現在のクリア演出状態を取得する
	/// \return 進行中の演出状態
	GameClearAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief カメラ上昇演出を適用するFollowCameraを設定する
	/// \param followCamera 演出で操作するカメラ。未設定時はカメラ演出を行わない。
	void SetFollowCamera(FollowCamera *followCamera) {
		followCamera_ = followCamera;
	}

	/// \brief 飛行演出の基準となるプレイヤーを設定する
	/// \param player 演出中に位置と姿勢を更新するプレイヤー。未設定時はプレイヤー演出を行わない。
	void SetPlayer(Player *player) {
		player_ = player;
	}

	/// \brief シネスコバーの表示色を設定する
	/// \param color RGBA形式の表示色
	void SetBarColor(const Vector4 &color) {
		barColor_ = color;
	}

	/// \brief クリアテキストに使用するテクスチャパスを設定する
	/// \param textureFilePath 読み込むテクスチャファイルのパス
	void SetTextTexture(const std::string &textureFilePath) {
		textTexture_ = textureFilePath;
	}

	/// \brief シネスコバーの高さ比率を設定する
	/// \param ratio 画面高に対する比率
	void SetBarHeightRatio(float ratio) {
		barHeightRatio_ = ratio;
	}

	/// \brief クリアテキストの表示サイズを設定する
	/// \param size スプライトの幅・高さ
	void SetTextSize(const Vector2 &size) {
		textSize_ = size;
	}

	/// \brief カメラ上昇演出の目標高さと距離を設定する
	/// \param height カメラの目標高さ
	/// \param distance プレイヤーからの目標距離
	void SetCameraUpParameters(float height, float distance) {
		cameraTargetHeight_ = height;
		cameraTargetDistance_ = distance;
	}

	/// \brief プレイヤー飛行演出の速度パラメータを設定する
	/// \param speed 前進速度
	/// \param spinRate 旋回速度（ラジアン/秒）
	/// \param climbRate 上昇速度
	void SetFlightParameters(float speed, float spinRate, float climbRate) {
		flightSpeed_ = speed;
		spinRate_ = spinRate;
		climbRate_ = climbRate;
	}

	/// \brief 演出完了またはキャンセル時に呼び出すコールバックを設定する
	/// \param callback 呼び出す処理
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	/// \brief バーを展開する開始フェーズを更新する
	void UpdateOpening();
	/// \brief クリアテキストを表示するフェーズを更新する
	void UpdateShowing();
	/// \brief カメラとプレイヤーを移動させるフェーズを更新する
	void UpdateCameraUp();
	/// \brief バーを収束させて演出を完了するフェーズを更新する
	void UpdateClosing();

	/// \brief 0から1の値へ滑らかな加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOut(float t);
	/// \brief 0から1の値へ減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOut(float t);
	/// \brief 0から1の値へ加速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseIn(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> topBar_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> bottomBar_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> textSprite_ = nullptr;

	// カメラ制御
	FollowCamera *followCamera_ = nullptr;
	Player *player_ = nullptr;
	Vector3 cameraStartPosition_;
	Vector3 cameraTargetPosition_;
	Vector3 playerStartPosition_;
	Vector3 playerStartRotation_;
	float cameraTargetHeight_ = 20.0f;
	float cameraTargetDistance_ = -30.0f;

	// 飛行演出パラメータ
	float flightSpeed_ = 15.0f; // 前進速度
	float spinRate_ = 2.0f;		// 旋回速度（rad/s）
	float climbRate_ = 8.0f;	// 上昇速度

	// アニメーション状態
	GameClearAnimationState state_ = GameClearAnimationState::Idle;
	float elapsedTime_ = 0.0f;
	float progress_ = 0.0f;

	// タイミング設定
	float openDuration_ = 1.0f;
	float showDuration_ = 2.0f;
	float cameraUpDuration_ = 3.0f;
	float closeDuration_ = 1.0f;

	// 表示設定
	Vector4 barColor_ = {0.0f, 0.0f, 0.0f, 1.0f};
	std::string textTexture_ = "white1x1.png";
	float barHeightRatio_ = 0.15f;
	Vector2 textSize_ = {600.0f, 150.0f};

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;
};

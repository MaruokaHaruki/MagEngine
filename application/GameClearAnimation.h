#pragma once
#include "Sprite.h"
#include "SpriteSetup.h"
#include "Vector3.h"
#include <functional>
#include <memory>
#include <string>

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

	/// \brief アニメーションのキャンセル
	void Cancel();

	/// \brief アニメーションのリセット
	void Reset();

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief アニメーション中かどうか
	bool IsAnimating() const {
		return state_ != GameClearAnimationState::Idle && state_ != GameClearAnimationState::Done;
	}

	/// \brief 完了したかどうか
	bool IsDone() const {
		return state_ == GameClearAnimationState::Done;
	}

	/// \brief 現在の状態を取得
	GameClearAnimationState GetState() const {
		return state_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief FollowCameraの設定
	void SetFollowCamera(FollowCamera *followCamera) {
		followCamera_ = followCamera;
	}

	/// \brief Playerの設定
	void SetPlayer(Player *player) {
		player_ = player;
	}

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

	/// \brief カメラ上昇パラメータを設定
	void SetCameraUpParameters(float height, float distance) {
		cameraTargetHeight_ = height;
		cameraTargetDistance_ = distance;
	}

	/// \brief 飛行演出パラメータを設定
	void SetFlightParameters(float speed, float spinRate, float climbRate) {
		flightSpeed_ = speed;
		spinRate_ = spinRate;
		climbRate_ = climbRate;
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
	void UpdateCameraUp();
	void UpdateClosing();

	float EaseInOut(float t);
	float EaseOut(float t);
	float EaseIn(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<Sprite> topBar_ = nullptr;
	std::unique_ptr<Sprite> bottomBar_ = nullptr;
	std::unique_ptr<Sprite> textSprite_ = nullptr;

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

#pragma once
#include "Sprite.h"
#include "SpriteSetup.h"
#include <functional>
#include <memory>

///=============================================================================
///                        トランジションタイプ
enum class TransitionType {
	Fade,			// フェードイン/アウト
	SlideLeft,		// 左からスライド
	SlideRight,		// 右からスライド
	SlideUp,		// 上からスライド
	SlideDown,		// 下からスライド
	WipeLeft,		// 左にワイプ
	WipeRight,		// 右にワイプ
	CircleExpand,	// 円形拡大
	CircleShrink,	// 円形縮小
	DiamondWipe,	// ひし形ワイプ
	CrossFade,		// クロスフェード（複数レイヤー）
	ZoomIn,			// ズームイン
	ZoomOut,		// ズームアウト
	Curtain,		// カーテン（左右から）
	VenetianBlinds, // ブラインド（水平線）
	Checkerboard,	// チェッカーボード
	PixelDissolve,	// ピクセル溶解
	Spiral,			// スパイラル
	Clock,			// 時計回り
};

///=============================================================================
///                        トランジション状態
enum class TransitionState {
	Idle,	   // アイドル（トランジションなし）
	Opening,   // 開始中（画面が見えるようになる）
	Closing,   // 終了中（画面が隠れる）
	Completed, // 完了
};

///=============================================================================
///                        シーントランジションクラス
class SceneTransition {
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
	///                        トランジション制御
	/// \brief トランジション開始（画面を隠す）
	void StartClosing(TransitionType type = TransitionType::Fade, float duration = 1.0f);

	/// \brief トランジション開始（画面を表示）
	void StartOpening(TransitionType type = TransitionType::Fade, float duration = 1.0f);

	/// \brief トランジションの即座完了
	void CompleteImmediate();

	/// \brief トランジションの中断
	void Cancel();

	/// \brief トランジションのリセット（完全初期化）
	void Reset() {
		state_ = TransitionState::Idle;
		progress_ = 0.0f;
		elapsedTime_ = 0.0f;
		additionalSprites_.clear();

		if (transitionSprite_) {
			Vector4 clearColor = transitionColor_;
			clearColor.w = 0.0f;
			transitionSprite_->SetColor(clearColor);
			transitionSprite_->SetSize({screenWidth_, screenHeight_});
			transitionSprite_->SetPosition({0.0f, 0.0f});
			transitionSprite_->SetAnchorPoint({0.0f, 0.0f});
			transitionSprite_->SetRotation(0.0f);
		}
	}

	///--------------------------------------------------------------
	///                        状態取得
	/// \brief トランジション中かどうかを取得
	bool IsTransitioning() const {
		return state_ == TransitionState::Opening || state_ == TransitionState::Closing;
	}

	/// \brief Closing中かどうか
	bool IsClosing() const {
		return state_ == TransitionState::Closing;
	}

	/// \brief Opening中かどうか
	bool IsOpening() const {
		return state_ == TransitionState::Opening;
	}

	/// \brief 完了したかどうか
	bool IsCompleted() const {
		return state_ == TransitionState::Completed;
	}

	/// \brief 進行度を取得（0.0〜1.0）
	float GetProgress() const {
		return progress_;
	}

	///--------------------------------------------------------------
	///                        設定
	/// \brief トランジション色の設定
	void SetColor(const Vector4 &color) {
		transitionColor_ = color;
	}

	/// \brief トランジション画像の設定
	void SetTexture(const std::string &textureFilePath) {
		transitionTexture_ = textureFilePath;
		useTexture_ = true;
	}

	/// \brief 単色モードに設定
	void UseSolidColor() {
		useTexture_ = false;
	}

	/// \brief コールバック設定（トランジション完了時）
	void SetOnCompleteCallback(std::function<void()> callback) {
		onCompleteCallback_ = callback;
	}

	///--------------------------------------------------------------
	///                        静的メンバ関数
private:
	void UpdateFade();
	void UpdateSlide();
	void UpdateWipe();
	void UpdateCircle();
	void UpdateDiamondWipe();
	void UpdateCrossFade();
	void UpdateZoom();
	void UpdateCurtain();
	void UpdateVenetianBlinds();
	void UpdateCheckerboard();
	void UpdatePixelDissolve();
	void UpdateSpiral();
	void UpdateClock();

	float EaseInOut(float t);
	float EaseIn(float t);
	float EaseOut(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	SpriteSetup *spriteSetup_ = nullptr;
	std::unique_ptr<Sprite> transitionSprite_ = nullptr;

	// トランジション設定
	TransitionType currentType_ = TransitionType::Fade;
	TransitionState state_ = TransitionState::Idle;
	float duration_ = 1.0f;	   // トランジション時間
	float elapsedTime_ = 0.0f; // 経過時間
	float progress_ = 0.0f;	   // 進行度（0.0〜1.0）

	// 表示設定
	Vector4 transitionColor_ = {0.0f, 0.0f, 0.0f, 1.0f}; // デフォルトは黒
	std::string transitionTexture_ = "";
	bool useTexture_ = false;

	// コールバック
	std::function<void()> onCompleteCallback_ = nullptr;

	// 画面サイズ（初期化時に取得）
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// おしゃれトランジション用の追加スプライト
	std::vector<std::unique_ptr<Sprite>> additionalSprites_;
};

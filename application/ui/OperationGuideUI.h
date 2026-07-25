#pragma once
#include "MagMath.h"
using Vector2 = MagMath::Vector2;
using Vector4 = MagMath::Vector4;
#include "Sprite.h"
#include "SpriteSetup.h"
#include <map>
#include <memory>
#include <string>

namespace MagEngine {
	class Input;
	class RenderWorld;
}

///=============================================================================
///                        コントローラーボタン種類
enum class ControllerButton {
	LeftStick,
	RightStick,
	ButtonA,
	ButtonB,
	ButtonX,
	ButtonY,
	LB,
	RB,
	LT,
	RT,
	DPadUp,
	DPadDown,
	DPadLeft,
	DPadRight,
	Max
};

///=============================================================================
///                        ボタン表示情報
struct ButtonDisplayInfo {
	std::unique_ptr<MagEngine::Sprite> sprite;
	Vector2 basePosition;
	Vector2 baseSize;
	Vector4 normalColor;
	Vector4 pressedColor;
	float currentScale;
	float targetScale;
	bool isPressed;
	float pulseTime;
	std::string labelText;	   // ボタンの説明テキスト
	Vector2 textLabelPosition; // テキストラベル表示位置

	// テキストラベル用スプライト
	std::unique_ptr<MagEngine::Sprite> textSprite;
	Vector2 textBasePosition;
	Vector2 textSize;
	float textAlpha;		// テキストの不透明度
	float textTargetAlpha;	// 目標不透明度
	float textSlideOffset;	// スライドアニメーション用オフセット
	float textTargetOffset; // 目標オフセット
};

///=============================================================================
///                        操作ガイドUIクラス
class OperationGuideUI {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// \brief 操作ガイドのスプライトと入力参照を初期化する
	/// \param spriteSetup スプライト生成に使用するセットアップ
	/// \param input ボタン状態の取得に使用する入力
	void Initialize(MagEngine::SpriteSetup *spriteSetup, MagEngine::Input &input);

	/// \brief 終了処理
	void Finalize();

	/// \brief 入力状態、ボタン表示、展開アニメーションを更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(float unscaledDeltaTime);

	/// \brief 描画
	void Draw();

	/// \brief 表示中の操作ガイドを描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        表示制御
	/// \brief 操作ガイドの表示・非表示を設定する
	/// \param visible trueの場合は表示し、falseの場合は非表示にする
	void SetVisible(bool visible) {
		isVisible_ = visible;
	}

	/// \brief 操作ガイドが表示有効かを取得する
	/// \return 表示有効の場合はtrue、それ以外はfalse
	bool IsVisible() const {
		return isVisible_;
	}

	/// \brief 操作ガイド全体の不透明度を設定する
	/// \param opacity 設定する不透明度
	void SetOpacity(float opacity) {
		opacity_ = opacity;
	}

	/// \brief 操作ガイドの基準表示位置を設定する
	/// \param position 画面上の基準座標
	void SetGuidePosition(const Vector2 &position) {
		guideBasePosition_ = position;
	}

	/// \brief 操作ガイドを展開するアニメーションを開始する
	/// \param duration 展開時間（秒）
	void StartDeployAnimation(float duration = 1.0f);

	/// \brief 操作ガイドを収束するアニメーションを開始する
	/// \param duration 収束時間（秒）
	void StartRetractAnimation(float duration = 0.8f);

	/// \brief 展開または収束アニメーション中かを取得する
	/// \return アニメーション中の場合はtrue、それ以外はfalse
	bool IsAnimating() const {
		return isAnimating_;
	}

	///--------------------------------------------------------------
	///                        プライベート関数
private:
	/// \brief コントローラーボタン表示用スプライトを生成する
	void InitializeButtons();
	/// \brief 現在の入力状態を各ボタン表示へ反映する
	void UpdateButtonStates();
	/// \brief 押下状態に応じたボタン表示を補間する
	/// \param deltaTime 前フレームからの経過時間（秒）
	void UpdateButtonAnimations(float deltaTime);
	/// \brief 展開・収束アニメーションの進行度を更新する
	/// \param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void UpdateDeployAnimation(float unscaledDeltaTime);
	/// \brief 弾性を伴う減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseOutElastic(float t);
	/// \brief 加減速補間を適用する
	/// \param t 補間率
	/// \return 補間後の値
	float EaseInOutQuad(float t);

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト管理
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	MagEngine::Input *input_ = nullptr;
	std::unique_ptr<MagEngine::Sprite> backgroundSprite_ = nullptr; // 背景パネル

	// ボタン情報マップ
	std::map<ControllerButton, ButtonDisplayInfo> buttons_;

	// 表示設定
	bool isVisible_ = true;
	float opacity_ = 0.8f;
	Vector2 guideBasePosition_ = {16.0f, 600.0f}; // デフォルトは左下寄り

	// アニメーション設定
	float pressAnimationSpeed_ = 8.0f;
	float releaseAnimationSpeed_ = 6.0f;
	float pressedScale_ = 1.3f;
	float normalScale_ = 1.0f;

	// 画面サイズ
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// スティック傾き連動
	Vector2 leftStickOffset_ = {0.0f, 0.0f};
	Vector2 currentStickOffset_ = {0.0f, 0.0f};
	float stickMoveRange_ = 15.0f;	   // スティックUIの最大移動範囲
	float stickMoveSmoothing_ = 0.15f; // 移動の滑らかさ

	// グロー効果
	float glowIntensity_ = 0.0f;
	float glowPulseSpeed_ = 5.0f;

	// 展開アニメーション状態
	bool isAnimating_ = false;
	bool isDeploying_ = false;
	float deployAnimationTime_ = 0.0f;
	float deployAnimationDuration_ = 1.0f;
	float deployProgress_ = 0.0f;
	float baseOpacity_ = 0.8f;
	Vector2 baseGuidePosition_ = {16.0f, 600.0f};
};

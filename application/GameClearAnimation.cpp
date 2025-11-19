#define _USE_MATH_DEFINES
#define NOMINMAX
#include "GameClearAnimation.h"
#include "Camera.h"
#include "FollowCamera.h"
#include "ImguiSetup.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

///=============================================================================
///                        初期化
void GameClearAnimation::Initialize(SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = GameClearAnimationState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// 上部バーの作成
	topBar_ = std::make_unique<Sprite>();
	topBar_->Initialize(spriteSetup_, "white1x1.png");
	topBar_->SetColor(barColor_);

	// 下部バーの作成
	bottomBar_ = std::make_unique<Sprite>();
	bottomBar_->Initialize(spriteSetup_, "white1x1.png");
	bottomBar_->SetColor(barColor_);

	// テキストスプライトの作成
	textSprite_ = std::make_unique<Sprite>();
	textSprite_->Initialize(spriteSetup_, textTexture_);
	textSprite_->SetSize(textSize_);
	textSprite_->SetAnchorPoint({0.5f, 0.5f});
}

///=============================================================================
///                        終了処理
void GameClearAnimation::Finalize() {
	topBar_.reset();
	bottomBar_.reset();
	textSprite_.reset();
}

///=============================================================================
///                        更新
void GameClearAnimation::Update() {
	if (state_ == GameClearAnimationState::Idle || state_ == GameClearAnimationState::Done) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;
	elapsedTime_ += deltaTime;

	switch (state_) {
	case GameClearAnimationState::Opening:
		UpdateOpening();
		break;
	case GameClearAnimationState::Showing:
		UpdateShowing();
		break;
	case GameClearAnimationState::CameraUp:
		UpdateCameraUp();
		break;
	case GameClearAnimationState::Closing:
		UpdateClosing();
		break;
	default:
		break;
	}

	// スプライト更新
	if (topBar_) {
		topBar_->Update();
	}
	if (bottomBar_) {
		bottomBar_->Update();
	}
	if (textSprite_) {
		textSprite_->Update();
	}
}

///=============================================================================
///                        各状態の更新処理
void GameClearAnimation::UpdateOpening() {
	float rawProgress = elapsedTime_ / openDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = EaseOut(rawProgress);

	float barHeight = screenHeight_ * barHeightRatio_;

	// シネスコバーが登場
	float topY = -barHeight + barHeight * progress_;
	float bottomY = screenHeight_ - barHeight * progress_;

	topBar_->SetPosition({0.0f, topY});
	topBar_->SetSize({screenWidth_, barHeight});
	topBar_->SetColor(barColor_);

	bottomBar_->SetPosition({0.0f, bottomY});
	bottomBar_->SetSize({screenWidth_, barHeight});
	bottomBar_->SetColor(barColor_);

	// テキストは非表示
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	if (rawProgress >= 1.0f) {
		state_ = GameClearAnimationState::Showing;
		elapsedTime_ = 0.0f;
		progress_ = 0.0f;
	}
}

void GameClearAnimation::UpdateShowing() {
	float rawProgress = elapsedTime_ / showDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = rawProgress;

	float barHeight = screenHeight_ * barHeightRatio_;

	// バーは固定位置
	topBar_->SetPosition({0.0f, 0.0f});
	topBar_->SetSize({screenWidth_, barHeight});
	topBar_->SetColor(barColor_);

	bottomBar_->SetPosition({0.0f, screenHeight_ - barHeight});
	bottomBar_->SetSize({screenWidth_, barHeight});
	bottomBar_->SetColor(barColor_);

	// テキストをフェードイン + 拡大
	float textProgress = std::min(1.0f, progress_ * 2.0f);
	float textAlpha = EaseOut(textProgress);
	float scale = 0.5f + textAlpha * 0.5f;

	Vector4 textColor = {1.0f, 1.0f, 1.0f, textAlpha};
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
	textSprite_->SetColor(textColor);

	if (rawProgress >= 1.0f) {
		state_ = GameClearAnimationState::CameraUp;
		elapsedTime_ = 0.0f;
		progress_ = 0.0f;

		// カメラ上昇演出の準備
		if (followCamera_) {
			cameraStartPosition_ = followCamera_->GetCamera()->GetTransform().translate;
			// 目標位置を計算（現在位置の上方＋後方）
			cameraTargetPosition_ = {
				cameraStartPosition_.x,
				cameraStartPosition_.y + cameraTargetHeight_,
				cameraStartPosition_.z + cameraTargetDistance_};
		}
	}
}

void GameClearAnimation::UpdateCameraUp() {
	float rawProgress = elapsedTime_ / cameraUpDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = EaseInOut(rawProgress);

	float barHeight = screenHeight_ * barHeightRatio_;

	// シネスコバーは維持
	topBar_->SetPosition({0.0f, 0.0f});
	topBar_->SetSize({screenWidth_, barHeight});
	topBar_->SetColor(barColor_);

	bottomBar_->SetPosition({0.0f, screenHeight_ - barHeight});
	bottomBar_->SetSize({screenWidth_, barHeight});
	bottomBar_->SetColor(barColor_);

	// テキストは徐々にフェードアウト
	float textAlpha = 1.0f - progress_;
	float scale = 1.0f - progress_ * 0.2f;
	Vector4 textColor = {1.0f, 1.0f, 1.0f, textAlpha};
	textSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	textSprite_->SetSize({textSize_.x * scale, textSize_.y * scale});
	textSprite_->SetColor(textColor);

	// プレイヤーを正面に飛ばしながら旋回させる
	if (player_) {
		Transform *playerTransform = player_->GetTransform();
		if (playerTransform) {
			// 経過時間に応じた移動量を計算
			float time = progress_ * cameraUpDuration_;

			// 前進（Z+方向）- 加速カーブ
			float forwardProgress = EaseOut(progress_);
			float forwardDistance = flightSpeed_ * time * (1.0f + forwardProgress * 2.0f);

			// 旋回（Y軸回転）- 螺旋を描くように
			float spinAngle = spinRate_ * time;
			float spiralRadius = 3.0f * progress_; // 徐々に外側へ

			// 上昇（Y+方向）- なめらかに上昇
			float climbProgress = EaseInOut(progress_);
			float climbHeight = climbRate_ * climbProgress * cameraUpDuration_;

			// 位置を計算
			playerTransform->translate = {
				playerStartPosition_.x + std::sin(spinAngle) * spiralRadius,
				playerStartPosition_.y + climbHeight,
				playerStartPosition_.z + forwardDistance};

			// 回転を計算（機体を進行方向に向ける + ロール）
			// ピッチ：上昇角度
			float pitchAngle = std::atan2(climbRate_ * climbProgress, flightSpeed_ * (1.0f + forwardProgress));

			// ヨー：旋回方向
			float yawAngle = playerStartRotation_.y + spinAngle;

			// ロール：旋回に応じた傾き（戦闘機らしく）
			float rollAngle = spinRate_ * 0.8f * std::sin(spinAngle * 2.0f); // より激しいロール

			playerTransform->rotate = {
				pitchAngle,
				yawAngle,
				rollAngle};
		}
	}

	// カメラを上昇させる（プレイヤーを追いかけるように）
	if (followCamera_) {
		Vector3 newPos = {
			cameraStartPosition_.x + (cameraTargetPosition_.x - cameraStartPosition_.x) * progress_,
			cameraStartPosition_.y + (cameraTargetPosition_.y - cameraStartPosition_.y) * progress_,
			cameraStartPosition_.z + (cameraTargetPosition_.z - cameraStartPosition_.z) * progress_};

		// 固定位置モードで上昇演出
		followCamera_->SetFixedPosition(newPos);
	}

	if (rawProgress >= 1.0f) {
		state_ = GameClearAnimationState::Closing;
		elapsedTime_ = 0.0f;
		progress_ = 0.0f;
	}
}

void GameClearAnimation::UpdateClosing() {
	float rawProgress = elapsedTime_ / closeDuration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}
	progress_ = EaseIn(rawProgress);

	float barHeight = screenHeight_ * barHeightRatio_;

	// シネスコバーが収束 + フェードアウト
	float effectiveProgress = 1.0f - progress_;
	float topY = -barHeight * (1.0f - effectiveProgress);
	float bottomY = screenHeight_ - barHeight + barHeight * (1.0f - effectiveProgress);

	Vector4 color = barColor_;
	color.w = effectiveProgress;

	topBar_->SetPosition({0.0f, topY});
	topBar_->SetSize({screenWidth_, barHeight});
	topBar_->SetColor(color);

	bottomBar_->SetPosition({0.0f, bottomY});
	bottomBar_->SetSize({screenWidth_, barHeight});
	bottomBar_->SetColor(color);

	// テキストは非表示
	textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});

	if (rawProgress >= 1.0f) {
		state_ = GameClearAnimationState::Done;

		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}
}

///=============================================================================
///                        アニメーション制御
void GameClearAnimation::StartClearAnimation(
	float openDuration, float showDuration, float cameraUpDuration, float closeDuration) {

	state_ = GameClearAnimationState::Opening;
	openDuration_ = openDuration;
	showDuration_ = showDuration;
	cameraUpDuration_ = cameraUpDuration;
	closeDuration_ = closeDuration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;

	// プレイヤーの初期位置と回転を保存
	if (player_) {
		Transform *playerTransform = player_->GetTransform();
		if (playerTransform) {
			playerStartPosition_ = playerTransform->translate;
			playerStartRotation_ = playerTransform->rotate;
		}
	}

	// 初期状態設定
	if (topBar_) {
		topBar_->SetColor(barColor_);
	}
	if (bottomBar_) {
		bottomBar_->SetColor(barColor_);
	}
	if (textSprite_) {
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}
}

void GameClearAnimation::Cancel() {
	state_ = GameClearAnimationState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 全て非表示に
	if (topBar_) {
		Vector4 color = barColor_;
		color.w = 0.0f;
		topBar_->SetColor(color);
	}
	if (bottomBar_) {
		Vector4 color = barColor_;
		color.w = 0.0f;
		bottomBar_->SetColor(color);
	}
	if (textSprite_) {
		textSprite_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	}
}

void GameClearAnimation::Reset() {
	Cancel();
}

///=============================================================================
///                        描画
void GameClearAnimation::Draw() {
	if (state_ == GameClearAnimationState::Idle) {
		return;
	}

	if (topBar_) {
		topBar_->Draw();
	}
	if (bottomBar_) {
		bottomBar_->Draw();
	}
	if (textSprite_) {
		textSprite_->Draw();
	}
}

///=============================================================================
///                        イージング関数
float GameClearAnimation::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float GameClearAnimation::EaseOut(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float GameClearAnimation::EaseIn(float t) {
	return t * t;
}

///=============================================================================
///                        ImGui描画
void GameClearAnimation::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Game Clear Animation");

	// 状態表示
	const char *stateNames[] = {"Idle", "Opening", "Showing", "CameraUp", "Closing", "Done"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);

	ImGui::Separator();

	// タイミング設定
	ImGui::SliderFloat("Open Duration", &openDuration_, 0.1f, 3.0f);
	ImGui::SliderFloat("Show Duration", &showDuration_, 0.5f, 5.0f);
	ImGui::SliderFloat("Camera Up Duration", &cameraUpDuration_, 1.0f, 10.0f);
	ImGui::SliderFloat("Close Duration", &closeDuration_, 0.1f, 3.0f);

	// カメラ設定
	ImGui::Separator();
	ImGui::Text("=== Camera Settings ===");
	ImGui::SliderFloat("Camera Target Height", &cameraTargetHeight_, 5.0f, 50.0f);
	ImGui::SliderFloat("Camera Target Distance", &cameraTargetDistance_, -50.0f, -10.0f);

	// 飛行演出設定
	ImGui::Separator();
	ImGui::Text("=== Flight Settings ===");
	ImGui::SliderFloat("Flight Speed", &flightSpeed_, 5.0f, 30.0f);
	ImGui::SliderFloat("Spin Rate (rad/s)", &spinRate_, 0.5f, 5.0f);
	ImGui::SliderFloat("Climb Rate", &climbRate_, 2.0f, 20.0f);

	// 表示設定
	ImGui::Separator();
	ImGui::ColorEdit4("Bar Color", &barColor_.x);
	ImGui::SliderFloat("Bar Height Ratio", &barHeightRatio_, 0.05f, 0.3f);
	ImGui::InputFloat2("Text Size", &textSize_.x);

	ImGui::Separator();

	// テスト用ボタン
	if (ImGui::Button("Start Clear Animation")) {
		StartClearAnimation(openDuration_, showDuration_, cameraUpDuration_, closeDuration_);
	}

	if (ImGui::Button("Cancel")) {
		Cancel();
	}

	ImGui::End();
#endif
}

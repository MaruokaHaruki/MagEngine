#include "SceneTransition.h"
#include "ImguiSetup.h"
#include <cmath>

///=============================================================================
///                        初期化
void SceneTransition::Initialize(SpriteSetup *spriteSetup) {
	spriteSetup_ = spriteSetup;
	state_ = TransitionState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;

	// 画面サイズを取得
	if (spriteSetup_) {
		screenWidth_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		screenHeight_ = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());
	}

	// トランジション用スプライトの作成（白い1x1テクスチャを使用）
	transitionSprite_ = std::make_unique<Sprite>();
	transitionSprite_->Initialize(spriteSetup_, "WolfOne_Triangle.png");
	transitionSprite_->SetSize({screenWidth_, screenHeight_});
	transitionSprite_->SetPosition({0.0f, 0.0f});
	transitionSprite_->SetColor(transitionColor_);
}

///=============================================================================
///                        終了処理
void SceneTransition::Finalize() {
	transitionSprite_.reset();
}

///=============================================================================
///                        更新
void SceneTransition::Update() {
	if (state_ == TransitionState::Idle || state_ == TransitionState::Completed) {
		return;
	}

	// 経過時間の更新
	const float deltaTime = 1.0f / 60.0f; // 60FPS想定
	elapsedTime_ += deltaTime;

	// 進行度の計算
	float rawProgress = elapsedTime_ / duration_;
	if (rawProgress > 1.0f) {
		rawProgress = 1.0f;
	}

	// イージング適用
	progress_ = EaseInOut(rawProgress);

	// Opening時は進行度を反転（1.0→0.0）
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// トランジションタイプに応じた更新
	switch (currentType_) {
	case TransitionType::Fade:
		UpdateFade();
		break;
	case TransitionType::SlideLeft:
	case TransitionType::SlideRight:
	case TransitionType::SlideUp:
	case TransitionType::SlideDown:
		UpdateSlide();
		break;
	case TransitionType::WipeLeft:
	case TransitionType::WipeRight:
		UpdateWipe();
		break;
	case TransitionType::CircleExpand:
	case TransitionType::CircleShrink:
		UpdateCircle();
		break;
	}

	// トランジション完了チェック
	if (rawProgress >= 1.0f) {
		state_ = TransitionState::Completed;
		if (onCompleteCallback_) {
			onCompleteCallback_();
		}
	}

	// スプライト更新
	if (transitionSprite_) {
		transitionSprite_->Update();
	}
}

///=============================================================================
///                        描画
void SceneTransition::Draw() {
	if (state_ == TransitionState::Idle) {
		return;
	}

	if (transitionSprite_) {
		transitionSprite_->Draw();
	}
}

///=============================================================================
///                        各トランジションタイプの更新
void SceneTransition::UpdateFade() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	Vector4 color = transitionColor_;
	color.w = effectiveProgress; // アルファ値を進行度に応じて変更
	transitionSprite_->SetColor(color);
}

void SceneTransition::UpdateSlide() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	Vector2 position = {0.0f, 0.0f};

	switch (currentType_) {
	case TransitionType::SlideLeft:
		position.x = -screenWidth_ + (screenWidth_ * effectiveProgress);
		break;
	case TransitionType::SlideRight:
		position.x = screenWidth_ - (screenWidth_ * effectiveProgress);
		break;
	case TransitionType::SlideUp:
		position.y = -screenHeight_ + (screenHeight_ * effectiveProgress);
		break;
	case TransitionType::SlideDown:
		position.y = screenHeight_ - (screenHeight_ * effectiveProgress);
		break;
	}

	transitionSprite_->SetPosition(position);
	transitionSprite_->SetColor(transitionColor_);
}

void SceneTransition::UpdateWipe() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	Vector2 size = {screenWidth_, screenHeight_};
	Vector2 position = {0.0f, 0.0f};

	switch (currentType_) {
	case TransitionType::WipeLeft:
		size.x = screenWidth_ * effectiveProgress;
		break;
	case TransitionType::WipeRight:
		size.x = screenWidth_ * effectiveProgress;
		position.x = screenWidth_ - size.x;
		break;
	}

	transitionSprite_->SetSize(size);
	transitionSprite_->SetPosition(position);
	transitionSprite_->SetColor(transitionColor_);
}

void SceneTransition::UpdateCircle() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// 円形トランジションは複数のスプライトで表現するか、
	// カスタムシェーダーが必要なため、ここでは簡易的にフェードで代用
	Vector4 color = transitionColor_;
	color.w = effectiveProgress;
	transitionSprite_->SetColor(color);
}

///=============================================================================
///                        トランジション制御
void SceneTransition::StartClosing(TransitionType type, float duration) {
	currentType_ = type;
	duration_ = duration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	state_ = TransitionState::Closing;

	// テクスチャ使用設定
	if (useTexture_ && !transitionTexture_.empty()) {
		transitionSprite_->SetTexture(transitionTexture_);
	}
}

void SceneTransition::StartOpening(TransitionType type, float duration) {
	currentType_ = type;
	duration_ = duration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	state_ = TransitionState::Opening;

	// テクスチャ使用設定
	if (useTexture_ && !transitionTexture_.empty()) {
		transitionSprite_->SetTexture(transitionTexture_);
	}
}

void SceneTransition::CompleteImmediate() {
	state_ = TransitionState::Completed;
	progress_ = 1.0f;
	if (onCompleteCallback_) {
		onCompleteCallback_();
	}
}

void SceneTransition::Cancel() {
	state_ = TransitionState::Idle;
	progress_ = 0.0f;
	elapsedTime_ = 0.0f;
}

///=============================================================================
///                        イージング関数
float SceneTransition::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float SceneTransition::EaseIn(float t) {
	return t * t;
}

float SceneTransition::EaseOut(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

///=============================================================================
///                        ImGui描画
void SceneTransition::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Scene Transition");

	// 状態表示
	const char *stateNames[] = {"Idle", "Opening", "Closing", "Completed"};
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Text("Progress: %.2f", progress_);
	ImGui::Text("Elapsed Time: %.2f / %.2f", elapsedTime_, duration_);

	ImGui::Separator();

	// トランジションタイプ選択
	const char *typeNames[] = {
		"Fade", "SlideLeft", "SlideRight", "SlideUp", "SlideDown",
		"WipeLeft", "WipeRight", "CircleExpand", "CircleShrink"};
	int currentTypeIndex = static_cast<int>(currentType_);
	if (ImGui::Combo("Transition Type", &currentTypeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
		currentType_ = static_cast<TransitionType>(currentTypeIndex);
	}

	// 時間設定
	ImGui::SliderFloat("Duration", &duration_, 0.1f, 5.0f);

	// 色設定
	ImGui::ColorEdit4("Transition Color", &transitionColor_.x);

	ImGui::Separator();

	// テスト用ボタン
	if (ImGui::Button("Start Closing")) {
		StartClosing(currentType_, duration_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Start Opening")) {
		StartOpening(currentType_, duration_);
	}

	if (ImGui::Button("Complete Immediate")) {
		CompleteImmediate();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		Cancel();
	}

	ImGui::End();
#endif
}
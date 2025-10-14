#define _USE_MATH_DEFINES
#define NOMINMAX
#include "SceneTransition.h"
#include "ImguiSetup.h"
#include <algorithm>
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
	transitionSprite_->Initialize(spriteSetup_, "white1x1.png");
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
	case TransitionType::DiamondWipe:
		UpdateDiamondWipe();
		break;
	case TransitionType::CrossFade:
		UpdateCrossFade();
		break;
	case TransitionType::ZoomIn:
	case TransitionType::ZoomOut:
		UpdateZoom();
		break;
	case TransitionType::Curtain:
		UpdateCurtain();
		break;
	case TransitionType::VenetianBlinds:
		UpdateVenetianBlinds();
		break;
	case TransitionType::Checkerboard:
		UpdateCheckerboard();
		break;
	case TransitionType::PixelDissolve:
		UpdatePixelDissolve();
		break;
	case TransitionType::Spiral:
		UpdateSpiral();
		break;
	case TransitionType::Clock:
		UpdateClock();
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
	for (auto &sprite : additionalSprites_) {
		if (sprite) {
			sprite->Update();
		}
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

	// 追加スプライトの描画
	for (auto &sprite : additionalSprites_) {
		if (sprite) {
			sprite->Draw();
		}
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
///                        おしゃれトランジション更新処理

void SceneTransition::UpdateDiamondWipe() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// ひし形を4つの三角形で表現
	if (additionalSprites_.empty()) {
		for (int i = 0; i < 4; ++i) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(spriteSetup_, "white1x1.png");
			sprite->SetColor(transitionColor_);
			additionalSprites_.push_back(std::move(sprite));
		}
	}

	float centerX = screenWidth_ / 2.0f;
	float centerY = screenHeight_ / 2.0f;
	float diagonal = std::sqrt(screenWidth_ * screenWidth_ + screenHeight_ * screenHeight_);
	float size = diagonal * effectiveProgress;

	// 上
	additionalSprites_[0]->SetPosition({centerX - size / 2.0f, centerY - size / 2.0f});
	additionalSprites_[0]->SetSize({size, size / 2.0f});
	additionalSprites_[0]->SetRotation(0.785f); // 45度

	// 右
	additionalSprites_[1]->SetPosition({centerX, centerY - size / 2.0f});
	additionalSprites_[1]->SetSize({size, size / 2.0f});
	additionalSprites_[1]->SetRotation(-0.785f);

	// 下
	additionalSprites_[2]->SetPosition({centerX - size / 2.0f, centerY});
	additionalSprites_[2]->SetSize({size, size / 2.0f});
	additionalSprites_[2]->SetRotation(-0.785f);

	// 左
	additionalSprites_[3]->SetPosition({centerX - size, centerY - size / 2.0f});
	additionalSprites_[3]->SetSize({size, size / 2.0f});
	additionalSprites_[3]->SetRotation(0.785f);
}

void SceneTransition::UpdateCrossFade() {
	// クロスフェードは通常のフェードと同じ
	UpdateFade();
}

void SceneTransition::UpdateZoom() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	float scale = 1.0f;
	Vector4 color = transitionColor_;

	if (currentType_ == TransitionType::ZoomIn) {
		scale = effectiveProgress * 2.0f; // 0→2倍
		color.w = effectiveProgress;
	} else {							  // ZoomOut
		scale = 2.0f - effectiveProgress; // 2→1倍
		color.w = 1.0f - effectiveProgress;
	}

	float width = screenWidth_ * scale;
	float height = screenHeight_ * scale;

	transitionSprite_->SetSize({width, height});
	transitionSprite_->SetPosition({(screenWidth_ - width) / 2.0f,
									(screenHeight_ - height) / 2.0f});
	transitionSprite_->SetColor(color);
}

void SceneTransition::UpdateCurtain() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// 左右からカーテンが閉じる
	if (additionalSprites_.empty()) {
		for (int i = 0; i < 2; ++i) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(spriteSetup_, "white1x1.png");
			sprite->SetColor(transitionColor_);
			additionalSprites_.push_back(std::move(sprite));
		}
	}

	float halfWidth = (screenWidth_ / 2.0f) * effectiveProgress;

	// 左カーテン
	additionalSprites_[0]->SetPosition({0.0f, 0.0f});
	additionalSprites_[0]->SetSize({halfWidth, screenHeight_});

	// 右カーテン
	additionalSprites_[1]->SetPosition({screenWidth_ - halfWidth, 0.0f});
	additionalSprites_[1]->SetSize({halfWidth, screenHeight_});
}

void SceneTransition::UpdateVenetianBlinds() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	const int blindCount = 8;
	if (additionalSprites_.empty()) {
		for (int i = 0; i < blindCount; ++i) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(spriteSetup_, "white1x1.png");
			sprite->SetColor(transitionColor_);
			additionalSprites_.push_back(std::move(sprite));
		}
	}

	float blindHeight = screenHeight_ / blindCount;
	for (int i = 0; i < blindCount; ++i) {
		float width = screenWidth_ * effectiveProgress;
		additionalSprites_[i]->SetPosition({(screenWidth_ - width) / 2.0f, blindHeight * i});
		additionalSprites_[i]->SetSize({width, blindHeight});
	}
}

void SceneTransition::UpdateCheckerboard() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	const int gridSize = 8;
	const int totalSquares = gridSize * gridSize;

	if (additionalSprites_.empty()) {
		for (int i = 0; i < totalSquares; ++i) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(spriteSetup_, "white1x1.png");
			sprite->SetColor(transitionColor_);
			additionalSprites_.push_back(std::move(sprite));
		}
	}

	float squareWidth = screenWidth_ / gridSize;
	float squareHeight = screenHeight_ / gridSize;

	for (int y = 0; y < gridSize; ++y) {
		for (int x = 0; x < gridSize; ++x) {
			int index = y * gridSize + x;

			// チェッカーパターンで表示タイミングをずらす
			float delay = ((x + y) % 2) * 0.3f;
			float localProgress = std::max(0.0f, std::min(1.0f, (effectiveProgress - delay) * 1.5f));

			Vector4 color = transitionColor_;
			color.w = localProgress;

			additionalSprites_[index]->SetPosition({x * squareWidth, y * squareHeight});
			additionalSprites_[index]->SetSize({squareWidth, squareHeight});
			additionalSprites_[index]->SetColor(color);
		}
	}
}

void SceneTransition::UpdatePixelDissolve() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// ピクセル溶解はランダムなフェードで簡易実装
	Vector4 color = transitionColor_;
	color.w = effectiveProgress;

	// ノイズ効果を追加したい場合はここでスケールを変更
	float scale = 1.0f + (1.0f - effectiveProgress) * 0.1f;
	transitionSprite_->SetSize({screenWidth_ * scale, screenHeight_ * scale});
	transitionSprite_->SetPosition({(screenWidth_ - screenWidth_ * scale) / 2.0f,
									(screenHeight_ - screenHeight_ * scale) / 2.0f});
	transitionSprite_->SetColor(color);
}

void SceneTransition::UpdateSpiral() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	// スパイラル効果（回転しながらズーム）
	float rotation = effectiveProgress * 6.28f * 2.0f; // 2回転
	float scale = effectiveProgress * 1.5f;

	Vector4 color = transitionColor_;
	color.w = effectiveProgress;

	transitionSprite_->SetRotation(rotation);
	transitionSprite_->SetSize({screenWidth_ * scale, screenHeight_ * scale});
	transitionSprite_->SetAnchorPoint({0.5f, 0.5f});
	transitionSprite_->SetPosition({screenWidth_ / 2.0f, screenHeight_ / 2.0f});
	transitionSprite_->SetColor(color);
}

void SceneTransition::UpdateClock() {
	float effectiveProgress = progress_;
	if (state_ == TransitionState::Opening) {
		effectiveProgress = 1.0f - progress_;
	}

	const int segmentCount = 12;
	if (additionalSprites_.empty()) {
		for (int i = 0; i < segmentCount; ++i) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(spriteSetup_, "white1x1.png");
			sprite->SetColor(transitionColor_);
			additionalSprites_.push_back(std::move(sprite));
		}
	}

	float centerX = screenWidth_ / 2.0f;
	float centerY = screenHeight_ / 2.0f;
	float maxRadius = std::sqrt(screenWidth_ * screenWidth_ + screenHeight_ * screenHeight_) / 2.0f;

	for (int i = 0; i < segmentCount; ++i) {
		float angle = (2.0f * 3.14159f / segmentCount) * i - 1.57f; // -90度から開始
		float segmentProgress = std::max(0.0f, std::min(1.0f, effectiveProgress * segmentCount - i));

		Vector4 color = transitionColor_;
		color.w = segmentProgress;

		float width = maxRadius * 2.0f / segmentCount;
		float height = maxRadius * segmentProgress;

		additionalSprites_[i]->SetPosition({centerX + std::cos(angle) * maxRadius / 2.0f - width / 2.0f,
											centerY + std::sin(angle) * maxRadius / 2.0f - height / 2.0f});
		additionalSprites_[i]->SetSize({width, height});
		additionalSprites_[i]->SetRotation(angle + 1.57f);
		additionalSprites_[i]->SetColor(color);
	}
}

///=============================================================================
///                        トランジション制御
void SceneTransition::StartClosing(TransitionType type, float duration) {
	currentType_ = type;
	duration_ = duration;
	elapsedTime_ = 0.0f;
	progress_ = 0.0f;
	state_ = TransitionState::Closing;

	// 追加スプライトをクリア
	additionalSprites_.clear();

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

	// 追加スプライトをクリア
	additionalSprites_.clear();

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
		"WipeLeft", "WipeRight", "CircleExpand", "CircleShrink",
		"DiamondWipe", "CrossFade", "ZoomIn", "ZoomOut",
		"Curtain", "VenetianBlinds", "Checkerboard",
		"PixelDissolve", "Spiral", "Clock"};
	int currentTypeIndex = static_cast<int>(currentType_);
	if (ImGui::Combo("Transition Type", &currentTypeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
		currentType_ = static_cast<TransitionType>(currentTypeIndex);
	}

	// 時間設定
	ImGui::SliderFloat("Duration", &duration_, 0.1f, 5.0f);

	// 色設定
	ImGui::ColorEdit4("Transition Color", &transitionColor_.x);

	ImGui::Separator();

	// おすすめトランジション
	ImGui::Text("Recommended Transitions:");
	if (ImGui::Button("Diamond Wipe")) {
		StartClosing(TransitionType::DiamondWipe, 1.2f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Curtain")) {
		StartClosing(TransitionType::Curtain, 1.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Spiral")) {
		StartClosing(TransitionType::Spiral, 1.5f);
	}

	if (ImGui::Button("Checkerboard")) {
		StartClosing(TransitionType::Checkerboard, 1.2f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clock")) {
		StartClosing(TransitionType::Clock, 1.5f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Venetian Blinds")) {
		StartClosing(TransitionType::VenetianBlinds, 1.0f);
	}

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
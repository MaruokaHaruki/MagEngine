/*********************************************************************
 * \file   PlayerDefeatComponent.cpp
 * \brief  プレイヤー敗北演出コンポーネント実装
 *
 * \author Harukichimaru
 * \date   February 2026
 * \note   敗北時の落下・回転アニメーション演出を制御
 *********************************************************************/
#include "PlayerDefeatComponent.h"
#include "Transform.h"
#include <algorithm>
#include <cmath>

//=============================================================================
// 初期化
void PlayerDefeatComponent::Initialize() {
	isDefeated_ = false;
	defeatAnimationComplete_ = false;
	defeatAnimationTime_ = 0.0f;
	animationDuration_ = 2.0f; // 2秒間の敗北演出

	// 物理パラメータ初期化
	defeatVelocity_ = {0.0f, 0.0f, 0.0f};
	defeatRotationSpeed_ = {0.0f, 0.0f, 0.0f};
	gravity_ = 9.8f;	  // 重力加速度
	deadHeight_ = -10.0f; // -10m以下で演出完了

	animationProgress_ = 0.0f;
}

//=============================================================================
// 敗北演出開始
void PlayerDefeatComponent::StartDefeatAnimation() {
	if (isDefeated_) {
		return; // 既に敗北演出中
	}

	isDefeated_ = true;
	defeatAnimationComplete_ = false;
	defeatAnimationTime_ = 0.0f;
	animationProgress_ = 0.0f;

	// 演出用の疑似ランダム速度を生成
	unsigned int seed = static_cast<unsigned int>(defeatAnimationTime_ * 1000.0f);

	// 横方向の速度（ランダム）
	float randomX = GeneratePseudoRandom(seed, 1, static_cast<int>(50.0f)) - 0.5f;
	float randomZ = GeneratePseudoRandom(seed, 7, static_cast<int>(500.0f)) - 0.1f;

	// 落下速度は固定（重力で加速）
	defeatVelocity_ = {
		randomX * 1.5f,
		0.0f, // 重力で動的に変わる
		randomZ * 0.5f};

	// 回転速度（ランダム）
	defeatRotationSpeed_ = {
		GeneratePseudoRandom(seed, 13, static_cast<int>(1000.0f)) - 0.05f,
		GeneratePseudoRandom(seed, 19, static_cast<int>(333.0f)) - 0.15f,
		GeneratePseudoRandom(seed, 23, static_cast<int>(250.0f)) - 0.02f};
}

//=============================================================================
// 敗北演出更新
void PlayerDefeatComponent::Update(MagMath::Transform *transform, float deltaTime) {
	if (!isDefeated_ || !transform) {
		return;
	}

	UpdateAnimation(transform, deltaTime);
}

//=============================================================================
// 敗北演出アニメーション更新
void PlayerDefeatComponent::UpdateAnimation(MagMath::Transform *transform, float deltaTime) {
	const float kFrameDelta = 1.0f / 60.0f; // 固定フレームタイム

	defeatAnimationTime_ += kFrameDelta;
	animationProgress_ = std::min(defeatAnimationTime_ / animationDuration_, 1.0f);

	// 重力による加速
	defeatVelocity_.y -= gravity_ * kFrameDelta * (1.0f + animationProgress_);

	// 位置更新（移動）
	transform->translate.x += defeatVelocity_.x * kFrameDelta;
	transform->translate.y += defeatVelocity_.y * kFrameDelta;
	transform->translate.z += defeatVelocity_.z * kFrameDelta;

	// 回転更新（加速度付き）
	transform->rotate.x += defeatRotationSpeed_.x * (1.0f + animationProgress_ * 2.0f);
	transform->rotate.y += defeatRotationSpeed_.y * (1.0f + animationProgress_ * 2.0f);
	transform->rotate.z += defeatRotationSpeed_.z * (1.0f + animationProgress_ * 2.0f);

	// 演出完了判定（指定高さに到達 または 時間満了）
	if (transform->translate.y <= deadHeight_ || animationProgress_ >= 1.0f) {
		defeatAnimationComplete_ = true;
	}
}

//=============================================================================
// 疑似ランダム値生成
float PlayerDefeatComponent::GeneratePseudoRandom(
	unsigned int seed,
	int multiplier,
	int divisor) {

	unsigned int value = (seed * multiplier) % 100;
	return static_cast<float>(value) / static_cast<float>(divisor);
}

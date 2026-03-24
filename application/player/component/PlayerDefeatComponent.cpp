/*********************************************************************
 * \file   PlayerDefeatComponent.cpp
 * \brief  プレイヤー敗北演出コンポーネント実装
 *
 * \author Harukichimaru
 * \date   February 2026
 * \note   敗北時の落下・回転アニメーション演出を制御
 *         改良版：自然な墜落演出（機首下向き → 後半回転）
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
	animationDuration_ = 3.0f; // 3秒間の敗北演出

	// 物理パラメータ初期化
	defeatVelocity_ = {0.0f, 0.0f, 0.0f};
	defeatRotationSpeed_ = {0.0f, 0.0f, 0.0f};
	localRotation_ = {0.0f, 0.0f, 0.0f}; // ローカル回転を初期化
	gravity_ = 15.0f;	  // 重力加速度（強め）
	deadHeight_ = -50.0f; // -50m以下で演出完了

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

	// ローカル回転をリセット
	localRotation_ = {0.0f, 0.0f, 0.0f};

	// 初期速度：小さい横移動（ランダム）
	float randomX = (rand() % 100 - 50) * 0.01f; // -0.5 ~ 0.5
	float randomZ = (rand() % 100 - 50) * 0.01f;

	defeatVelocity_ = {
		randomX * 5.0f,  // 小さい横方向速度
		0.0f,			 // 初期はY速度なし（重力で落ちる）
		randomZ * 5.0f};

	// 回転速度：機首を下に向ける（X軸回転）のに使う
	defeatRotationSpeed_ = {
		-30.0f,  // 機首を下向き（ラジアン/秒）
		15.0f,	 // ヨー（左右回転）
		0.0f};	 // ロール（左右転回）
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

	// ===================================================================
	// フェーズ分け：
	// Phase 1 (0.0 ~ 0.6): 機首下向きで加速落下
	// Phase 2 (0.6 ~ 1.0): 回転しながら最終落下
	// ===================================================================
	
	if (animationProgress_ < 0.6f) {
		// === Phase 1: 機首下向き落下 ===
		float phase1Progress = animationProgress_ / 0.6f; // 0.0 ~ 1.0
		
		// 重力による加速
		defeatVelocity_.y -= gravity_ * kFrameDelta;

		// 位置更新（移動）
		transform->translate.x += defeatVelocity_.x * kFrameDelta;
		transform->translate.y += defeatVelocity_.y * kFrameDelta;
		transform->translate.z += defeatVelocity_.z * kFrameDelta;

		// 回転：機首を下に向ける（スムーズに）
		float targetPitchAngle = -1.57f; // -90度（ラジアン）
		localRotation_.x = targetPitchAngle * phase1Progress;
		
		// わずかなヨー（進行方向の変化）
		localRotation_.y = defeatRotationSpeed_.y * phase1Progress * 0.3f;
		
		// ロールはなし
		localRotation_.z = 0.0f;

	} else {
		// === Phase 2: 回転落下 (0.6 ~ 1.0) ===
		float phase2Progress = (animationProgress_ - 0.6f) / 0.4f; // 0.0 ~ 1.0
		
		// 重力による加速を継続
		defeatVelocity_.y -= gravity_ * kFrameDelta;

		// 位置更新（移動）
		transform->translate.x += defeatVelocity_.x * kFrameDelta * 0.8f; // 落下が速くなるので横移動は抑える
		transform->translate.y += defeatVelocity_.y * kFrameDelta;
		transform->translate.z += defeatVelocity_.z * kFrameDelta * 0.8f;

		// 回転：機体がくるくる回転する
		float targetPitchAngle = -1.57f; // 機首下を保持
		localRotation_.x = targetPitchAngle;
		
		// ヨー（左右回転）を加速
		localRotation_.y = defeatRotationSpeed_.y * (0.3f + phase2Progress * 1.5f);
		
		// ロール（左右転回）を追加
		localRotation_.z = phase2Progress * 6.28f; // 1回転（2π）
	}

	// ローカル回転をtransform->rotateに反映
	transform->rotate.x = localRotation_.x;
	transform->rotate.y = localRotation_.y;
	transform->rotate.z = localRotation_.z;

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

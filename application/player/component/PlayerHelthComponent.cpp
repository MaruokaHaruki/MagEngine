#include "PlayerHelthComponent.h"
#include <algorithm>

//=============================================================================
// 初期化
void PlayerHelthComponent::Initialize(int maxHP) {
	maxHP_ = maxHP;
	currentHP_ = maxHP_;
	isInvincible_ = false;
	invincibleTime_ = 0.0f;
	maxInvincibleTime_ = 1.0f;
}

//=============================================================================
// 更新
void PlayerHelthComponent::Update(float deltaTime) {
	// 無敵時間の更新
	if (isInvincible_) {
		invincibleTime_ -= deltaTime;
		if (invincibleTime_ <= 0.0f) {
			invincibleTime_ = 0.0f;
			isInvincible_ = false;
		}
	}
}

//=============================================================================
// ダメージ処理
void PlayerHelthComponent::TakeDamage(int damage) {
	// 無敵状態または既に死亡している場合はダメージを受けない
	if (isInvincible_ || !IsAlive()) {
		return;
	}

	// ダメージを適用
	currentHP_ = std::max(0, currentHP_ - damage);

	// ダメージを受けた場合は無敵状態にする
	if (damage > 0 && IsAlive()) {
		isInvincible_ = true;
		invincibleTime_ = maxInvincibleTime_;
	}
}

//=============================================================================
// 回復処理
void PlayerHelthComponent::Heal(int healAmount) {
	if (!IsAlive()) {
		return;
	}

	currentHP_ = std::min(maxHP_, currentHP_ + healAmount);
}

//=============================================================================
// HP初期化
void PlayerHelthComponent::ResetHP() {
	currentHP_ = maxHP_;
	isInvincible_ = false;
	invincibleTime_ = 0.0f;
}

//=============================================================================
// 最大HP設定
void PlayerHelthComponent::SetMaxHP(int maxHP) {
	maxHP_ = maxHP;
	currentHP_ = std::min(currentHP_, maxHP_);
}

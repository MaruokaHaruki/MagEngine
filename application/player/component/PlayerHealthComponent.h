#pragma once

///=============================================================================
///						HP管理コンポーネント
/// @brief プレイヤーの体力（HP）管理を担当するコンポーネント
/// @details ダメージ処理、回復処理、無敵時間管理を行う
class PlayerHealthComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize(int maxHP);
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        HP操作
	void TakeDamage(int damage);
	void Heal(int healAmount);
	void ResetHP();

	///--------------------------------------------------------------
	///                        ゲッター
	int GetCurrentHP() const {
		return currentHP_;
	}
	int GetMaxHP() const {
		return maxHP_;
	}
	float GetHPRatio() const {
		return static_cast<float>(currentHP_) / maxHP_;
	}
	bool IsAlive() const {
		return currentHP_ > 0;
	}
	bool IsInvincible() const {
		return isInvincible_;
	}
	float GetInvincibleTime() const {
		return invincibleTime_;
	}

	///--------------------------------------------------------------
	///                        セッター
	void SetMaxHP(int maxHP);

	void SetBarrelRollInvincible(bool invincible);

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	int currentHP_;			  // 現在のHP
	int maxHP_;				  // 最大HP
	bool isInvincible_;		  // 無敵状態フラグ
	float invincibleTime_;	  // 無敵時間
	float maxInvincibleTime_; // 最大無敵時間
};

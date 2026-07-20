#pragma once

///=============================================================================
///						HP管理コンポーネント
/// @brief プレイヤーの体力（HP）管理を担当するコンポーネント
/// @details ダメージ、回復、被弾直後の無敵時間を一箇所で管理する。
///          Player本体は死亡状態や演出の判断に集中するため、HPの範囲保証はこのクラスで行う。
class PlayerHealthComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief HPと被弾無敵状態を初期化する
	/// @param maxHP 初期値かつ上限となるHP。GetHPRatio()を利用するため0より大きい値を指定する。
	void Initialize(int maxHP);
	/// @brief 被弾無敵時間を進める
	/// @param deltaTime 前フレームからの経過秒数
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        HP操作
	/// @brief ダメージを適用する
	/// @details 無敵中または死亡後は何もしない。生存中に正のダメージを受けた場合だけ無敵時間を開始する。
	void TakeDamage(int damage);
	/// @brief HPを回復する
	/// @note 死亡後の復帰は担当しない。復帰が必要な場合はResetHP()を使用する。
	void Heal(int healAmount);
	/// @brief HPと無敵状態を初期化直後の状態へ戻す
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
		// NOTE: Initialize()でmaxHP_を正値に設定済みであることを呼び出し側との契約とする。
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
	/// @brief 最大HPを変更し、現在HPを新しい上限内に収める
	/// @note 下限値の補正は行わないため、GetHPRatio()を使う間は正の値を指定する。
	void SetMaxHP(int maxHP);

	/// @brief バレルロール由来の無敵状態を設定する
	/// @note falseにすると被弾無敵タイマーも解除する。通常の被弾無敵との重ね合わせは扱わない。
	void SetBarrelRollInvincible(bool invincible);

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	int currentHP_;              // 被弾・回復時に0からmaxHP_へクランプする現在HP
	int maxHP_;                  // HP比率計算の分母にも使う上限値
	bool isInvincible_;          // 被弾とバレルロールで共有する無敵状態
	float invincibleTime_;       // 自動解除までの残り秒数
	float maxInvincibleTime_;    // 被弾時に設定する無敵秒数
};

#pragma once

///=============================================================================
///					Just Avoidance（ジャスト回避）コンポーネント
/// @brief プレイヤーの精密な回避機能を管理するコンポーネント
/// @details 敵の攻撃が迫ってくる直前の一瞬のタイミングで
///          バレルロールを成功させると、ボーストゲージが回復される
class PlayerJustAvoidanceComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	void Initialize();
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        Just Avoidance判定
	/// @brief ダメージがきそうな敵弾を登録
	void RegisterIncomingDamage();
	
	/// @brief バレルロール実行を通知
	void OnBarrelRollStarted();
	
	/// @brief ジャスト回避の判定と報酬付与
	/// @param outWasJustAvoidance ジャスト回避が成功したか
	/// @return 報酬ブーストゲージ量
	float CheckJustAvoidanceSuccess(bool &outWasJustAvoidance);

	///--------------------------------------------------------------
	///                        ゲッター
	bool IsInJustAvoidanceWindow() const;
	float GetJustAvoidanceWindowTimeRemaining() const;
	float GetJustAvoidanceSuccessRate() const; // 直近のジャスト回避成功率（0-1）

	///--------------------------------------------------------------
	///                        セッター
	/// @brief Just Avoidanceウィンドウのサイズを設定（秒）
	void SetJustAvoidanceWindowSize(float windowSize) {
		justAvoidanceWindowSize_ = windowSize;
	}
	
	/// @brief 攻撃通知のタイムアウトを設定（秒）
	void SetIncomingDamageTimeout(float timeout) {
		incomingDamageTimeout_ = timeout;
	}
	
	/// @brief ジャスト回避成功時のボーストゲージ回復量を設定
	void SetJustAvoidanceBoostReward(float reward) {
		justAvoidanceBoostReward_ = reward;
	}

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	
	// Just Avoidance実行関連
	float incomingDamageTime_;		  // 敵の攻撃が迫ってきた時刻
	bool hasIncomingDamage_;		  // 迫ってくる攻撃があるフラグ
	float lastBarrelRollStartTime_;   // 最後にバレルロールを開始した時刻

	// Just Avoidanceウィンドウ（秒）
	// この時間範囲内でバレルロールを開始すると成功扱い
	float justAvoidanceWindowSize_;	  // デフォルト: 0.3秒
	float incomingDamageTimeout_;	  // 攻撃通知の有効期限（デフォルト: 0.5秒）

	// 報酬
	float justAvoidanceBoostReward_; // 成功時のボーストゲージ回復量（デフォルト: 30.0f）

	// パフォーマンス追跡用
	float lastSuccessRate_; // 直近のジャスト回避の精度（0-1、1.0が完璧）
};

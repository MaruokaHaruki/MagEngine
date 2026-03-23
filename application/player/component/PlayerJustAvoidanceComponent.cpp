#include "PlayerJustAvoidanceComponent.h"
#include <algorithm>

//=============================================================================
// 初期化
void PlayerJustAvoidanceComponent::Initialize() {
	hasIncomingDamage_ = false;
	incomingDamageTime_ = 0.0f;
	lastBarrelRollStartTime_ = -999.0f;

	// Just Avoidanceウィンドウサイズ（秒）
	// 0.3秒間のウィンドウで敵攻撃が迫ってくる直前を狙う
	justAvoidanceWindowSize_ = 0.3f;
	
	// 敵の攻撃通知有効期限
	incomingDamageTimeout_ = 0.5f;

	// ジャスト回避成功時のボーストゲージ報酬
	justAvoidanceBoostReward_ = 30.0f;

	lastSuccessRate_ = 0.0f;
}

//=============================================================================
// 更新
void PlayerJustAvoidanceComponent::Update(float deltaTime) {
	// 敵攻撃通知が古い場合はクリア
	if (hasIncomingDamage_) {
		float timeSinceWarning = deltaTime; // 簡易実装：毎フレーム次フレームの時間加算
		if (timeSinceWarning > incomingDamageTimeout_) {
			hasIncomingDamage_ = false;
		}
	}
}

//=============================================================================
// 敵の攻撃が迫ってくることを登録
void PlayerJustAvoidanceComponent::RegisterIncomingDamage() {
	hasIncomingDamage_ = true;
	incomingDamageTime_ = 0.0f;
}

//=============================================================================
// バレルロール開始を通知
void PlayerJustAvoidanceComponent::OnBarrelRollStarted() {
	lastBarrelRollStartTime_ = 0.0f; // 現在の時刻をセット
}

//=============================================================================
// Just Avoidance成功判定
float PlayerJustAvoidanceComponent::CheckJustAvoidanceSuccess(bool &outWasJustAvoidance) {
	outWasJustAvoidance = false;
	float boostReward = 0.0f;

	// 条件1: 敵からの攻撃通知がある
	if (!hasIncomingDamage_) {
		return 0.0f;
	}

	// 条件2: バレルロールが最近開始された
	float timeSinceBarrelRoll = std::abs(lastBarrelRollStartTime_ - incomingDamageTime_);
	
	// 条件3: バレルロール開始がウィンドウ内のタイミング
	if (timeSinceBarrelRoll <= justAvoidanceWindowSize_) {
		outWasJustAvoidance = true;
		boostReward = justAvoidanceBoostReward_;

		// 成功率を計算（ウィンドウ内のどこで成功したか）
		// 1.0 = 完璧、0.5 = ウィンドウの端
		lastSuccessRate_ = 1.0f - (timeSinceBarrelRoll / justAvoidanceWindowSize_) * 0.5f;

		// クリアして次のチャンスに備える
		hasIncomingDamage_ = false;
	}

	return boostReward;
}

//=============================================================================
// Just Avoidanceウィンドウ内かどうか
bool PlayerJustAvoidanceComponent::IsInJustAvoidanceWindow() const {
	if (!hasIncomingDamage_) {
		return false;
	}

	float timeSinceBarrelRoll = std::abs(lastBarrelRollStartTime_ - incomingDamageTime_);
	return timeSinceBarrelRoll <= justAvoidanceWindowSize_;
}

//=============================================================================
// ウィンドウ内の残り時間を取得
float PlayerJustAvoidanceComponent::GetJustAvoidanceWindowTimeRemaining() const {
	if (!hasIncomingDamage_) {
		return 0.0f;
	}

	float timeSinceBarrelRoll = std::abs(lastBarrelRollStartTime_ - incomingDamageTime_);
	float remaining = justAvoidanceWindowSize_ - timeSinceBarrelRoll;
	
	return std::max(0.0f, remaining);
}

//=============================================================================
// 直近のジャスト回避成功率を取得
float PlayerJustAvoidanceComponent::GetJustAvoidanceSuccessRate() const {
	return lastSuccessRate_;
}

#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "PlayerJustAvoidanceComponent.h"
#include <algorithm>
#include <cmath>

//=============================================================================
// 初期化
void PlayerJustAvoidanceComponent::Initialize() {
	hasIncomingBullet_ = false;
	lastIncomingBulletTime_ = 0.0f;
	lastBarrelRollStartTime_ = -999.0f;
	lastBulletPosition_ = {0.0f, 0.0f, 0.0f};
	lastBulletPreviousPos_ = {0.0f, 0.0f, 0.0f};

	//! ===== ジャスト判定パラメータ =====
	// ウィンドウサイズは0.25秒
	// プレイヤーがバレルロールを開始してから、このウィンドウ内に敵弾が最接近したら成功
	justAvoidanceWindowSize_ = 0.25f;

	// 敵弾検出範囲：この距離以内で接近中なら登録
	detectionRadius_ = 5.0f;

	// 検出状態の有効期限
	detectionTimeout_ = 0.5f;

	//! ===== スロー演出パラメータ =====
	slowTimer_ = 0.0f;
	slowDuration_ = 0.5f; // スロー演出は0.5秒間継続
	slowStrength_ = 0.5f; // 時間が半速になる（倍率 0.5 = 50%スロー）

	//! ===== 機体強化バフパラメータ =====
	powerUpTimer_ = 0.0f;
	powerUpDuration_ = 2.0f;  // 強化バフは2秒間継続
	attackMultiplier_ = 1.5f; // 攻撃力1.5倍
	speedMultiplier_ = 1.3f;  // 移動速度1.3倍

	lastSuccessRate_ = 0.0f;
}

//=============================================================================
// 更新
void PlayerJustAvoidanceComponent::Update(float deltaTime) {
	//! ===== 敵弾検出状態の更新 =====
	// 検出状態が古い場合はクリア
	if (hasIncomingBullet_) {
		lastIncomingBulletTime_ += deltaTime;
		if (lastIncomingBulletTime_ > detectionTimeout_) {
			hasIncomingBullet_ = false;
		}
	}

	//! ===== スロー演出の更新 =====
	if (slowTimer_ > 0.0f) {
		slowTimer_ -= deltaTime;
		if (slowTimer_ < 0.0f) {
			slowTimer_ = 0.0f;
		}
	}

	//! ===== 機体強化バフの更新 =====
	if (powerUpTimer_ > 0.0f) {
		powerUpTimer_ -= deltaTime;
		if (powerUpTimer_ < 0.0f) {
			powerUpTimer_ = 0.0f;
		}
	}
}

//=============================================================================
// 敵弾を検出・登録（敵弾クラスから呼び出し）
void PlayerJustAvoidanceComponent::RegisterIncomingBullet(
	const Vector3 &bulletPosition,
	const Vector3 &bulletPreviousPosition,
	float bulletRadius,
	const Vector3 &playerPosition) {

	//! ===== 1. 距離チェック: プレイヤーの近くにいるか =====
	float distance = CalculateBulletDistance(bulletPosition, playerPosition);
	if (distance > detectionRadius_) {
		return; // 遠すぎる敵弾は無視
	}

	//! ===== 2. 接近判定: 敵弾がプレイヤーに向かってきているか =====
	if (!IsBulletApproaching(bulletPosition, bulletPreviousPosition, playerPosition)) {
		return; // 遠ざかっている敵弾は無視
	}

	//! ===== 登録 =====
	hasIncomingBullet_ = true;
	lastIncomingBulletTime_ = 0.0f;
	lastBulletPosition_ = bulletPosition;
	lastBulletPreviousPos_ = bulletPreviousPosition;
}

//=============================================================================
// バレルロール開始を通知
void PlayerJustAvoidanceComponent::OnBarrelRollStarted() {
	lastBarrelRollStartTime_ = 0.0f; // 現在の時刻をセット
}

//=============================================================================
// ジャスト回避成功判定
float PlayerJustAvoidanceComponent::CheckJustAvoidanceSuccess(
	bool &outWasJustAvoidance,
	float &outSuccessRate) {

	outWasJustAvoidance = false;
	outSuccessRate = 0.0f;
	float slowStrength = 0.0f;

	//! ===== 条件1: 検出中の敵弾があるか =====
	if (!hasIncomingBullet_) {
		return 0.0f;
	}

	//! ===== 条件2: バレルロールが最近開始されたか =====
	float timeSinceBarrelRoll = lastBarrelRollStartTime_;
	if (timeSinceBarrelRoll < 0.0f || timeSinceBarrelRoll > detectionTimeout_) {
		return 0.0f; // タイムアウト
	}

	//! ===== 条件3: バレルロール開始がウィンドウ内のタイミングか =====
	if (timeSinceBarrelRoll <= justAvoidanceWindowSize_) {
		outWasJustAvoidance = true;

		//! ===== 成功率計算 =====
		// ウィンドウの中心（0秒）が最も完璧、端に行くほど低下
		// ウィンドウの中心を狙い気味に設計する場合は調整可能
		outSuccessRate = 1.0f - (timeSinceBarrelRoll / justAvoidanceWindowSize_) * 0.3f;
		outSuccessRate = outSuccessRate > 0.7f ? outSuccessRate : 0.7f; // 最低でも70%

		//! ===== スロー演出を開始 =====
		slowTimer_ = slowDuration_;
		slowStrength = slowStrength_;

		//! ===== 機体強化バフを開始 =====
		powerUpTimer_ = powerUpDuration_;

		//! ===== 次の判定に備える =====
		hasIncomingBullet_ = false;
		lastSuccessRate_ = outSuccessRate;
	}

	return slowStrength;
}

//=============================================================================
// ジャスト判定ウィンドウ内かどうか（UI表示用）
bool PlayerJustAvoidanceComponent::IsInJustAvoidanceWindow() const {
	if (!hasIncomingBullet_) {
		return false;
	}

	float timeSinceBarrelRoll = lastBarrelRollStartTime_;
	return timeSinceBarrelRoll >= 0.0f && timeSinceBarrelRoll <= justAvoidanceWindowSize_;
}

//=============================================================================
// ウィンドウ内の残り時間を取得（UI表示用）
float PlayerJustAvoidanceComponent::GetJustAvoidanceWindowTimeRemaining() const {
	if (!hasIncomingBullet_) {
		return 0.0f;
	}

	float timeSinceBarrelRoll = lastBarrelRollStartTime_;
	float remaining = justAvoidanceWindowSize_ - timeSinceBarrelRoll;

	return remaining > 0.0f ? remaining : 0.0f;
}

//=============================================================================
// 内部ヘルパー関数：敵弾が接近中か判定
bool PlayerJustAvoidanceComponent::IsBulletApproaching(
	const Vector3 &bulletPos,
	const Vector3 &bulletPrevPos,
	const Vector3 &playerPos) const {

	Vector3 bulletDiff = bulletPos - playerPos;
	Vector3 bulletPrevDiff = bulletPrevPos - playerPos;

	float currentDist = std::sqrt(bulletDiff.x * bulletDiff.x +
								  bulletDiff.y * bulletDiff.y +
								  bulletDiff.z * bulletDiff.z);

	float previousDist = std::sqrt(bulletPrevDiff.x * bulletPrevDiff.x +
								   bulletPrevDiff.y * bulletPrevDiff.y +
								   bulletPrevDiff.z * bulletPrevDiff.z);

	// 距離が減少していたら接近中
	return currentDist < previousDist;
}

//=============================================================================
// 内部ヘルパー関数：敵弾との距離を計算
float PlayerJustAvoidanceComponent::CalculateBulletDistance(const Vector3 &bulletPos, const Vector3 &playerPos) const {
	Vector3 diff = bulletPos - playerPos;
	return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

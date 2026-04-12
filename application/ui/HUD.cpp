#define _USE_MATH_DEFINES
#define NOMINMAX
#include "HUD.h"
#include "EnemyBase.h"
#include "EnemyManager.h"
#include "Camera.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

const float SCREEN_WIDTH = 1280.0f;
const float SCREEN_HEIGHT = 720.0f;

///=============================================================================
///                        初期化
void HUD::Initialize() {
	// HUD基本設定
	hudScale_ = 0.85f;
	hudDistance_ = 20.0f;
	hudSizeX_ = 0.4f;
	hudSizeY_ = 0.28f;

	// モダンカラーパレット（視認性重視）
	hudColor_ = {0.0f, 1.0f, 0.3f, 0.95f};		  // 鮮やかな緑
	hudColorWarning_ = {1.0f, 0.8f, 0.0f, 0.95f}; // 明るい黄色
	hudColorCritical_ = {1.0f, 0.2f, 0.0f, 1.0f}; // 鮮やかな赤
	hudColorCyan_ = {0.0f, 0.9f, 1.0f, 0.8f};	  // シアン（アクセント）

	// オフセット・カメラ
	boresightOffset_ = rollScaleOffset_ = {0.0f, -3.5f, 0.0f};
	followCamera_ = nullptr;

	// プレイヤー・カメラ参照
	currentPlayer_ = nullptr;
	currentCamera_ = nullptr;

	// 表示制御（デフォルト全ON）
	showBoresight_ = showRollScale_ = showCompass_ = true;
	showGForce_ = showVelocityVector_ = showFlightPath_ = showPitchLadder_ = true;
	showLockOnReticle_ = true;
	showEnemyIndicators_ = true;

	// ロックオン情報
	lockOnTarget_ = nullptr;
	lockOnRange_ = 30.0f;
	lockOnFOV_ = 60.0f;
	lockedEnemyCount_ = 0;

	// データ初期化
	playerPosition_ = playerRotation_ = playerVelocity_ = {0.0f, 0.0f, 0.0f};
	currentGForce_ = 1.0f;
	currentSpeed_ = currentAltitude_ = currentHeading_ = 0.0f;
	currentBoostGauge_ = maxBoostGauge_ = 100.0f;
	isBarrelRolling_ = false;
	barrelRollProgress_ = 0.0f;

	// アニメーション
	isAnimating_ = isDeploying_ = false;
	animationTime_ = 0.0f;
	animationDuration_ = 1.2f;
	deployProgress_ = 0.0f;

	// ジャスト回避表示の初期化
	justAvoidanceDisplayActive_ = false;
	justAvoidanceNotificationTimer_ = 0.0f;
	justAvoidanceNotificationDuration_ = 1.2f; // 1.2秒間表示
	justAvoidanceSuccessRate_ = 0.0f;

	// 展開タイミング（高速化）
	frameDeployStart_ = 0.0f;
	boresightDeployStart_ = 0.08f;
	pitchLadderDeployStart_ = 0.12f;
	velocityVectorDeployStart_ = 0.16f;
	rollScaleDeployStart_ = 0.2f;
	headingTapeDeployStart_ = 0.3f;
	gForceDeployStart_ = 0.35f;
	boostBarrelDeployStart_ = 0.4f;
}

///=============================================================================
///                        アニメーション開始
void HUD::StartDeployAnimation(float duration) {
	isAnimating_ = true;
	isDeploying_ = true;
	animationTime_ = 0.0f;
	animationDuration_ = duration;
}

void HUD::StartRetractAnimation(float duration) {
	isAnimating_ = true;
	isDeploying_ = false;
	animationTime_ = 0.0f;
	animationDuration_ = duration;
}

///=============================================================================
///                        アニメーション更新
void HUD::UpdateAnimation() {
	if (!isAnimating_) {
		return;
	}

	animationTime_ += 1.0f / 60.0f;
	float rawProgress = animationTime_ / animationDuration_;
	rawProgress = std::min(rawProgress, 1.0f);
	float easedProgress = EaseOutCubic(rawProgress);

	if (isDeploying_) {
		deployProgress_ = easedProgress;
	} else {
		deployProgress_ = 1.0f - easedProgress;
	}

	if (rawProgress >= 1.0f) {
		isAnimating_ = false;
		deployProgress_ = isDeploying_ ? 1.0f : 0.0f;
	}
}

///=============================================================================
///                        展開進行度取得
float HUD::GetDeployProgress() const {
	return deployProgress_;
}

///=============================================================================
///                        イージング関数
float HUD::EaseOutCubic(float t) const {
	float f = t - 1.0f;
	return f * f * f + 1.0f;
}

///=============================================================================
///                        FollowCameraの設定
void HUD::SetFollowCamera(FollowCamera *followCamera) {
	followCamera_ = followCamera;
}

///=============================================================================
///                        視野内制限
Vector3 HUD::ClampHUDPosition(const Vector3 &worldPos, const Vector3 &cameraPos, const Vector3 &cameraForward) {
	Vector3 relativePos = worldPos - cameraPos;
	float depthDistance = Dot(relativePos, cameraForward);

	if (depthDistance <= 0.0f) {
		return cameraPos + cameraForward * hudDistance_;
	}

	return worldPos;
}

///=============================================================================
///                        スクリーン座標変換
Vector3 HUD::GetHUDPosition(float screenX, float screenY) {
	MagEngine::Camera *currentCamera = nullptr;

	if (followCamera_) {
		currentCamera = followCamera_->GetCamera();
	}

	if (!currentCamera) {
		currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
	}

	if (!currentCamera) {
		return {screenX, screenY, hudDistance_};
	}

	// カメラの位置と回転を取得
	Vector3 cameraPos = currentCamera->GetTransform().translate;
	Vector3 cameraRot = currentCamera->GetTransform().rotate;

	// カメラの前方ベクトル（Z軸正方向が前方）
	Vector3 forward = {
		sinf(cameraRot.y) * cosf(cameraRot.x),
		-sinf(cameraRot.x),
		cosf(cameraRot.y) * cosf(cameraRot.x)};

	// カメラの右ベクトル（X軸正方向）
	Vector3 right = {
		cosf(cameraRot.y),
		0.0f,
		-sinf(cameraRot.y)};

	// カメラの上ベクトル（Y軸正方向）
	Vector3 up = {
		sinf(cameraRot.y) * sinf(cameraRot.x),
		cosf(cameraRot.x),
		cosf(cameraRot.y) * sinf(cameraRot.x)};

	// HUDの基準位置（カメラから前方に一定距離）
	Vector3 hudCenter = {
		cameraPos.x + forward.x * hudDistance_,
		cameraPos.y + forward.y * hudDistance_,
		cameraPos.z + forward.z * hudDistance_};

	// スクリーン座標をワールド座標に変換（HUD縦横サイズを個別に適用）
	float scaledX = screenX * hudSizeX_;
	float scaledY = screenY * hudSizeY_;

	Vector3 worldPos = {
		hudCenter.x + right.x * scaledX + up.x * scaledY,
		hudCenter.y + right.y * scaledX + up.y * scaledY,
		hudCenter.z + right.z * scaledX + up.z * scaledY};

	return ClampHUDPosition(worldPos, cameraPos, forward);
}

///=============================================================================
///                        プレイヤー正面座標変換（ガンボアサイト専用）
Vector3 HUD::GetPlayerFrontPosition(float screenX, float screenY) {
	// プレイヤーデータがない場合はカメラベース
	if (playerPosition_.x == 0.0f && playerPosition_.y == 0.0f && playerPosition_.z == 0.0f) {
		return GetHUDPosition(screenX, screenY);
	}

	// プレイヤーのY軸回転のみを使用（機首方向）
	float playerYaw = playerRotation_.y;

	// プレイヤーの前方ベクトル（Y軸回転のみ、傾きは無視）
	Vector3 forward = {
		sinf(playerYaw),
		0.0f, // Y成分は0で水平を保つ
		cosf(playerYaw)};

	// プレイヤーの右ベクトル（水平面での右方向）
	Vector3 right = {
		cosf(playerYaw),
		0.0f,
		-sinf(playerYaw)};

	// 上ベクトル（常にワールドY軸正方向）
	Vector3 up = {0.0f, 1.0f, 0.0f};

	// ガンボアサイトの基準位置（プレイヤーから前方に一定距離、水平に配置）
	Vector3 boresightCenter = {
		playerPosition_.x + forward.x * hudDistance_,
		playerPosition_.y + hudDistance_ * 0.1f, // プレイヤーより少し上に配置
		playerPosition_.z + forward.z * hudDistance_};

	// スクリーン座標をワールド座標に変換（HUD縦横サイズを個別に適用）
	float scaledX = screenX * hudSizeX_;
	float scaledY = screenY * hudSizeY_;

	Vector3 worldPos = {
		boresightCenter.x + right.x * scaledX + up.x * scaledY,
		boresightCenter.y + right.y * scaledX + up.y * scaledY,
		boresightCenter.z + right.z * scaledX + up.z * scaledY};

	return worldPos;
}

///=============================================================================
///                        プレイヤー正面座標変換（オフセット付き）
Vector3 HUD::GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset) {
	// プレイヤーデータがない場合はカメラベース
	if (playerPosition_.x == 0.0f && playerPosition_.y == 0.0f && playerPosition_.z == 0.0f) {
		return GetHUDPosition(screenX + offset.x, screenY + offset.y);
	}

	// プレイヤーのY軸回転のみを使用（機首方向）
	float playerYaw = playerRotation_.y;

	// プレイヤーの前方ベクトル（Y軸回転のみ、傾きは無視）
	Vector3 forward = {
		sinf(playerYaw),
		0.0f, // Y成分は0で水平を保つ
		cosf(playerYaw)};

	// プレイヤーの右ベクトル（水平面での右方向）
	Vector3 right = {
		cosf(playerYaw),
		0.0f,
		-sinf(playerYaw)};

	// 上ベクトル（常にワールドY軸正方向）
	Vector3 up = {0.0f, 1.0f, 0.0f};

	// ベース位置（プレイヤーから前方に一定距離、水平に配置）
	Vector3 baseCenter = {
		playerPosition_.x + forward.x * hudDistance_,
		playerPosition_.y + hudDistance_ * 0.1f, // プレイヤーより少し上に配置
		playerPosition_.z + forward.z * hudDistance_};

	// オフセットを適用
	Vector3 offsetCenter = {
		baseCenter.x + right.x * offset.x * hudSizeX_ + up.x * offset.y * hudSizeY_,
		baseCenter.y + right.y * offset.x * hudSizeX_ + up.y * offset.y * hudSizeY_,
		baseCenter.z + right.z * offset.x * hudSizeX_ + up.z * offset.y * hudSizeY_};

	// スクリーン座標をワールド座標に変換（HUD縦横サイズを個別に適用）
	float scaledX = screenX * hudSizeX_;
	float scaledY = screenY * hudSizeY_;

	Vector3 worldPos = {
		offsetCenter.x + right.x * scaledX + up.x * scaledY,
		offsetCenter.y + right.y * scaledX + up.y * scaledY,
		offsetCenter.z + right.z * scaledX + up.z * scaledY};

	return worldPos;
}

///=============================================================================
///                        更新
void HUD::Update(const Player *player) {
	if (!player)
		return;

	// プレイヤー・カメラ参照を保存
	currentPlayer_ = player;
	if (followCamera_) {
		currentCamera_ = followCamera_->GetCamera();
	} else {
		currentCamera_ = CameraManager::GetInstance()->GetCurrentCamera();
	}

	UpdateAnimation();

	playerPosition_ = player->GetPosition();

	Object3d *playerObj = player->GetObject3d();
	if (playerObj && playerObj->GetTransform()) {
		playerRotation_ = playerObj->GetTransform()->rotate;
	}

	// 弾発射方向の取得（新規）
	bulletFireDirection_ = player->GetBulletFireDirection();

	static Vector3 previousPosition = playerPosition_;
	playerVelocity_ = {
		(playerPosition_.x - previousPosition.x) * 60.0f,
		(playerPosition_.y - previousPosition.y) * 60.0f,
		(playerPosition_.z - previousPosition.z) * 60.0f};
	previousPosition = playerPosition_;

	currentSpeed_ = sqrtf(playerVelocity_.x * playerVelocity_.x +
						  playerVelocity_.y * playerVelocity_.y +
						  playerVelocity_.z * playerVelocity_.z);

	static float previousSpeed = currentSpeed_;
	float acceleration = (currentSpeed_ - previousSpeed) * 60.0f;
	currentGForce_ = 1.0f + acceleration / 9.8f;
	previousSpeed = currentSpeed_;

	currentAltitude_ = playerPosition_.y;

	currentHeading_ = RadiansToDegrees(playerRotation_.y);
	while (currentHeading_ < 0.0f)
		currentHeading_ += 360.0f;
	while (currentHeading_ >= 360.0f)
		currentHeading_ -= 360.0f;

	currentBoostGauge_ = player->GetBoostGauge();
	maxBoostGauge_ = player->GetMaxBoostGauge();
	isBarrelRolling_ = player->IsBarrelRolling();
	barrelRollProgress_ = player->GetBarrelRollProgress();

	// ロックオン情報の更新
	lockOnTarget_ = player->GetLockOnTarget();
	lockOnRange_ = player->GetLockOnRange();
	lockOnFOV_ = player->GetLockOnFOV();
	isMissileLockOnMode_ = player->IsMissileLockOnMode();

	// ロックオン中の敵数をカウント
	lockedEnemyCount_ = static_cast<int>(player->GetLockOnTargetCount());

	// ジャスト回避演出の更新
	if (justAvoidanceDisplayActive_) {
		justAvoidanceNotificationTimer_ += 0.016f; // 約60FPS基準
		if (justAvoidanceNotificationTimer_ >= justAvoidanceNotificationDuration_) {
			justAvoidanceDisplayActive_ = false;
		}
	}
}

///=============================================================================
///                        描画
void HUD::Draw() {
	MagEngine::Camera *currentCamera = nullptr;

	if (followCamera_) {
		currentCamera = followCamera_->GetCamera();
	}

	if (!currentCamera) {
		currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
	}

	if (!currentCamera) {
		return;
	}

	if (deployProgress_ <= 0.0f) {
		return;
	}

	screenCenter_ = GetHUDPosition(0.0f, 0.0f);

	// フレーム描画
	float frameProgress = std::max(0.0f, (deployProgress_ - frameDeployStart_) / (1.0f - frameDeployStart_));
	if (frameProgress > 0.0f) {
		DrawHUDFrame(frameProgress);
	}

	// ピッチラダー
	float pitchProgress = std::max(0.0f, (deployProgress_ - pitchLadderDeployStart_) / (1.0f - pitchLadderDeployStart_));
	if (showPitchLadder_ && pitchProgress > 0.0f) {
		DrawPitchLadder(pitchProgress);
	}

	// ボアサイト
	float boresightProgress = std::max(0.0f, (deployProgress_ - boresightDeployStart_) / (1.0f - boresightDeployStart_));
	if (showBoresight_ && boresightProgress > 0.0f) {
		DrawBoresight(boresightProgress);
	}

	// ベロシティベクトル
	float velocityProgress = std::max(0.0f, (deployProgress_ - velocityVectorDeployStart_) / (1.0f - velocityVectorDeployStart_));
	if (showVelocityVector_ && velocityProgress > 0.0f) {
		DrawVelocityVector(velocityProgress);
	}

	// ロールスケール
	float rollProgress = std::max(0.0f, (deployProgress_ - rollScaleDeployStart_) / (1.0f - rollScaleDeployStart_));
	if (showRollScale_ && rollProgress > 0.0f) {
		float rollDeg = RadiansToDegrees(playerRotation_.z);
		DrawRollScale(rollDeg, rollProgress);
	}

	// 方位テープ
	float headingProgress = std::max(0.0f, (deployProgress_ - headingTapeDeployStart_) / (1.0f - headingTapeDeployStart_));
	if (showCompass_ && headingProgress > 0.0f) {
		DrawHeadingTape(headingProgress);
	}

	// G-Force
	float gForceProgress = std::max(0.0f, (deployProgress_ - gForceDeployStart_) / (1.0f - gForceDeployStart_));
	if (showGForce_ && gForceProgress > 0.0f) {
		DrawGForceIndicator(gForceProgress);
	}

	// ブースト＆回避統合UI
	float boostBarrelProgress = std::max(0.0f, (deployProgress_ - boostBarrelDeployStart_) / (1.0f - boostBarrelDeployStart_));
	if (boostBarrelProgress > 0.0f) {
		DrawBoostBarrel(boostBarrelProgress);
	}

	// ロックオン用レティクル（長押しモード中も表示）
	if (showLockOnReticle_ && (isMissileLockOnMode_ || lockOnTarget_)) {
		DrawLockOnReticle(deployProgress_);
	}

	// ジャスト回避成功通知を描画
	if (justAvoidanceDisplayActive_) {
		DrawJustAvoidanceNotification(deployProgress_);
	}
}

///=============================================================================
///                        ガンボアサイト（シンプリファイド / エレガント照準 ）
void HUD::DrawBoresight(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	// 弾発射方向オフセット計算
	Vector3 playerForward = {
		sinf(playerRotation_.y) * cosf(playerRotation_.x),
		-sinf(playerRotation_.x),
		cosf(playerRotation_.y) * cosf(playerRotation_.x)};
	Vector3 fireOffset = bulletFireDirection_ - playerForward;
	float offsetX = Dot(fireOffset, Vector3{cosf(playerRotation_.y), 0.0f, -sinf(playerRotation_.y)}) * 12.0f;
	float offsetY = -Dot(fireOffset, Vector3{sinf(playerRotation_.y) * sinf(playerRotation_.x), cosf(playerRotation_.x), cosf(playerRotation_.y) * sinf(playerRotation_.x)}) * 12.0f;

	// 照準中心 (HUDローカル座標)
	float cx = boresightOffset_.x + offsetX * 0.4f;
	float cy = boresightOffset_.y + offsetY * 0.4f;

	// グロウ効果を付与（パルス）
	float glowPulse = 0.7f + 0.3f * sinf(animationTime_ * 3.5f);
	Vector4 col = hudColor_;
	col.w *= progress * glowPulse;

	Vector4 colGlow = {hudColor_.x * 1.2f, hudColor_.y, hudColor_.z * 0.8f, 0.3f * glowPulse};
	const float gapR = 0.5f * hudScale_;	// 中心からの隙間（縮小）
	const float tickLen = 1.0f * hudScale_; // ティックの長さ（縮小）
	const float thick = 2.0f;
	const float glowThick = 3.5f;

	// === グロウベース（背景処理） ===
	if (progress > 0.0f) {
		float avgS = (hudSizeX_ + hudSizeY_) * 0.5f;
		Vector3 centerPos = GetPlayerFrontPositionWithOffset(cx, cy, boresightOffset_);
		int glowSegs = static_cast<int>(16 * progress);
		if (glowSegs > 0) {
			lineManager->DrawCircle(centerPos, 0.35f * hudScale_ * avgS, colGlow, 1.0f, {0.0f, 0.0f, 1.0f}, glowSegs);
		}
	}

	// === 水平ティック (左右) ===
	if (progress > 0.0f) {
		Vector3 r0 = GetPlayerFrontPositionWithOffset(cx + gapR, cy, boresightOffset_);
		Vector3 r1 = GetPlayerFrontPositionWithOffset(cx + gapR + tickLen, cy, boresightOffset_);
		Vector3 l0 = GetPlayerFrontPositionWithOffset(cx - gapR, cy, boresightOffset_);
		Vector3 l1 = GetPlayerFrontPositionWithOffset(cx - gapR - tickLen, cy, boresightOffset_);
		lineManager->DrawLine(r0, Lerp(r0, r1, progress), col, thick);
		lineManager->DrawLine(l0, Lerp(l0, l1, progress), col, thick);
	}

	// === 垂直ティック (上下) ===
	if (progress > 0.25f) {
		float p2 = (progress - 0.25f) / 0.75f;
		Vector3 t0 = GetPlayerFrontPositionWithOffset(cx, cy + gapR, boresightOffset_);
		Vector3 t1 = GetPlayerFrontPositionWithOffset(cx, cy + gapR + tickLen, boresightOffset_);
		Vector3 b0 = GetPlayerFrontPositionWithOffset(cx, cy - gapR, boresightOffset_);
		Vector3 b1 = GetPlayerFrontPositionWithOffset(cx, cy - gapR - tickLen, boresightOffset_);
		lineManager->DrawLine(t0, Lerp(t0, t1, p2), col, thick);
		lineManager->DrawLine(b0, Lerp(b0, b1, p2), col, thick);
	}

	// === 中央の細いドット（統合） ===
	if (progress > 0.5f) {
		float p3 = (progress - 0.5f) / 0.5f;
		int segs = static_cast<int>(12 * p3);
		if (segs > 0) {
			float avgS = (hudSizeX_ + hudSizeY_) * 0.5f;
			Vector3 centerPos = GetPlayerFrontPositionWithOffset(cx, cy, boresightOffset_);
			lineManager->DrawCircle(centerPos, 0.08f * hudScale_ * avgS, col, 1.5f, {0.0f, 0.0f, 1.0f}, segs);
		}
	}

// === デバッグ用：発射方向インジケーター ===
#ifdef _DEBUG
	if (offsetX != 0.0f || offsetY != 0.0f) {
		Vector3 offsetStart = GetPlayerFrontPositionWithOffset(boresightOffset_.x, boresightOffset_.y, boresightOffset_);
		Vector3 offsetEnd = GetPlayerFrontPositionWithOffset(cx, cy, boresightOffset_);
		lineManager->DrawLine(offsetStart, offsetEnd, {1.0f, 1.0f, 0.0f, 0.5f}, 1.0f);
	}
#endif
}

///=============================================================================
///                        ロールスケール（シンプル化版）
void HUD::DrawRollScale(float rollAngle, float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float radius = 5.0f * hudScale_; // サイズ縮小
	Vector3 arcCenter = GetPlayerFrontPositionWithOffset(0.0f + rollScaleOffset_.x, radius - 0.5f + rollScaleOffset_.y, rollScaleOffset_);

	// 背景の円弧（超シンプル）
	if (progress > 0.0f) {
		Vector4 bgColor = {hudColor_.x, hudColor_.y, hudColor_.z, 0.12f};
		int segments = static_cast<int>(24 * progress);
		if (segments > 0) {
			lineManager->DrawCircle(arcCenter, radius, bgColor, 0.7f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// スケール目盛り（±60°のみ）
	int maxTicks = static_cast<int>(3 * progress);
	int tickIndex = 0;
	for (int angle = -60; angle <= 60; angle += 60) {
		if (tickIndex >= maxTicks)
			break;
		tickIndex++;

		float radians = DegreesToRadians(static_cast<float>(angle));
		float tickLength = 0.6f;

		float outerX = sinf(radians) * radius;
		float outerY = radius - cosf(radians) * radius;
		float innerX = sinf(radians) * (radius - tickLength);
		float innerY = radius - cosf(radians) * (radius - tickLength);

		Vector3 outerPoint = GetPlayerFrontPositionWithOffset(outerX + rollScaleOffset_.x, outerY + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 innerPoint = GetPlayerFrontPositionWithOffset(innerX + rollScaleOffset_.x, innerY + rollScaleOffset_.y, rollScaleOffset_);

		lineManager->DrawLine(outerPoint, innerPoint, hudColor_, 1.0f);
	}

	// 現在のロール角指示器（シンプル化）
	if (progress > 0.6f) {
		float rollRad = DegreesToRadians(rollAngle);
		float indicatorX = sinf(rollRad) * (radius + 0.3f);
		float indicatorY = radius - cosf(rollRad) * (radius + 0.3f);

		Vector3 rollIndicator = GetPlayerFrontPositionWithOffset(indicatorX + rollScaleOffset_.x, indicatorY + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 tri1 = GetPlayerFrontPositionWithOffset(indicatorX - 0.3f + rollScaleOffset_.x, indicatorY - 0.6f + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 tri2 = GetPlayerFrontPositionWithOffset(indicatorX + 0.3f + rollScaleOffset_.x, indicatorY - 0.6f + rollScaleOffset_.y, rollScaleOffset_);

		Vector4 indicatorColor = {hudColor_.x * 0.8f, hudColor_.y, hudColor_.z * 1.2f, 0.9f};
		lineManager->DrawLine(rollIndicator, tri1, indicatorColor, 1.5f);
		lineManager->DrawLine(rollIndicator, tri2, indicatorColor, 1.5f);
		if (progress > 0.7f) {
			lineManager->DrawLine(tri1, tri2, indicatorColor, 1.0f);
		}
	}
}

///=============================================================================
///                        レーダー高度計
void HUD::DrawRadarAltitude(float radarAlt, float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	Vector3 radarStart = GetHUDPosition(12.0f, -6.0f);

	// レーダー高度バー（左から右に展開）
	float radarLength = std::min(radarAlt / 100.0f * 2.0f, 4.0f);
	Vector3 radarEnd = GetHUDPosition(12.0f - radarLength, -6.0f);
	Vector3 radarDraw = Lerp(radarStart, radarEnd, progress);

	lineManager->DrawLine(radarStart, radarDraw, hudColor_);

	// 危険高度マーカー（進行度50%以降で表示）
	if (progress > 0.5f) {
		Vector3 dangerTop = GetHUDPosition(11.0f, -5.8f);
		Vector3 dangerBottom = GetHUDPosition(11.0f, -6.2f);
		lineManager->DrawLine(dangerTop, dangerBottom, {1.0f, 0.0f, 0.0f, 1.0f});
	}
}

///=============================================================================
///                        HUDフレーム改良版
void HUD::DrawHUDFrame(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float cornerSize = 1.5f;
	float frameSize = 12.0f;

	// 各コーナーを順次展開
	// 左上
	if (progress > 0.0f) {
		float cornerProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 leftTopH1 = GetHUDPosition(-frameSize, frameSize);
		Vector3 leftTopH2 = GetHUDPosition(-frameSize + cornerSize, frameSize);
		Vector3 leftTopH2Draw = Lerp(leftTopH1, leftTopH2, cornerProgress);
		lineManager->DrawLine(leftTopH1, leftTopH2Draw, hudColor_, 2.0f);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 leftTopV2 = GetHUDPosition(-frameSize, frameSize - cornerSize);
			Vector3 leftTopV2Draw = Lerp(leftTopH1, leftTopV2, vProgress);
			lineManager->DrawLine(leftTopH1, leftTopV2Draw, hudColor_, 2.0f);
		}
	}

	// 右上
	if (progress > 0.25f) {
		float cornerProgress = std::min((progress - 0.25f) / 0.25f, 1.0f);
		Vector3 rightTopH1 = GetHUDPosition(frameSize, frameSize);
		Vector3 rightTopH2 = GetHUDPosition(frameSize - cornerSize, frameSize);
		Vector3 rightTopH2Draw = Lerp(rightTopH1, rightTopH2, cornerProgress);
		lineManager->DrawLine(rightTopH1, rightTopH2Draw, hudColor_, 2.0f);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 rightTopV2 = GetHUDPosition(frameSize, frameSize - cornerSize);
			Vector3 rightTopV2Draw = Lerp(rightTopH1, rightTopV2, vProgress);
			lineManager->DrawLine(rightTopH1, rightTopV2Draw, hudColor_, 2.0f);
		}
	}

	// 左下
	if (progress > 0.5f) {
		float cornerProgress = std::min((progress - 0.5f) / 0.25f, 1.0f);
		Vector3 leftBottomH1 = GetHUDPosition(-frameSize, -frameSize);
		Vector3 leftBottomH2 = GetHUDPosition(-frameSize + cornerSize, -frameSize);
		Vector3 leftBottomH2Draw = Lerp(leftBottomH1, leftBottomH2, cornerProgress);
		lineManager->DrawLine(leftBottomH1, leftBottomH2Draw, hudColor_, 2.0f);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 leftBottomV2 = GetHUDPosition(-frameSize, -frameSize + cornerSize);
			Vector3 leftBottomV2Draw = Lerp(leftBottomH1, leftBottomV2, vProgress);
			lineManager->DrawLine(leftBottomH1, leftBottomV2Draw, hudColor_, 2.0f);
		}
	}

	// 右下
	if (progress > 0.75f) {
		float cornerProgress = (progress - 0.75f) / 0.25f;
		Vector3 rightBottomH1 = GetHUDPosition(frameSize, -frameSize);
		Vector3 rightBottomH2 = GetHUDPosition(frameSize - cornerSize, -frameSize);
		Vector3 rightBottomH2Draw = Lerp(rightBottomH1, rightBottomH2, cornerProgress);
		lineManager->DrawLine(rightBottomH1, rightBottomH2Draw, hudColor_, 2.0f);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 rightBottomV2 = GetHUDPosition(frameSize, -frameSize + cornerSize);
			Vector3 rightBottomV2Draw = Lerp(rightBottomH1, rightBottomV2, vProgress);
			lineManager->DrawLine(rightBottomH1, rightBottomV2Draw, hudColor_, 2.0f);
		}
	}
}

///=============================================================================
///                        ベロシティベクトル（簡潔化版）
void HUD::DrawVelocityVector(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	if (currentSpeed_ < 0.1f) {
		return;
	}

	// 弾発射方向オフセット計算（ボアサイトと同じ）
	Vector3 playerForward = {
		sinf(playerRotation_.y) * cosf(playerRotation_.x),
		-sinf(playerRotation_.x),
		cosf(playerRotation_.y) * cosf(playerRotation_.x)};
	Vector3 fireOffset = bulletFireDirection_ - playerForward;
	float offsetX = Dot(fireOffset, Vector3{cosf(playerRotation_.y), 0.0f, -sinf(playerRotation_.y)}) * 12.0f;
	float offsetY = -Dot(fireOffset, Vector3{sinf(playerRotation_.y) * sinf(playerRotation_.x), cosf(playerRotation_.x), cosf(playerRotation_.y) * sinf(playerRotation_.x)}) * 12.0f;

	float size = 0.7f * hudScale_;
	Vector3 centerPos = GetPlayerFrontPositionWithOffset(
		boresightOffset_.x + offsetX * 0.4f,
		boresightOffset_.y + offsetY * 0.4f,
		boresightOffset_);

	Vector4 velocityCol = {0.0f, 0.85f, 1.0f, 0.85f}; // シアン系

	// 円形マーカー（速度インジケーター）
	if (progress > 0.0f) {
		float circleProgress = std::min(progress, 1.0f);
		int segments = static_cast<int>(16 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(centerPos, size, velocityCol, 1.5f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// 翼マーク（シンプル化：1本のみ）
	if (progress > 0.4f) {
		float wingProgress = std::min((progress - 0.4f) / 0.6f, 1.0f);
		Vector3 leftWing = GetPlayerFrontPositionWithOffset(-size * 1.2f + offsetX * 0.4f, offsetY * 0.4f, boresightOffset_);
		Vector3 leftWingEnd = GetPlayerFrontPositionWithOffset(-size * 2.0f + offsetX * 0.4f, offsetY * 0.4f, boresightOffset_);
		Vector3 leftWingDraw = Lerp(leftWing, leftWingEnd, wingProgress);
		lineManager->DrawLine(leftWing, leftWingDraw, velocityCol, 2.0f);

		Vector3 rightWing = GetPlayerFrontPositionWithOffset(size * 1.2f + offsetX * 0.4f, offsetY * 0.4f, boresightOffset_);
		Vector3 rightWingEnd = GetPlayerFrontPositionWithOffset(size * 2.0f + offsetX * 0.4f, offsetY * 0.4f, boresightOffset_);
		Vector3 rightWingDraw = Lerp(rightWing, rightWingEnd, wingProgress);
		lineManager->DrawLine(rightWing, rightWingDraw, velocityCol, 2.0f);
	}
}

///=============================================================================
///                        フライトパスマーカー（統合簡潔版）
void HUD::DrawFlightPathMarker(float progress) {
	// VelocityVectorで統合されたため、ここは表示を最小化
	// フライトパスの方向指示は必要に応じてPitchLadderで表現
	// 削減により中央のゴチャつきを解消
	(void)progress; // 未使用警告を抑止
}

///=============================================================================
///                        ピッチラダー（簡潔化版）
void HUD::DrawPitchLadder(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float pitchDeg = RadiansToDegrees(playerRotation_.x);

	// 削減：主要な角度（±20°, ±40°）のみ表示
	int maxLines = static_cast<int>(3 * progress);
	int lineIndex = 0;
	for (int angle = -40; angle <= 40; angle += 20) {
		if (angle == 0) {
			continue; // 水平線は別途描画
		}
		if (lineIndex >= maxLines)
			break;
		lineIndex++;

		float offsetY = (angle - pitchDeg) * 0.3f;
		if (std::abs(offsetY) > 12.0f)
			continue;

		float lineLength = 2.0f; // 統一化
		Vector3 left = GetPlayerFrontPositionWithOffset(-lineLength, offsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(lineLength, offsetY, boresightOffset_);

		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, offsetY, boresightOffset_);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		// 透明度を下げて背景化
		Vector4 ladderCol = hudColor_;
		ladderCol.w *= 0.5f; // 半透明化
		lineManager->DrawLine(leftDraw, rightDraw, ladderCol, 1.0f);
	}

	// 水平線強調（ピッチリファレンス）
	float horizonOffsetY = -pitchDeg * 0.3f;
	if (std::abs(horizonOffsetY) <= 12.0f && progress > 0.3f) {
		Vector3 left = GetPlayerFrontPositionWithOffset(-4.5f, horizonOffsetY, boresightOffset_);
		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, horizonOffsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(4.5f, horizonOffsetY, boresightOffset_);

		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		lineManager->DrawLine(leftDraw, center, hudColor_, 2.5f);
		lineManager->DrawLine(center, rightDraw, hudColor_, 2.5f);
	}
}

///=============================================================================
///                        方位テープ（シンプル化版）
void HUD::DrawHeadingTape(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeY = 7.0f;

	// ベースライン
	if (progress > 0.0f) {
		Vector3 center = GetHUDPosition(0.0f, tapeY);
		Vector3 left = GetHUDPosition(-4.0f, tapeY);
		Vector3 right = GetHUDPosition(4.0f, tapeY);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_, 1.5f);
	}

	// 中央マーカー（簡潔化）
	if (progress > 0.4f) {
		float markerProgress = (progress - 0.4f) / 0.6f;
		Vector3 centerTop = GetHUDPosition(0.0f, tapeY + 0.7f);
		Vector3 centerBottom = GetHUDPosition(0.0f, tapeY);
		Vector3 centerLeft = GetHUDPosition(-0.35f, tapeY + 0.35f);
		Vector3 centerRight = GetHUDPosition(0.35f, tapeY + 0.35f);

		Vector3 centerTopDraw = Lerp(centerBottom, centerTop, markerProgress);
		lineManager->DrawLine(centerTopDraw, centerBottom, hudColor_, 2.0f);

		if (markerProgress > 0.5f) {
			lineManager->DrawLine(centerTop, centerLeft, hudColor_, 1.5f);
			lineManager->DrawLine(centerTop, centerRight, hudColor_, 1.5f);
		}
	}

	// 方位目盛り（90°刻みのみ）
	if (progress > 0.6f) {
		int baseHeading = static_cast<int>(currentHeading_ / 90) * 90;
		for (int i = -1; i <= 1; ++i) {
			int heading = baseHeading + i * 90;
			while (heading < 0)
				heading += 360;
			while (heading >= 360)
				heading -= 360;

			float offsetX = (currentHeading_ - heading) * 0.08f;
			if (std::abs(offsetX) > 4.0f)
				continue;

			Vector3 tickTop = GetHUDPosition(offsetX, tapeY + 0.35f);
			Vector3 tickBottom = GetHUDPosition(offsetX, tapeY);
			lineManager->DrawLine(tickTop, tickBottom, hudColor_, 0.8f);
		}
	}
}

///=============================================================================
///                        G-Force表示（シンプル化版）
void HUD::DrawGForceIndicator(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float posX = -10.5f;
	float posY = -8.5f;

	// 枠描画（シンプル化）
	if (progress > 0.0f) {
		Vector3 barLeft = GetHUDPosition(posX, posY);
		Vector3 barRight = GetHUDPosition(posX + 3.0f, posY);
		Vector3 barRightDraw = Lerp(barLeft, barRight, progress);
		lineManager->DrawLine(barLeft, barRightDraw, hudColor_, 1.5f);

		// 枠の左端
		Vector3 barTop = GetHUDPosition(posX, posY + 0.3f);
		Vector3 barBottom = GetHUDPosition(posX, posY - 0.3f);
		lineManager->DrawLine(barTop, barBottom, hudColor_, 1.0f);
	}

	// G値バー（危険度により色変更）
	if (progress > 0.4f) {
		float barProgress = (progress - 0.4f) / 0.6f;
		Vector4 gColor = hudColor_;
		if (currentGForce_ > 8.0f) {
			gColor = hudColorCritical_;
		} else if (currentGForce_ > 5.5f) {
			gColor = hudColorWarning_;
		}

		float gBarLength = std::min(std::abs(currentGForce_ - 1.0f) / 9.0f * 3.0f, 3.0f);
		Vector3 barLeft = GetHUDPosition(posX, posY);
		Vector3 gBarEnd = GetHUDPosition(posX + gBarLength, posY);
		Vector3 gBarDraw = Lerp(barLeft, gBarEnd, barProgress);
		lineManager->DrawLine(barLeft, gBarDraw, gColor, 3.0f);
	}
}

///=============================================================================
///                        ブースト＆回避統合UI（画面下部）
void HUD::DrawBoostBarrel(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	// === 画面下部中央にブーストと回避UIを配置 ===
	float centerX = 0.0f;
	float centerY = -10.0f;
	float spacing = 6.0f;

	// === ブーストゲージ（左側） ===
	float boostX = centerX - spacing;
	float boostY = centerY;

	// ブーストゲージの枠（横方向に展開）
	if (progress > 0.0f) {
		float frameProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 frameLeft = GetHUDPosition(boostX - 2.5f, boostY);
		Vector3 frameRight = GetHUDPosition(boostX + 2.5f, boostY);
		Vector3 frameRightDraw = Lerp(frameLeft, frameRight, frameProgress);
		lineManager->DrawLine(frameLeft, frameRightDraw, hudColor_, 2.0f);

		// 枠の上下
		Vector3 frameTop = GetHUDPosition(boostX - 2.5f, boostY + 0.6f);
		Vector3 frameTopRight = GetHUDPosition(boostX + 2.5f, boostY + 0.6f);
		Vector3 frameBottom = GetHUDPosition(boostX - 2.5f, boostY - 0.6f);
		Vector3 frameBottomRight = GetHUDPosition(boostX + 2.5f, boostY - 0.6f);

		if (frameProgress > 0.33f) {
			lineManager->DrawLine(frameLeft, frameTop, hudColor_, 1.5f);
			lineManager->DrawLine(frameLeft, frameBottom, hudColor_, 1.5f);
		}
		if (frameProgress > 0.66f) {
			lineManager->DrawLine(frameRightDraw, frameTopRight, hudColor_, 1.5f);
			lineManager->DrawLine(frameRightDraw, frameBottomRight, hudColor_, 1.5f);
		}
	}

	// ブーストバー本体
	if (progress > 0.25f) {
		float barProgress = (progress - 0.25f) / 0.25f;
		float gaugeRatio = currentBoostGauge_ / maxBoostGauge_;
		float gaugeLength = 5.0f * gaugeRatio;

		Vector4 gaugeColor = hudColor_;
		if (gaugeRatio < 0.2f) {
			gaugeColor = hudColorCritical_;
		} else if (gaugeRatio < 0.5f) {
			gaugeColor = hudColorWarning_;
		}

		Vector3 gaugeLeft = GetHUDPosition(boostX - 2.5f, boostY);
		Vector3 gaugeRight = GetHUDPosition(boostX - 2.5f + gaugeLength, boostY);
		Vector3 gaugeRightDraw = Lerp(gaugeLeft, gaugeRight, barProgress);
		lineManager->DrawLine(gaugeLeft, gaugeRightDraw, gaugeColor, 5.0f);
	}

	// === バレルロール回避インジケーター（右側） ===
	float barrelX = centerX + spacing;
	float barrelY = centerY;

	// 回避ゲージの枠（円形で展開）
	if (progress > 0.0f) {
		float frameProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 barrelCenter = GetHUDPosition(barrelX, barrelY);
		float radius = 1.2f;
		int segments = static_cast<int>(32 * frameProgress);
		if (segments > 0) {
			lineManager->DrawCircle(barrelCenter, radius, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// バレルロール中の表示
	if (progress > 0.25f) {
		if (isBarrelRolling_) {
			Vector3 barrelCenter = GetHUDPosition(barrelX, barrelY);
			float barrelProgress = (progress - 0.25f) / 0.25f;
			float fillRadius = 1.2f * (barrelRollProgress_);
			int fillSegments = static_cast<int>(32 * barrelRollProgress_);
			if (fillSegments > 0) {
				Vector4 activeColor = {hudColor_.x, hudColor_.y * 0.5f, hudColor_.z, 0.8f};
				lineManager->DrawCircle(barrelCenter, fillRadius, activeColor, 1.0f, {0.0f, 0.0f, 1.0f}, fillSegments);
			}

			// 回転矢印（進行度に応じて回転）
			float rotAngle = barrelRollProgress_ * 6.28f; // 2π
			Vector3 arrowStart = GetHUDPosition(barrelX + cosf(rotAngle) * 0.8f, barrelY + sinf(rotAngle) * 0.8f);
			Vector3 arrowEnd = GetHUDPosition(barrelX + cosf(rotAngle + 0.5f) * 0.6f, barrelY + sinf(rotAngle + 0.5f) * 0.6f);
			lineManager->DrawLine(arrowStart, arrowEnd, hudColor_, 2.0f);
		} else {
			// スタンバイ状態
			Vector3 barrelCenter = GetHUDPosition(barrelX, barrelY);
			float barrelProgress = (progress - 0.25f) / 0.25f;

			// 十字マーク
			Vector3 crossH1 = GetHUDPosition(barrelX - 0.8f, barrelY);
			Vector3 crossH2 = GetHUDPosition(barrelX + 0.8f, barrelY);
			Vector3 crossV1 = GetHUDPosition(barrelX, barrelY - 0.8f);
			Vector3 crossV2 = GetHUDPosition(barrelX, barrelY + 0.8f);

			if (barrelProgress > 0.5f) {
				lineManager->DrawLine(crossH1, crossH2, hudColor_, 1.5f);
				lineManager->DrawLine(crossV1, crossV2, hudColor_, 1.5f);
			}
		}
	}

	// === 下部ラベルバー ===
	if (progress > 0.75f) {
		float labelProgress = (progress - 0.75f) / 0.25f;
		Vector3 labelLeft = GetHUDPosition(-7.0f, centerY - 2.0f);
		Vector3 labelRight = GetHUDPosition(7.0f, centerY - 2.0f);
		Vector3 labelRightDraw = Lerp(labelLeft, labelRight, labelProgress);
		lineManager->DrawLine(labelLeft, labelRightDraw, hudColor_, 1.0f);
	}
}

///=============================================================================
///                        ImGui描画（簡略版）
void HUD::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("HUD Settings");

	ImGui::Text("HUD Display Control");
	ImGui::Checkbox("Show Boresight", &showBoresight_);
	ImGui::Checkbox("Show Pitch Ladder", &showPitchLadder_);
	ImGui::Checkbox("Show Roll Scale", &showRollScale_);
	ImGui::Checkbox("Show Velocity Vector", &showVelocityVector_);
	ImGui::Checkbox("Show Flight Path", &showFlightPath_);
	ImGui::Checkbox("Show Compass", &showCompass_);
	ImGui::Checkbox("Show G-Force", &showGForce_);

	ImGui::Separator();
	ImGui::Text("Animation Control");
	ImGui::Text("Deploy Progress: %.2f", deployProgress_);
	ImGui::Text("Is Animating: %s", isAnimating_ ? "Yes" : "No");
	if (ImGui::Button("Deploy HUD")) {
		StartDeployAnimation(1.5f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Retract HUD")) {
		StartRetractAnimation(1.0f);
	}

	ImGui::Separator();
	ImGui::Text("Current Values:");
	ImGui::Text("Speed: %.1f m/s", currentSpeed_);
	ImGui::Text("Altitude: %.1f m", currentAltitude_);
	ImGui::Text("Heading: %.1f deg", currentHeading_);
	ImGui::Text("G-Force: %.2f G", currentGForce_);
	ImGui::Text("Boost: %.1f / %.1f", currentBoostGauge_, maxBoostGauge_);
	ImGui::Text("Barrel Rolling: %s", isBarrelRolling_ ? "Yes" : "No");

	ImGui::End();
#endif // _DEBUG
}

///=============================================================================
/// ロックオン用レティクル描画（ワールド空間 4コーナーブラケット）
void HUD::DrawLockOnReticle(float progress) {
	if (!lockOnTarget_)
		return;

	LineManager *lineManager = LineManager::GetInstance();

	// ボアサイト中心 (HUDローカル座標)
	float cx = boresightOffset_.x;
	float cy = boresightOffset_.y;

	// ブラケットサイズ
	float lockSize = 1.9f * hudScale_;
	float armLen = lockSize * 0.52f;

	// ロック時のパルスアニメーション
	float pulse = 0.55f + 0.45f * sinf(animationTime_ * 5.0f);
	Vector4 lockCol = hudColorCyan_;
	lockCol.w = pulse * progress;
	const float th = 2.0f;

	// ─── 左上コーナー ───
	{
		Vector3 c = GetPlayerFrontPositionWithOffset(cx - lockSize, cy + lockSize, boresightOffset_);
		Vector3 cR = GetPlayerFrontPositionWithOffset(cx - lockSize + armLen, cy + lockSize, boresightOffset_);
		Vector3 cD = GetPlayerFrontPositionWithOffset(cx - lockSize, cy + lockSize - armLen, boresightOffset_);
		lineManager->DrawLine(c, cR, lockCol, th);
		lineManager->DrawLine(c, cD, lockCol, th);
	}
	// ─── 右上コーナー ───
	{
		Vector3 c = GetPlayerFrontPositionWithOffset(cx + lockSize, cy + lockSize, boresightOffset_);
		Vector3 cL = GetPlayerFrontPositionWithOffset(cx + lockSize - armLen, cy + lockSize, boresightOffset_);
		Vector3 cD = GetPlayerFrontPositionWithOffset(cx + lockSize, cy + lockSize - armLen, boresightOffset_);
		lineManager->DrawLine(c, cL, lockCol, th);
		lineManager->DrawLine(c, cD, lockCol, th);
	}
	// ─── 左下コーナー ───
	{
		Vector3 c = GetPlayerFrontPositionWithOffset(cx - lockSize, cy - lockSize, boresightOffset_);
		Vector3 cR = GetPlayerFrontPositionWithOffset(cx - lockSize + armLen, cy - lockSize, boresightOffset_);
		Vector3 cU = GetPlayerFrontPositionWithOffset(cx - lockSize, cy - lockSize + armLen, boresightOffset_);
		lineManager->DrawLine(c, cR, lockCol, th);
		lineManager->DrawLine(c, cU, lockCol, th);
	}
	// ─── 右下コーナー ───
	{
		Vector3 c = GetPlayerFrontPositionWithOffset(cx + lockSize, cy - lockSize, boresightOffset_);
		Vector3 cL = GetPlayerFrontPositionWithOffset(cx + lockSize - armLen, cy - lockSize, boresightOffset_);
		Vector3 cU = GetPlayerFrontPositionWithOffset(cx + lockSize, cy - lockSize + armLen, boresightOffset_);
		lineManager->DrawLine(c, cL, lockCol, th);
		lineManager->DrawLine(c, cU, lockCol, th);
	}

	// ─── 中心に小さなドット ───
	{
		float avgS = (hudSizeX_ + hudSizeY_) * 0.5f;
		Vector3 center = GetPlayerFrontPositionWithOffset(cx, cy, boresightOffset_);
		lineManager->DrawCircle(center, 0.1f * hudScale_ * avgS, lockCol, 2.0f, {0.0f, 0.0f, 1.0f}, 8);
	}
}

///=============================================================================
/// 敵位置インジケーター描画
void HUD::DrawEnemyIndicators(float progress) {
	if (!currentPlayer_ || !currentCamera_)
		return;

	auto enemyManager = currentPlayer_->GetEnemyManager();
	if (!enemyManager)
		return;

	LineManager *lineManager = LineManager::GetInstance();
	Camera *camera = currentCamera_;
	Vector3 screenCenter = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f};

	// 敵リストを取得
	const auto &enemies = enemyManager->GetEnemies();

	for (const auto &enemy : enemies) {
		if (!enemy)
			continue;

		// ワールド座標を取得
		Vector3 enemyWorldPos = enemy->GetPosition();
		Vector3 playerToEnemy = enemyWorldPos - currentPlayer_->GetPosition();

		// プレイヤーからの距離
		float distance = std::sqrt(playerToEnemy.x * playerToEnemy.x + playerToEnemy.y * playerToEnemy.y + playerToEnemy.z * playerToEnemy.z);

		// 簡易的なスクリーン座標計算
		// X: 敵が左右どこにいるか（プレイヤーの横方向）
		// Y: 敵が上下どこにいるか（プレイヤーの上下方向）
		float screenX = SCREEN_WIDTH / 2.0f;
		float screenY = SCREEN_HEIGHT / 2.0f;

		if (distance > 0.1f) {
			// プレイヤーの回転情報を取得
			Object3d *playerObj = currentPlayer_->GetObject3d();
			Vector3 playerRotation = {0.0f, 0.0f, 0.0f};
			if (playerObj && playerObj->GetTransform()) {
				playerRotation = playerObj->GetTransform()->rotate;
			}

			// プレイヤーの前方向ベクトル
			Vector3 playerForward = {
				sinf(playerRotation.y),
				0.0f,
				cosf(playerRotation.y)};

			// プレイヤーの右方向ベクトル
			Vector3 playerRight = {
				cosf(playerRotation.y),
				0.0f,
				-sinf(playerRotation.y)};

			// 敵の相対位置を計算
			float horizontalDist = Dot(playerToEnemy, playerRight) / distance;
			float verticalDist = playerToEnemy.y / distance;

			// スクリーン座標にマッピング
			screenX += horizontalDist * 200.0f;
			screenY -= verticalDist * 150.0f;
		}

		// 画面外チェック（画面内のみ描画）
		if (screenX < 0.0f || screenX > SCREEN_WIDTH ||
			screenY < 0.0f || screenY > SCREEN_HEIGHT) {
			continue;
		}

		// ロックオン対象かチェック
		bool isLocked = (lockOnTarget_ == enemy.get());

		// インジケーターの色
		Vector4 indicatorColor;
		if (isLocked) {
			// ロック中は赤
			indicatorColor = {1.0f, 0.0f, 0.0f, 0.9f};
		} else {
			// 通常はシアン
			indicatorColor = {0.0f, 1.0f, 1.0f, 0.6f};
		}

		// インジケーターサイズ
		float boxSize = 8.0f;
		if (isLocked)
			boxSize = 12.0f;

		Vector3 screenPos = {screenX, screenY, 0.0f};

		// 敵位置を小さい四角形で表示
		Vector3 topLeft = {screenPos.x - boxSize, screenPos.y - boxSize, 0.0f};
		Vector3 topRight = {screenPos.x + boxSize, screenPos.y - boxSize, 0.0f};
		Vector3 bottomLeft = {screenPos.x - boxSize, screenPos.y + boxSize, 0.0f};
		Vector3 bottomRight = {screenPos.x + boxSize, screenPos.y + boxSize, 0.0f};

		// 四角形の枠線を描画
		lineManager->DrawLine(topLeft, topRight, indicatorColor);
		lineManager->DrawLine(topRight, bottomRight, indicatorColor);
		lineManager->DrawLine(bottomRight, bottomLeft, indicatorColor);
		lineManager->DrawLine(bottomLeft, topLeft, indicatorColor);

		// ロック中はさらに中心点を描画
		if (isLocked) {
			lineManager->DrawCircle(screenPos, 3.0f, indicatorColor);
		}

		// ロック中は中心線を表示
		if (isLocked) {
			lineManager->DrawLine(
				screenCenter,
				screenPos,
				{1.0f, 1.0f, 0.0f, 0.5f});
		}
	}
}

///=============================================================================
///                        ジャスト回避成功演出を開始
void HUD::PlayJustAvoidanceEffect(float successRate) {
	justAvoidanceDisplayActive_ = true;
	justAvoidanceNotificationTimer_ = 0.0f;
	justAvoidanceSuccessRate_ = std::max(0.0f, std::min(1.0f, successRate));
}

///=============================================================================
///                        ジャスト回避通知表示（画面上部中央）
void HUD::DrawJustAvoidanceNotification(float progress) {
	if (!justAvoidanceDisplayActive_ || justAvoidanceNotificationTimer_ >= justAvoidanceNotificationDuration_) {
		justAvoidanceDisplayActive_ = false;
		return;
	}

	// フェードアウト計算
	float fadeOutStartTime = justAvoidanceNotificationDuration_ * 0.7f; // 70%経過後からフェードアウト開始
	float alpha = 1.0f;
	if (justAvoidanceNotificationTimer_ >= fadeOutStartTime) {
		float fadeProgress = (justAvoidanceNotificationTimer_ - fadeOutStartTime) / (justAvoidanceNotificationDuration_ - fadeOutStartTime);
		alpha = 1.0f - fadeProgress;
	}

	// テキスト表示位置（画面上部中央）
	Vector3 textPos = {SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.2f, 0.0f};

	// 成功率に基づいて色を決定
	Vector4 textColor;
	if (justAvoidanceSuccessRate_ >= 0.99f) {
		// 100%: 金色
		textColor = {1.0f, 0.84f, 0.0f, 0.95f * alpha};
	} else if (justAvoidanceSuccessRate_ >= 0.90f) {
		// 90%以上: 緑
		textColor = {0.0f, 1.0f, 0.3f, 0.95f * alpha};
	} else if (justAvoidanceSuccessRate_ >= 0.80f) {
		// 80%以上: 黄
		textColor = {1.0f, 1.0f, 0.0f, 0.95f * alpha};
	} else {
		// 80%未満: オレンジ
		textColor = {1.0f, 0.65f, 0.0f, 0.95f * alpha};
	}

	// テキストを描画（LineManagerを使用）
	// 簡略版：成功率をパーセンテージで表示
	// NOTE: より詳細な実装にはDebugTextManagerやフォント描画システムの活用が必要
	LineManager *lineManager = LineManager::GetInstance();

	// パルス効果: スケールアニメーション
	float pulseScale = 1.0f + 0.15f * sinf(animationTime_ * 8.0f);
	
	// 成功確認用の視覚的フィードバック
	// スクリーン中央付近に成功ライン（円形フレーム）を描画
	float circleRadius = 30.0f * pulseScale;
	Vector4 frameColor = textColor;
	frameColor.w *= 0.6f; // 枠線は少し薄く

	lineManager->DrawCircle(textPos, circleRadius, frameColor);
	
	// チェックマーク的な視覚効果（2本の直線で簡易版）
	Vector3 checkStart = {textPos.x - 15.0f * pulseScale, textPos.y, 0.0f};
	Vector3 checkMid = {textPos.x - 5.0f * pulseScale, textPos.y + 10.0f * pulseScale, 0.0f};
	Vector3 checkEnd = {textPos.x + 15.0f * pulseScale, textPos.y - 15.0f * pulseScale, 0.0f};
	
	lineManager->DrawLine(checkStart, checkMid, textColor);
	lineManager->DrawLine(checkMid, checkEnd, textColor);
}

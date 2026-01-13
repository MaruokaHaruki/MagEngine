#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Camera.h"
#include "HUD.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

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
	boresightOffset_ = rollScaleOffset_ = {0.0f, -3.0f, 0.0f};
	followCamera_ = nullptr;

	// 表示制御（デフォルト全ON）
	showBoresight_ = showRollScale_ = showCompass_ = true;
	showGForce_ = showVelocityVector_ = showFlightPath_ = showPitchLadder_ = true;

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

void HUD::UpdateAnimation() {
	if (!isAnimating_)
		return;
	animationTime_ += 1.0f / 60.0f;
	float raw = std::min(animationTime_ / animationDuration_, 1.0f);
	float eased = EaseOutCubic(raw);
	deployProgress_ = isDeploying_ ? eased : (1.0f - eased);
	if (raw >= 1.0f) {
		isAnimating_ = false;
		deployProgress_ = isDeploying_ ? 1.0f : 0.0f;
	}
}

float HUD::GetDeployProgress() const {
	return deployProgress_;
}
float HUD::EaseOutCubic(float t) const {
	float f = t - 1.0f;
	return f * f * f + 1.0f;
}
void HUD::SetFollowCamera(FollowCamera *cam) {
	followCamera_ = cam;
}

Vector3 HUD::ClampHUDPosition(const Vector3 &worldPos, const Vector3 &cameraPos, const Vector3 &cameraForward) {
	Vector3 relative = worldPos - cameraPos;
	return (Dot(relative, cameraForward) <= 0.0f) ? cameraPos + cameraForward * hudDistance_ : worldPos;
}

Vector3 HUD::GetHUDPosition(float screenX, float screenY) {
	MagEngine::Camera *cam = followCamera_ ? followCamera_->GetCamera() : CameraManager::GetInstance()->GetCurrentCamera();
	if (!cam)
		return {screenX, screenY, hudDistance_};

	Vector3 pos = cam->GetTransform().translate;
	Vector3 rot = cam->GetTransform().rotate;
	Vector3 fwd = {sinf(rot.y) * cosf(rot.x), -sinf(rot.x), cosf(rot.y) * cosf(rot.x)};
	Vector3 right = {cosf(rot.y), 0.0f, -sinf(rot.y)};
	Vector3 up = {sinf(rot.y) * sinf(rot.x), cosf(rot.x), cosf(rot.y) * sinf(rot.x)};
	Vector3 center = pos + fwd * hudDistance_;
	Vector3 world = center + right * (screenX * hudSizeX_) + up * (screenY * hudSizeY_);
	return ClampHUDPosition(world, pos, fwd);
}

Vector3 HUD::GetPlayerFrontPosition(float screenX, float screenY) {
	if (playerPosition_.x == 0.0f && playerPosition_.y == 0.0f && playerPosition_.z == 0.0f)
		return GetHUDPosition(screenX, screenY);
	float yaw = playerRotation_.y;
	Vector3 fwd = {sinf(yaw), 0.0f, cosf(yaw)};
	Vector3 right = {cosf(yaw), 0.0f, -sinf(yaw)};
	Vector3 up = {0.0f, 1.0f, 0.0f};
	Vector3 center = playerPosition_ + fwd * hudDistance_;
	center.y += hudDistance_ * 0.1f;
	return center + right * (screenX * hudSizeX_) + up * (screenY * hudSizeY_);
}

Vector3 HUD::GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset) {
	if (playerPosition_.x == 0.0f && playerPosition_.y == 0.0f && playerPosition_.z == 0.0f)
		return GetHUDPosition(screenX + offset.x, screenY + offset.y);
	float yaw = playerRotation_.y;
	Vector3 fwd = {sinf(yaw), 0.0f, cosf(yaw)};
	Vector3 right = {cosf(yaw), 0.0f, -sinf(yaw)};
	Vector3 up = {0.0f, 1.0f, 0.0f};
	Vector3 base = playerPosition_ + fwd * hudDistance_;
	base.y += hudDistance_ * 0.1f;
	Vector3 offsetCenter = base + right * (offset.x * hudSizeX_) + up * (offset.y * hudSizeY_);
	return offsetCenter + right * (screenX * hudSizeX_) + up * (screenY * hudSizeY_);
}

void HUD::Update(const Player *player) {
	if (!player)
		return;
	UpdateAnimation();
	playerPosition_ = player->GetPosition();
	Object3d *obj = player->GetObject3d();
	if (obj && obj->GetTransform())
		playerRotation_ = obj->GetTransform()->rotate;
	bulletFireDirection_ = player->GetBulletFireDirection();

	static Vector3 prevPos = playerPosition_;
	playerVelocity_ = (playerPosition_ - prevPos) * 60.0f;
	prevPos = playerPosition_;

	currentSpeed_ = sqrtf(Dot(playerVelocity_, playerVelocity_));
	static float prevSpeed = currentSpeed_;
	currentGForce_ = 1.0f + (currentSpeed_ - prevSpeed) * 60.0f / 9.8f;
	prevSpeed = currentSpeed_;

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
}

void HUD::Draw() {
	MagEngine::Camera *cam = followCamera_ ? followCamera_->GetCamera() : CameraManager::GetInstance()->GetCurrentCamera();
	if (!cam || deployProgress_ <= 0.0f)
		return;

	auto calcProgress = [&](float start) { return std::max(0.0f, (deployProgress_ - start) / (1.0f - start)); };

	if (float p = calcProgress(frameDeployStart_); p > 0.0f)
		DrawHUDFrame(p);
	if (showPitchLadder_ && (float p = calcProgress(pitchLadderDeployStart_); p > 0.0f))
		DrawPitchLadder(p);
	if (showBoresight_ && (float p = calcProgress(boresightDeployStart_); p > 0.0f))
		DrawBoresight(p);
	if (showVelocityVector_ && (float p = calcProgress(velocityVectorDeployStart_); p > 0.0f))
		DrawVelocityVector(p);
	if (showFlightPath_ && (float p = calcProgress(velocityVectorDeployStart_); p > 0.0f))
		DrawFlightPathMarker(p);
	if (showRollScale_ && (float p = calcProgress(rollScaleDeployStart_); p > 0.0f))
		DrawRollScale(RadiansToDegrees(playerRotation_.z), p);
	if (showCompass_ && (float p = calcProgress(headingTapeDeployStart_); p > 0.0f))
		DrawHeadingTape(p);
	if (showGForce_ && (float p = calcProgress(gForceDeployStart_); p > 0.0f))
		DrawGForceIndicator(p);
	if (float p = calcProgress(boostBarrelDeployStart_); p > 0.0f)
		DrawBoostBarrel(p);
}

///=============================================================================
///                        ガンボアサイト（見やすさ重視）
void HUD::DrawBoresight(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float size = 2.2f * hudScale_;

	// 弾発射方向オフセット
	Vector3 fwd = {sinf(playerRotation_.y) * cosf(playerRotation_.x), -sinf(playerRotation_.x), cosf(playerRotation_.y) * cosf(playerRotation_.x)};
	Vector3 fireOffset = bulletFireDirection_ - fwd;
	float ox = Dot(fireOffset, Vector3{cosf(playerRotation_.y), 0.0f, -sinf(playerRotation_.y)}) * 12.0f;
	float oy = -Dot(fireOffset, Vector3{sinf(playerRotation_.y) * sinf(playerRotation_.x), cosf(playerRotation_.x), cosf(playerRotation_.y) * sinf(playerRotation_.x)}) * 12.0f;
	Vector3 center = GetPlayerFrontPositionWithOffset(boresightOffset_.x + ox * 0.4f, boresightOffset_.y + oy * 0.4f, boresightOffset_);

	// 外側円（ターゲティングリング）
	if (progress > 0.0f) {
		int seg = static_cast<int>(24 * std::min(progress / 0.25f, 1.0f));
		for (int i = 0; i < seg; i++) {
			float a1 = (float)i / 24.0f * 2.0f * M_PI, a2 = (float)(i + 1) / 24.0f * 2.0f * M_PI;
			Vector3 p1 = GetPlayerFrontPositionWithOffset(boresightOffset_.x + cosf(a1) * size * 1.5f + ox * 0.4f, boresightOffset_.y + sinf(a1) * size * 1.5f + oy * 0.4f, boresightOffset_);
			Vector3 p2 = GetPlayerFrontPositionWithOffset(boresightOffset_.x + cosf(a2) * size * 1.5f + ox * 0.4f, boresightOffset_.y + sinf(a2) * size * 1.5f + oy * 0.4f, boresightOffset_);
			lm->DrawLine(p1, p2, hudColorCyan_);
		}
	}

	// 水平・垂直十字（クリアで太い線）
	if (progress > 0.2f) {
		float p = (progress - 0.2f) / 0.25f;
		Vector3 l = GetPlayerFrontPositionWithOffset(-size + boresightOffset_.x + ox * 0.4f, boresightOffset_.y + oy * 0.4f, boresightOffset_);
		Vector3 r = GetPlayerFrontPositionWithOffset(size + boresightOffset_.x + ox * 0.4f, boresightOffset_.y + oy * 0.4f, boresightOffset_);
		Vector3 t = GetPlayerFrontPositionWithOffset(boresightOffset_.x + ox * 0.4f, size + boresightOffset_.y + oy * 0.4f, boresightOffset_);
		Vector3 b = GetPlayerFrontPositionWithOffset(boresightOffset_.x + ox * 0.4f, -size + boresightOffset_.y + oy * 0.4f, boresightOffset_);
		lm->DrawLine(Lerp(center, l, p), Lerp(center, r, p), hudColor_, 2.5f);
		lm->DrawLine(Lerp(center, t, p), Lerp(center, b, p), hudColor_, 2.5f);
	}

	// 中心ドット
	if (progress > 0.5f) {
		float dotSize = 0.3f * hudScale_;
		int seg = static_cast<int>(12 * (progress - 0.5f) / 0.5f);
		for (int i = 0; i < seg; i++) {
			float a1 = (float)i / 12.0f * 2.0f * M_PI, a2 = (float)(i + 1) / 12.0f * 2.0f * M_PI;
			Vector3 p1 = GetPlayerFrontPositionWithOffset(boresightOffset_.x + cosf(a1) * dotSize + ox * 0.4f, boresightOffset_.y + sinf(a1) * dotSize + oy * 0.4f, boresightOffset_);
			Vector3 p2 = GetPlayerFrontPositionWithOffset(boresightOffset_.x + cosf(a2) * dotSize + ox * 0.4f, boresightOffset_.y + sinf(a2) * dotSize + oy * 0.4f, boresightOffset_);
			lm->DrawLine(p1, p2, hudColor_, 2.0f);
		}
	}
}

///=============================================================================
///                        ロールスケール（簡潔版）
void HUD::DrawRollScale(float rollAngle, float progress) {
	LineManager *lm = LineManager::GetInstance();
	float radius = 8.5f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(rollScaleOffset_.x, radius + rollScaleOffset_.y, rollScaleOffset_);

	// 目盛り（-60°~+60°、30°間隔）
	int maxTicks = static_cast<int>(5 * progress);
	for (int i = 0, angle = -60; angle <= 60; angle += 30, i++) {
		if (i >= maxTicks)
			break;
		float rad = DegreesToRadians(angle - 90.0f);
		float tickLen = (angle == 0) ? 1.2f : 0.6f;
		Vector3 p1 = GetPlayerFrontPositionWithOffset(rollScaleOffset_.x + cosf(rad) * radius, rollScaleOffset_.y + sinf(rad) * radius + radius, rollScaleOffset_);
		Vector3 p2 = GetPlayerFrontPositionWithOffset(rollScaleOffset_.x + cosf(rad) * (radius - tickLen), rollScaleOffset_.y + sinf(rad) * (radius - tickLen) + radius, rollScaleOffset_);
		Vector4 color = (angle == 0) ? hudColor_ : Vector4{hudColor_.x, hudColor_.y, hudColor_.z, 0.7f};
		lm->DrawLine(p1, p2, color, (angle == 0) ? 2.0f : 1.0f);
	}

	// ロール角インジケーター
	if (progress > 0.7f) {
		float rad = DegreesToRadians(-rollAngle - 90.0f);
		Vector3 p1 = GetPlayerFrontPositionWithOffset(rollScaleOffset_.x + cosf(rad) * (radius + 0.5f), rollScaleOffset_.y + sinf(rad) * (radius + 0.5f) + radius, rollScaleOffset_);
		Vector3 p2 = GetPlayerFrontPositionWithOffset(rollScaleOffset_.x + cosf(rad) * (radius - 1.0f), rollScaleOffset_.y + sinf(rad) * (radius - 1.0f) + radius, rollScaleOffset_);
		lm->DrawLine(p1, p2, hudColorWarning_, 2.5f);
	}
}

///=============================================================================
///                        HUDフレーム（コンパクト版）
void HUD::DrawHUDFrame(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float corner = 1.8f, frame = 13.0f;
	float positions[4][2] = {{-frame, frame}, {frame, frame}, {-frame, -frame}, {frame, -frame}};
	for (int i = 0; i < 4; i++) {
		float p = std::max(0.0f, (progress - i * 0.2f) / 0.8f);
		if (p <= 0.0f)
			continue;
		float x = positions[i][0], y = positions[i][1];
		float sx = (x < 0) ? 1.0f : -1.0f, sy = (y < 0) ? 1.0f : -1.0f;
		lm->DrawLine(GetHUDPosition(x, y), GetHUDPosition(x + sx * corner * p, y), hudColorCyan_, 2.0f);
		lm->DrawLine(GetHUDPosition(x, y), GetHUDPosition(x, y + sy * corner * p), hudColorCyan_, 2.0f);
	}
}

///=============================================================================
///                        ベロシティベクトル
void HUD::DrawVelocityVector(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float size = 1.0f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, 0.0f, boresightOffset_);

	if (progress > 0.0f) {
		int seg = static_cast<int>(16 * progress);
		for (int i = 0; i < seg; i++) {
			float a1 = (float)i / 16.0f * 2.0f * M_PI, a2 = (float)(i + 1) / 16.0f * 2.0f * M_PI;
			Vector3 p1 = GetPlayerFrontPositionWithOffset(cosf(a1) * size, sinf(a1) * size, boresightOffset_);
			Vector3 p2 = GetPlayerFrontPositionWithOffset(cosf(a2) * size, sinf(a2) * size, boresightOffset_);
			lm->DrawLine(p1, p2, hudColor_);
		}
	}
	if (progress > 0.6f) {
		lm->DrawLine(GetPlayerFrontPositionWithOffset(-size * 1.3f, 0.0f, boresightOffset_), GetPlayerFrontPositionWithOffset(-size * 0.8f, 0.0f, boresightOffset_), hudColor_, 1.5f);
		lm->DrawLine(GetPlayerFrontPositionWithOffset(size * 1.3f, 0.0f, boresightOffset_), GetPlayerFrontPositionWithOffset(size * 0.8f, 0.0f, boresightOffset_), hudColor_, 1.5f);
	}
}

///=============================================================================
///                        フライトパスマーカー
void HUD::DrawFlightPathMarker(float progress) {
	if (currentSpeed_ < 0.1f)
		return;
	LineManager *lm = LineManager::GetInstance();
	float vx = playerVelocity_.x * 0.3f, vy = playerVelocity_.y * 0.3f;
	float size = 0.6f * hudScale_;

	if (progress > 0.0f) {
		int seg = static_cast<int>(12 * progress);
		for (int i = 0; i < seg; i++) {
			float a1 = (float)i / 12.0f * 2.0f * M_PI, a2 = (float)(i + 1) / 12.0f * 2.0f * M_PI;
			Vector3 p1 = GetPlayerFrontPositionWithOffset(vx + cosf(a1) * size, vy + sinf(a1) * size, boresightOffset_);
			Vector3 p2 = GetPlayerFrontPositionWithOffset(vx + cosf(a2) * size, vy + sinf(a2) * size, boresightOffset_);
			lm->DrawLine(p1, p2, hudColorCyan_);
		}
	}
	if (progress > 0.5f) {
		lm->DrawLine(GetPlayerFrontPositionWithOffset(vx, vy + size * 1.2f, boresightOffset_), GetPlayerFrontPositionWithOffset(vx, vy + size * 0.7f, boresightOffset_), hudColorCyan_);
		lm->DrawLine(GetPlayerFrontPositionWithOffset(vx, vy - size * 1.2f, boresightOffset_), GetPlayerFrontPositionWithOffset(vx, vy - size * 0.7f, boresightOffset_), hudColorCyan_);
	}
}

///=============================================================================
///                        ピッチラダー（見やすさ重視）
void HUD::DrawPitchLadder(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float pitchDeg = RadiansToDegrees(playerRotation_.x);
	int maxLines = static_cast<int>(7 * progress);

	for (int i = 0, angle = -30; angle <= 30 && i < maxLines; angle += 10, i++) {
		float offsetY = (angle - pitchDeg) * 0.35f;
		if (std::abs(offsetY) > 12.0f)
			continue;
		float lineWidth = (angle % 20 == 0) ? 5.0f : 3.0f;
		bool isHorizon = (angle == 0);
		Vector4 color = isHorizon ? hudColor_ : Vector4{hudColor_.x, hudColor_.y, hudColor_.z, 0.7f};
		float thickness = isHorizon ? 2.5f : 1.5f;
		lm->DrawLine(GetHUDPosition(-lineWidth, offsetY), GetHUDPosition(lineWidth, offsetY), color, thickness);
	}
}

///=============================================================================
///                        方位テープ
void HUD::DrawHeadingTape(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float tapeY = 7.0f;

	if (progress > 0.0f) {
		Vector3 center = GetHUDPosition(0.0f, tapeY), left = GetHUDPosition(-6.0f, tapeY), right = GetHUDPosition(6.0f, tapeY);
		lm->DrawLine(Lerp(center, left, progress), Lerp(center, right, progress), hudColor_);
	}
	if (progress > 0.5f) {
		float p = (progress - 0.5f) / 0.5f;
		Vector3 top = GetHUDPosition(0.0f, tapeY + 1.0f), bottom = GetHUDPosition(0.0f, tapeY);
		lm->DrawLine(Lerp(bottom, top, p), bottom, hudColor_, 2.0f);
		if (p > 0.5f) {
			lm->DrawLine(top, GetHUDPosition(-0.5f, tapeY + 0.5f), hudColor_, 2.0f);
			lm->DrawLine(top, GetHUDPosition(0.5f, tapeY + 0.5f), hudColor_, 2.0f);
		}
	}
	if (progress > 0.7f) {
		int base = static_cast<int>(currentHeading_ / 30) * 30;
		for (int i = -2; i <= 2; i++) {
			int heading = (base + i * 30 + 360) % 360;
			float offsetX = (currentHeading_ - heading) * 0.15f;
			if (std::abs(offsetX) <= 6.0f)
				lm->DrawLine(GetHUDPosition(offsetX, tapeY + 0.5f), GetHUDPosition(offsetX, tapeY), hudColor_);
		}
	}
}

///=============================================================================
///                        G-Force表示
void HUD::DrawGForceIndicator(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float px = -11.0f, py = -8.0f;

	if (progress > 0.0f) {
		Vector3 bl = GetHUDPosition(px, py), br = GetHUDPosition(px + 4.0f, py);
		Vector3 tl = GetHUDPosition(px, py + 1.5f), tr = GetHUDPosition(px + 4.0f, py + 1.5f);
		lm->DrawLine(Lerp(bl, br, progress), bl, hudColor_);
		lm->DrawLine(Lerp(tl, tr, progress), tl, hudColor_);
		lm->DrawLine(Lerp(bl, tl, progress), bl, hudColor_);
		lm->DrawLine(Lerp(br, tr, progress), br, hudColor_);
	}
	if (progress > 0.5f) {
		float ratio = std::clamp((currentGForce_ - 0.5f) / 5.5f, 0.0f, 1.0f);
		Vector4 color = (ratio > 0.7f) ? hudColorCritical_ : (ratio > 0.5f) ? hudColorWarning_
																			: hudColor_;
		Vector3 start = GetHUDPosition(px + 0.2f, py + 0.2f);
		Vector3 end = GetHUDPosition(px + 0.2f + 3.6f * ratio, py + 0.2f);
		lm->DrawLine(start, end, color, 5.0f);
	}
}

///=============================================================================
///                        ブースト＆回避統合UI
void HUD::DrawBoostBarrel(float progress) {
	LineManager *lm = LineManager::GetInstance();
	float cx = 0.0f, cy = -10.0f, spacing = 6.0f;

	// ブーストゲージ（左）
	float boostX = cx - spacing, boostY = cy;
	if (progress > 0.0f) {
		Vector3 bl = GetHUDPosition(boostX - 3.0f, boostY - 1.0f), br = GetHUDPosition(boostX + 3.0f, boostY - 1.0f);
		Vector3 tl = GetHUDPosition(boostX - 3.0f, boostY + 1.0f), tr = GetHUDPosition(boostX + 3.0f, boostY + 1.0f);
		float p = std::min(progress / 0.25f, 1.0f);
		lm->DrawLine(Lerp(bl, br, p), bl, hudColor_);
		lm->DrawLine(Lerp(tl, tr, p), tl, hudColor_);
		lm->DrawLine(Lerp(bl, tl, p), bl, hudColor_);
		lm->DrawLine(Lerp(br, tr, p), br, hudColor_);
	}
	if (progress > 0.25f) {
		float ratio = currentBoostGauge_ / maxBoostGauge_;
		Vector4 color = (ratio > 0.3f) ? hudColor_ : hudColorCritical_;
		Vector3 start = GetHUDPosition(boostX - 2.8f, boostY);
		Vector3 end = GetHUDPosition(boostX - 2.8f + 5.6f * ratio, boostY);
		lm->DrawLine(start, end, color, 8.0f);
	}

	// バレルロールインジケーター（右）
	float barrelX = cx + spacing, barrelY = cy;
	if (progress > 0.0f && isBarrelRolling_) {
		int seg = static_cast<int>(32 * barrelRollProgress_);
		for (int i = 0; i < seg; i++) {
			float a1 = (float)i / 32.0f * 2.0f * M_PI, a2 = (float)(i + 1) / 32.0f * 2.0f * M_PI;
			Vector3 p1 = GetHUDPosition(barrelX + cosf(a1) * 1.5f, barrelY + sinf(a1) * 1.5f);
			Vector3 p2 = GetHUDPosition(barrelX + cosf(a2) * 1.5f, barrelY + sinf(a2) * 1.5f);
			lm->DrawLine(p1, p2, hudColorCyan_, 2.0f);
		}
	}
}

///=============================================================================
///                        ImGui描画
void HUD::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("HUD Settings");
	ImGui::Checkbox("Boresight", &showBoresight_);
	ImGui::Checkbox("Roll Scale", &showRollScale_);
	ImGui::Checkbox("Compass", &showCompass_);
	ImGui::Checkbox("G-Force", &showGForce_);
	ImGui::Checkbox("Velocity Vector", &showVelocityVector_);
	ImGui::Checkbox("Flight Path", &showFlightPath_);
	ImGui::Checkbox("Pitch Ladder", &showPitchLadder_);
	ImGui::SliderFloat("Scale", &hudScale_, 0.5f, 1.5f);
	ImGui::SliderFloat("Distance", &hudDistance_, 10.0f, 30.0f);
	ImGui::Text("Speed: %.1f", currentSpeed_);
	ImGui::Text("Altitude: %.1f", currentAltitude_);
	ImGui::Text("G-Force: %.2f", currentGForce_);
	ImGui::Text("Boost: %.1f%%", (currentBoostGauge_ / maxBoostGauge_) * 100.0f);
	ImGui::End();
#endif
}

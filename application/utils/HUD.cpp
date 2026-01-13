#define _USE_MATH_DEFINES
#define NOMINMAX
#include "HUD.h"
#include "Camera.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void HUD::Initialize() {
	screenCenter_ = {0.0f, -3.0f, 0.0f};
	hudScale_ = 0.8f;
	hudColor_ = {0.0f, 1.0f, 0.0f, 1.0f};
	hudColorWarning_ = {1.0f, 1.0f, 0.0f, 1.0f};
	hudColorCritical_ = {1.0f, 0.0f, 0.0f, 1.0f};
	hudDistance_ = 20.0f;
	hudSizeX_ = 0.35f;
	hudSizeY_ = 0.25f;
	viewportMargin_ = 0.85f;

	boresightOffset_ = {0.0f, -3.0f, 0.0f};
	rollScaleOffset_ = {0.0f, -3.0f, 0.0f};

	followCamera_ = nullptr;

	// 表示設定の初期化（未使用項目削除）
	showBoresight_ = true;
	showRollScale_ = true;
	showCompass_ = true;
	showGForce_ = true;
	showVelocityVector_ = true;
	showFlightPath_ = true;
	showPitchLadder_ = true;

	// データの初期化
	playerPosition_ = {0.0f, 0.0f, 0.0f};
	playerRotation_ = {0.0f, 0.0f, 0.0f};
	playerVelocity_ = {0.0f, 0.0f, 0.0f};
	currentGForce_ = 1.0f;
	currentSpeed_ = 0.0f;
	currentAltitude_ = 0.0f;
	currentHeading_ = 0.0f;
	currentBoostGauge_ = 100.0f;
	maxBoostGauge_ = 100.0f;
	isBarrelRolling_ = false;
	barrelRollProgress_ = 0.0f;

	// アニメーション状態の初期化
	isAnimating_ = false;
	isDeploying_ = false;
	animationTime_ = 0.0f;
	animationDuration_ = 1.5f;
	deployProgress_ = 0.0f;

	// 各要素の展開タイミング
	frameDeployStart_ = 0.0f;
	boresightDeployStart_ = 0.1f;
	pitchLadderDeployStart_ = 0.15f;
	velocityVectorDeployStart_ = 0.2f;
	rollScaleDeployStart_ = 0.25f;
	headingTapeDeployStart_ = 0.4f;
	gForceDeployStart_ = 0.45f;
	boostBarrelDeployStart_ = 0.5f;
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

	// フライトパス
	float flightPathProgress = std::max(0.0f, (deployProgress_ - velocityVectorDeployStart_) / (1.0f - velocityVectorDeployStart_));
	if (showFlightPath_ && flightPathProgress > 0.0f) {
		DrawFlightPathMarker(flightPathProgress);
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
}

///=============================================================================
///                        ガンボアサイト改良版（弾発射方向対応）
void HUD::DrawBoresight(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float size = 2.0f * hudScale_;

	// === 弾発射方向に基づくサイト位置を計算 ===
	// プレイヤーの前方ベクトルと弾発射方向の差分を計算
	Vector3 playerForward = {
		sinf(playerRotation_.y) * cosf(playerRotation_.x),
		-sinf(playerRotation_.x),
		cosf(playerRotation_.y) * cosf(playerRotation_.x)};

	// 弾発射方向のオフセット（正規化）
	Vector3 fireDirectionOffset = bulletFireDirection_ - playerForward;

	// オフセットを画面座標に変換（視差補正）
	float offsetScreenX = Dot(fireDirectionOffset, Vector3{cosf(playerRotation_.y), 0.0f, -sinf(playerRotation_.y)}) * 15.0f;
	float offsetScreenY = -Dot(fireDirectionOffset, Vector3{sinf(playerRotation_.y) * sinf(playerRotation_.x), cosf(playerRotation_.x), cosf(playerRotation_.y) * sinf(playerRotation_.x)}) * 15.0f;

	// サイト中心位置を動的に計算
	Vector3 centerPos = GetPlayerFrontPositionWithOffset(
		boresightOffset_.x + offsetScreenX * 0.5f,
		boresightOffset_.y + offsetScreenY * 0.5f,
		boresightOffset_);

	// === 外側の大きな円（ロック範囲を示唆） ===
	if (progress > 0.0f) {
		float circleProgress = std::min(progress / 0.3f, 1.0f);
		int segments = static_cast<int>(32 * circleProgress);
		if (segments > 0) {
			Vector4 outlineColor = {hudColor_.x, hudColor_.y, hudColor_.z, 0.5f};
			lineManager->DrawCircle(centerPos, size * 1.5f, outlineColor, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// === 水平線（中央から左右に展開） ===
	if (progress > 0.2f) {
		float hProgress = (progress - 0.2f) / 0.3f;
		Vector3 leftPos = GetPlayerFrontPositionWithOffset(
			-size + boresightOffset_.x + offsetScreenX * 0.5f,
			0.0f + boresightOffset_.y + offsetScreenY * 0.5f,
			boresightOffset_);
		Vector3 rightPos = GetPlayerFrontPositionWithOffset(
			size + boresightOffset_.x + offsetScreenX * 0.5f,
			0.0f + boresightOffset_.y + offsetScreenY * 0.5f,
			boresightOffset_);
		Vector3 leftDraw = Lerp(centerPos, leftPos, hProgress);
		Vector3 rightDraw = Lerp(centerPos, rightPos, hProgress);
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_, 2.0f);
	}

	// === 垂直線（中央から上下に展開） ===
	if (progress > 0.4f) {
		float vProgress = (progress - 0.4f) / 0.3f;
		Vector3 topPos = GetPlayerFrontPositionWithOffset(
			0.0f + boresightOffset_.x + offsetScreenX * 0.5f,
			size + boresightOffset_.y + offsetScreenY * 0.5f,
			boresightOffset_);
		Vector3 bottomPos = GetPlayerFrontPositionWithOffset(
			0.0f + boresightOffset_.x + offsetScreenX * 0.5f,
			-size + boresightOffset_.y + offsetScreenY * 0.5f,
			boresightOffset_);
		Vector3 topDraw = Lerp(centerPos, topPos, vProgress);
		Vector3 bottomDraw = Lerp(centerPos, bottomPos, vProgress);
		lineManager->DrawLine(topDraw, bottomDraw, hudColor_, 2.0f);
	}

	// === 中央の小さな円 ===
	if (progress > 0.6f) {
		float circleProgress = (progress - 0.6f) / 0.4f;
		float averageSize = (hudSizeX_ + hudSizeY_) * 0.5f;
		int segments = static_cast<int>(16 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(centerPos, 0.3f * hudScale_ * averageSize, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

// === デバッグ用：発射方向インジケーター ===
#ifdef _DEBUG
	if (offsetScreenX != 0.0f || offsetScreenY != 0.0f) {
		Vector3 offsetStart = GetPlayerFrontPositionWithOffset(0.0f, 0.0f, boresightOffset_);
		Vector3 offsetEnd = centerPos;
		Vector4 debugColor = {1.0f, 1.0f, 0.0f, 0.5f}; // 黄色
		lineManager->DrawLine(offsetStart, offsetEnd, debugColor, 1.0f);
	}
#endif
}

///=============================================================================
///                        ロールスケール改良版
void HUD::DrawRollScale(float rollAngle, float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float radius = 8.0f * hudScale_;
	Vector3 arcCenter = GetPlayerFrontPositionWithOffset(0.0f + rollScaleOffset_.x, radius + rollScaleOffset_.y, rollScaleOffset_);

	// 背景の円弧（薄い表示）
	if (progress > 0.0f) {
		Vector4 bgColor = {hudColor_.x, hudColor_.y, hudColor_.z, 0.2f};
		lineManager->DrawCircle(arcCenter, radius, bgColor, 1.0f, {0.0f, 0.0f, 1.0f}, 64);
	}

	// スケール目盛り
	int maxTicks = static_cast<int>(5 * progress);
	int tickIndex = 0;
	for (int angle = -60; angle <= 60; angle += 30) {
		if (tickIndex >= maxTicks)
			break;
		tickIndex++;

		float radians = DegreesToRadians(static_cast<float>(angle));
		float tickLength = (angle == 0) ? 1.5f : 1.0f;

		float outerX = sinf(radians) * radius;
		float outerY = radius - cosf(radians) * radius;
		float innerX = sinf(radians) * (radius - tickLength);
		float innerY = radius - cosf(radians) * (radius - tickLength);

		Vector3 outerPoint = GetPlayerFrontPositionWithOffset(outerX + rollScaleOffset_.x, outerY + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 innerPoint = GetPlayerFrontPositionWithOffset(innerX + rollScaleOffset_.x, innerY + rollScaleOffset_.y, rollScaleOffset_);

		float thickness = (angle == 0) ? 2.0f : 1.5f;
		lineManager->DrawLine(outerPoint, innerPoint, hudColor_, thickness);
	}

	// 現在のロール角指示器（進行度80%以降で表示）
	if (progress > 0.8f) {
		float indicatorProgress = (progress - 0.8f) / 0.2f;
		float rollRad = DegreesToRadians(rollAngle);
		float indicatorX = sinf(rollRad) * (radius - 0.5f);
		float indicatorY = radius - cosf(rollRad) * (radius - 0.5f);

		Vector3 rollIndicator = GetPlayerFrontPositionWithOffset(indicatorX + rollScaleOffset_.x, indicatorY + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 tri1 = GetPlayerFrontPositionWithOffset(indicatorX - 0.5f + rollScaleOffset_.x, indicatorY - 1.0f + rollScaleOffset_.y, rollScaleOffset_);
		Vector3 tri2 = GetPlayerFrontPositionWithOffset(indicatorX + 0.5f + rollScaleOffset_.x, indicatorY - 1.0f + rollScaleOffset_.y, rollScaleOffset_);

		Vector4 indicatorColor = {hudColor_.x, hudColor_.y * 0.7f, hudColor_.z, 1.0f};
		if (indicatorProgress > 0.33f) {
			lineManager->DrawLine(rollIndicator, tri1, indicatorColor, 2.0f);
		}
		if (indicatorProgress > 0.66f) {
			lineManager->DrawLine(rollIndicator, tri2, indicatorColor, 2.0f);
			lineManager->DrawLine(tri1, tri2, indicatorColor, 2.0f);
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
///                        ベロシティベクトル改良版
void HUD::DrawVelocityVector(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float size = 1.0f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, 0.0f, boresightOffset_);

	// 円形マーカー（円弧として展開）
	if (progress > 0.0f) {
		float circleProgress = std::min(progress / 0.6f, 1.0f);
		int segments = static_cast<int>(20 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(center, size, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// 左翼マーク
	if (progress > 0.6f) {
		float wingProgress = (progress - 0.6f) / 0.2f;
		Vector3 leftWing = GetPlayerFrontPositionWithOffset(-size * 1.5f, 0.0f, boresightOffset_);
		Vector3 leftWingEnd = GetPlayerFrontPositionWithOffset(-size * 2.5f, 0.0f, boresightOffset_);
		Vector3 leftWingDraw = Lerp(leftWing, leftWingEnd, wingProgress);
		lineManager->DrawLine(leftWing, leftWingDraw, hudColor_, 2.5f);
	}

	// 右翼マーク
	if (progress > 0.8f) {
		float wingProgress = (progress - 0.8f) / 0.2f;
		Vector3 rightWing = GetPlayerFrontPositionWithOffset(size * 1.5f, 0.0f, boresightOffset_);
		Vector3 rightWingEnd = GetPlayerFrontPositionWithOffset(size * 2.5f, 0.0f, boresightOffset_);
		Vector3 rightWingDraw = Lerp(rightWing, rightWingEnd, wingProgress);
		lineManager->DrawLine(rightWing, rightWingDraw, hudColor_, 2.5f);
	}
}

///=============================================================================
///                        フライトパスマーカー
void HUD::DrawFlightPathMarker(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	if (currentSpeed_ < 0.1f) {
		return;
	}

	float velocityOffsetX = playerVelocity_.x * 0.3f;
	float velocityOffsetY = playerVelocity_.y * 0.3f;

	float size = 0.6f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY, boresightOffset_);

	if (progress > 0.0f) {
		float circleProgress = std::min(progress / 0.5f, 1.0f);
		int segments = static_cast<int>(12 * circleProgress);
		if (segments > 0) {
			Vector4 pathColor = {hudColor_.x * 0.6f, hudColor_.y, hudColor_.z, 1.0f};
			lineManager->DrawCircle(center, size, pathColor, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	if (progress > 0.5f) {
		float lineProgress = (progress - 0.5f) / 0.5f;
		Vector3 top = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 1.5f, boresightOffset_);
		Vector3 topEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 0.8f, boresightOffset_);
		Vector3 topDraw = Lerp(top, topEnd, lineProgress);
		lineManager->DrawLine(top, topDraw, hudColor_, 2.0f);
	}

	if (progress > 0.75f) {
		float lineProgress = (progress - 0.75f) / 0.25f;
		Vector3 bottom = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 1.5f, boresightOffset_);
		Vector3 bottomEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 0.8f, boresightOffset_);
		Vector3 bottomDraw = Lerp(bottom, bottomEnd, lineProgress);
		lineManager->DrawLine(bottom, bottomDraw, hudColor_, 2.0f);
	}
}

///=============================================================================
///                        ピッチラダー
void HUD::DrawPitchLadder(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float pitchDeg = RadiansToDegrees(playerRotation_.x);

	int maxLines = static_cast<int>(7 * progress);
	int lineIndex = 0;
	for (int angle = -30; angle <= 30; angle += 10) {
		if (angle == 0 && lineIndex < maxLines) {
			lineIndex++;
			continue;
		}
		if (lineIndex >= maxLines)
			break;
		lineIndex++;

		float offsetY = (angle - pitchDeg) * 0.3f;
		if (std::abs(offsetY) > 12.0f)
			continue;

		float lineLength = (angle % 20 == 0) ? 3.5f : 2.5f;
		Vector3 left = GetPlayerFrontPositionWithOffset(-lineLength, offsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(lineLength, offsetY, boresightOffset_);

		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, offsetY, boresightOffset_);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		float thickness = (angle % 20 == 0) ? 2.0f : 1.5f;
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_, thickness);
	}

	// 水平線強調
	float horizonOffsetY = -pitchDeg * 0.3f;
	if (std::abs(horizonOffsetY) <= 12.0f && progress > 0.0f) {
		Vector3 left = GetPlayerFrontPositionWithOffset(-7.0f, horizonOffsetY, boresightOffset_);
		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, horizonOffsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(7.0f, horizonOffsetY, boresightOffset_);

		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		lineManager->DrawLine(leftDraw, center, hudColor_, 3.0f);
		lineManager->DrawLine(center, rightDraw, hudColor_, 3.0f);
	}
}

///=============================================================================
///                        方位テープ
void HUD::DrawHeadingTape(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeY = 7.0f;

	// ベースライン
	if (progress > 0.0f) {
		Vector3 center = GetHUDPosition(0.0f, tapeY);
		Vector3 left = GetHUDPosition(-5.0f, tapeY);
		Vector3 right = GetHUDPosition(5.0f, tapeY);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_, 2.0f);
	}

	// 中央マーカー
	if (progress > 0.5f) {
		float markerProgress = (progress - 0.5f) / 0.5f;
		Vector3 centerTop = GetHUDPosition(0.0f, tapeY + 1.0f);
		Vector3 centerBottom = GetHUDPosition(0.0f, tapeY);
		Vector3 centerLeft = GetHUDPosition(-0.5f, tapeY + 0.5f);
		Vector3 centerRight = GetHUDPosition(0.5f, tapeY + 0.5f);

		Vector3 centerTopDraw = Lerp(centerBottom, centerTop, markerProgress);
		lineManager->DrawLine(centerTopDraw, centerBottom, hudColor_, 2.5f);

		if (markerProgress > 0.5f) {
			lineManager->DrawLine(centerTop, centerLeft, hudColor_, 2.0f);
			lineManager->DrawLine(centerTop, centerRight, hudColor_, 2.0f);
		}
	}

	// 方位目盛り
	if (progress > 0.7f) {
		int baseHeading = static_cast<int>(currentHeading_ / 30) * 30;
		for (int i = -2; i <= 2; ++i) {
			int heading = baseHeading + i * 30;
			while (heading < 0)
				heading += 360;
			while (heading >= 360)
				heading -= 360;

			float offsetX = (currentHeading_ - heading) * 0.15f;
			if (std::abs(offsetX) > 6.0f)
				continue;

			Vector3 tickTop = GetHUDPosition(offsetX, tapeY + 0.5f);
			Vector3 tickBottom = GetHUDPosition(offsetX, tapeY);
			lineManager->DrawLine(tickTop, tickBottom, hudColor_, 1.5f);
		}
	}
}

///=============================================================================
///                        G-Force表示改良版
void HUD::DrawGForceIndicator(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float posX = -11.0f;
	float posY = -8.0f;

	// 枠描画
	if (progress > 0.0f) {
		Vector3 barLeft = GetHUDPosition(posX, posY);
		Vector3 barRight = GetHUDPosition(posX + 4.0f, posY);
		Vector3 barRightDraw = Lerp(barLeft, barRight, progress);
		lineManager->DrawLine(barLeft, barRightDraw, hudColor_, 2.0f);

		// 枠の上下
		Vector3 barTop = GetHUDPosition(posX, posY + 0.4f);
		Vector3 barBottom = GetHUDPosition(posX, posY - 0.4f);
		lineManager->DrawLine(barTop, barBottom, hudColor_, 1.5f);
	}

	// G値バー
	if (progress > 0.5f) {
		float barProgress = (progress - 0.5f) / 0.5f;
		Vector4 gColor = hudColor_;
		if (currentGForce_ > 7.0f) {
			gColor = hudColorCritical_;
		} else if (currentGForce_ > 5.0f) {
			gColor = hudColorWarning_;
		}

		float gBarLength = std::min(std::abs(currentGForce_ - 1.0f) / 8.0f * 4.0f, 4.0f);
		Vector3 barLeft = GetHUDPosition(posX, posY);
		Vector3 gBarEnd = GetHUDPosition(posX + gBarLength, posY);
		Vector3 gBarDraw = Lerp(barLeft, gBarEnd, barProgress);
		lineManager->DrawLine(barLeft, gBarDraw, gColor, 4.0f);
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

	// ブーストテキスト風マーク
	if (progress > 0.5f) {
		float textProgress = (progress - 0.5f) / 0.25f;
		Vector3 boostLabel1 = GetHUDPosition(boostX - 3.2f, boostY + 1.0f);
		Vector3 boostLabel2 = GetHUDPosition(boostX + 3.2f, boostY + 1.0f);
		Vector3 boostLabel1Draw = Lerp(boostLabel1, boostLabel1, textProgress);
		lineManager->DrawLine(boostLabel1Draw, boostLabel2, hudColor_, 1.0f);
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

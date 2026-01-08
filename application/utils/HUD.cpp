#define _USE_MATH_DEFINES
#define NOMINMAX
#include "HUD.h"
#include "ImguiSetup.h"
#include <algorithm>
#include <cmath>

namespace {
	const float PI = 3.14159265f;

	inline float RadiansToDegrees(float radians) {
		return radians * (180.0f / PI);
	}

	inline float DegreesToRadians(float degrees) {
		return degrees * (PI / 180.0f);
	}

	// 2点間を補間する関数
	inline Vector3 Lerp(const Vector3 &a, const Vector3 &b, float t) {
		return {
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t};
	}
}

///=============================================================================
///                        初期化
void HUD::Initialize() {
	screenCenter_ = {0.0f, -3.0f, 0.0f};
	hudScale_ = 1.0f;
	hudColor_ = {0.0f, 1.0f, 0.0f, 1.0f};		  // 緑色のHUD
	hudColorWarning_ = {1.0f, 1.0f, 0.0f, 1.0f};  // 黄色（警告）
	hudColorCritical_ = {1.0f, 0.0f, 0.0f, 1.0f}; // 赤色（危険）
	hudDistance_ = 20.0f;						  // カメラから20単位前方にHUDを配置
	hudSizeX_ = 0.5f;							  // HUD横幅サイズ倍率
	hudSizeY_ = 0.3f;							  // HUD縦幅サイズ倍率

	// プレイヤー正面HUD要素の位置調整初期化
	boresightOffset_ = {0.0f, -3.0f, 0.0f}; // ガンボアサイトのオフセット
	rollScaleOffset_ = {0.0f, -3.0f, 0.0f}; // ロールスケールのオフセット（デフォルトで上方に配置）

	// FollowCameraの参照を初期化
	followCamera_ = nullptr;

	// 表示設定の初期化
	showBoresight_ = true;
	showPitchScale_ = true;
	showRollScale_ = true;
	showSpeedIndicator_ = true;
	showAltitudeIndicator_ = true;
	showCompass_ = true;
	showGForce_ = true;
	showVelocityVector_ = true;
	showFlightPath_ = true;
	showPitchLadder_ = true;
	showBoostGauge_ = true;			 // 追加
	showBarrelRollIndicator_ = true; // 追加

	// データの初期化
	playerPosition_ = {0.0f, 0.0f, 0.0f};
	playerRotation_ = {0.0f, 0.0f, 0.0f};
	playerVelocity_ = {0.0f, 0.0f, 0.0f};
	currentGForce_ = 1.0f;
	currentSpeed_ = 0.0f;
	currentAltitude_ = 0.0f;
	currentHeading_ = 0.0f;
	currentBoostGauge_ = 100.0f; // 追加
	maxBoostGauge_ = 100.0f;	 // 追加
	isBarrelRolling_ = false;	 // 追加
	barrelRollProgress_ = 0.0f;	 // 追加

	// アニメーション状態の初期化
	isAnimating_ = false;
	isDeploying_ = false;
	animationTime_ = 0.0f;
	animationDuration_ = 1.5f;
	deployProgress_ = 0.0f; // 初期状態は格納状態

	// 各要素の展開タイミング（順次展開するように設定）
	frameDeployStart_ = 0.0f;
	boresightDeployStart_ = 0.1f;
	pitchLadderDeployStart_ = 0.15f;
	velocityVectorDeployStart_ = 0.2f;
	rollScaleDeployStart_ = 0.25f;
	speedTapeDeployStart_ = 0.3f;
	altitudeTapeDeployStart_ = 0.35f;
	headingTapeDeployStart_ = 0.4f;
	gForceDeployStart_ = 0.45f;
	boostGaugeDeployStart_ = 0.5f;			 // 追加
	barrelRollIndicatorDeployStart_ = 0.55f; // 追加
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

	// アニメーション時間を進める（60FPS想定）
	animationTime_ += 1.0f / 60.0f;

	// 進行度を計算（0.0f～1.0f）
	float rawProgress = animationTime_ / animationDuration_;
	rawProgress = std::min(rawProgress, 1.0f);

	// イージングを適用
	float easedProgress = EaseOutCubic(rawProgress);

	// 展開中か格納中かで進行度を設定
	if (isDeploying_) {
		deployProgress_ = easedProgress;
	} else {
		deployProgress_ = 1.0f - easedProgress;
	}

	// アニメーション完了チェック
	if (rawProgress >= 1.0f) {
		isAnimating_ = false;
		deployProgress_ = isDeploying_ ? 1.0f : 0.0f;
	}
}

///=============================================================================
///                        展開進行度取得（要素ごと）
float HUD::GetDeployProgress() const {
	return deployProgress_;
}

///=============================================================================
///                        イージング関数（EaseOutCubic）
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
///                        スクリーン座標変換
Vector3 HUD::GetHUDPosition(float screenX, float screenY) {
	// FollowCameraが設定されている場合はそれを優先使用
	Camera *currentCamera = nullptr;

	if (followCamera_) {
		currentCamera = followCamera_->GetCamera();
	}

	// FollowCameraが取得できない場合はCameraManagerから取得
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

	return worldPos;
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

	// アニメーション更新
	UpdateAnimation();

	// プレイヤーデータの取得
	playerPosition_ = player->GetPosition();

	Object3d *playerObj = player->GetObject3d();
	if (playerObj && playerObj->GetTransform()) {
		playerRotation_ = playerObj->GetTransform()->rotate;
	}

	// 速度の計算（簡易版）
	static Vector3 previousPosition = playerPosition_;
	playerVelocity_ = {
		(playerPosition_.x - previousPosition.x) * 60.0f, // 60FPS想定
		(playerPosition_.y - previousPosition.y) * 60.0f,
		(playerPosition_.z - previousPosition.z) * 60.0f};
	previousPosition = playerPosition_;

	// 速度の計算
	currentSpeed_ = sqrtf(playerVelocity_.x * playerVelocity_.x +
						  playerVelocity_.y * playerVelocity_.y +
						  playerVelocity_.z * playerVelocity_.z);

	// G力の計算（簡易版：速度変化を基に）
	static float previousSpeed = currentSpeed_;
	float acceleration = (currentSpeed_ - previousSpeed) * 60.0f; // 60FPS想定
	currentGForce_ = 1.0f + acceleration / 9.8f;				  // 重力加速度で正規化
	previousSpeed = currentSpeed_;

	// 高度
	currentAltitude_ = playerPosition_.y;

	// 方位角（Y軸回転をヘディングに変換）
	currentHeading_ = RadiansToDegrees(playerRotation_.y);
	while (currentHeading_ < 0.0f)
		currentHeading_ += 360.0f;
	while (currentHeading_ >= 360.0f)
		currentHeading_ -= 360.0f;

	// ブーストゲージの取得
	currentBoostGauge_ = player->GetBoostGauge();
	maxBoostGauge_ = player->GetMaxBoostGauge();
	isBarrelRolling_ = player->IsBarrelRolling();
	barrelRollProgress_ = player->GetBarrelRollProgress();
}

///=============================================================================
///                        描画
void HUD::Draw() {
	// FollowCameraまたは現在のカメラが存在しない場合は描画しない
	Camera *currentCamera = nullptr;

	if (followCamera_) {
		currentCamera = followCamera_->GetCamera();
	}

	if (!currentCamera) {
		currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
	}

	if (!currentCamera) {
		return;
	}

	// 展開進行度が0の場合は描画しない
	if (deployProgress_ <= 0.0f) {
		return;
	}

	// HUDの中心位置を更新
	screenCenter_ = GetHUDPosition(0.0f, 0.0f);

	// 各要素の展開進行度を計算して描画
	float frameProgress = std::max(0.0f, (deployProgress_ - frameDeployStart_) / (1.0f - frameDeployStart_));
	if (frameProgress > 0.0f) {
		DrawHUDFrame(frameProgress);
	}

	float pitchProgress = std::max(0.0f, (deployProgress_ - pitchLadderDeployStart_) / (1.0f - pitchLadderDeployStart_));
	if (showPitchLadder_ && pitchProgress > 0.0f) {
		DrawPitchLadder(pitchProgress);
	}

	float boresightProgress = std::max(0.0f, (deployProgress_ - boresightDeployStart_) / (1.0f - boresightDeployStart_));
	if (showBoresight_ && boresightProgress > 0.0f) {
		DrawBoresight(boresightProgress);
	}

	float velocityProgress = std::max(0.0f, (deployProgress_ - velocityVectorDeployStart_) / (1.0f - velocityVectorDeployStart_));
	if (showVelocityVector_ && velocityProgress > 0.0f) {
		DrawVelocityVector(velocityProgress);
	}

	float flightPathProgress = std::max(0.0f, (deployProgress_ - velocityVectorDeployStart_) / (1.0f - velocityVectorDeployStart_));
	if (showFlightPath_ && flightPathProgress > 0.0f) {
		DrawFlightPathMarker(flightPathProgress);
	}

	float rollProgress = std::max(0.0f, (deployProgress_ - rollScaleDeployStart_) / (1.0f - rollScaleDeployStart_));
	if (showRollScale_ && rollProgress > 0.0f) {
		float rollDeg = RadiansToDegrees(playerRotation_.z);
		DrawRollScale(rollDeg, rollProgress);
	}

	float speedProgress = std::max(0.0f, (deployProgress_ - speedTapeDeployStart_) / (1.0f - speedTapeDeployStart_));
	if (showSpeedIndicator_ && speedProgress > 0.0f) {
		DrawSpeedTape(speedProgress);
	}

	float altProgress = std::max(0.0f, (deployProgress_ - altitudeTapeDeployStart_) / (1.0f - altitudeTapeDeployStart_));
	if (showAltitudeIndicator_ && altProgress > 0.0f) {
		DrawAltitudeTape(altProgress);
		DrawRadarAltitude(currentAltitude_, altProgress);
	}

	float headingProgress = std::max(0.0f, (deployProgress_ - headingTapeDeployStart_) / (1.0f - headingTapeDeployStart_));
	if (showCompass_ && headingProgress > 0.0f) {
		DrawHeadingTape(headingProgress);
	}

	float gForceProgress = std::max(0.0f, (deployProgress_ - gForceDeployStart_) / (1.0f - gForceDeployStart_));
	if (showGForce_ && gForceProgress > 0.0f) {
		DrawGForceIndicator(gForceProgress);
	}

	// ブーストゲージの描画
	float boostProgress = std::max(0.0f, (deployProgress_ - boostGaugeDeployStart_) / (1.0f - boostGaugeDeployStart_));
	if (showBoostGauge_ && boostProgress > 0.0f) {
		DrawBoostGauge(boostProgress);
	}

	// バレルロールインジケーターの描画
	float barrelRollProgress = std::max(0.0f, (deployProgress_ - barrelRollIndicatorDeployStart_) / (1.0f - barrelRollIndicatorDeployStart_));
	if (showBarrelRollIndicator_ && barrelRollProgress > 0.0f && isBarrelRolling_) {
		DrawBarrelRollIndicator(barrelRollProgress);
	}
}

///=============================================================================
///                        ガンボアサイト（画面中央、プレイヤー機首方向固定）
void HUD::DrawBoresight(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float size = 2.0f * hudScale_;
	Vector3 centerPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);

	// 水平線（中央から左右に展開）
	if (progress > 0.0f) {
		Vector3 leftPos = GetPlayerFrontPositionWithOffset(-size + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);
		Vector3 rightPos = GetPlayerFrontPositionWithOffset(size + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);
		Vector3 leftDraw = Lerp(centerPos, leftPos, progress);
		Vector3 rightDraw = Lerp(centerPos, rightPos, progress);
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_);
	}

	// 垂直線（中央から上下に展開）
	if (progress > 0.25f) {
		float vProgress = (progress - 0.25f) / 0.75f;
		Vector3 topPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, size + boresightOffset_.y, boresightOffset_);
		Vector3 bottomPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, -size + boresightOffset_.y, boresightOffset_);
		Vector3 topDraw = Lerp(centerPos, topPos, vProgress);
		Vector3 bottomDraw = Lerp(centerPos, bottomPos, vProgress);
		lineManager->DrawLine(topDraw, bottomDraw, hudColor_);
	}

	// 中央の小さな円（円弧として展開）
	if (progress > 0.5f) {
		float circleProgress = (progress - 0.5f) / 0.5f;
		float averageSize = (hudSizeX_ + hudSizeY_) * 0.5f;
		int segments = static_cast<int>(12 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(centerPos, 0.5f * hudScale_ * averageSize, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}
}

///=============================================================================
///                        ロールスケール（画面上部中央、-60°～+60°の円弧）
void HUD::DrawRollScale(float rollAngle, float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float radius = 8.0f * hudScale_;
	Vector3 arcCenter = GetPlayerFrontPositionWithOffset(0.0f + rollScaleOffset_.x, radius + rollScaleOffset_.y, rollScaleOffset_);

	// スケール目盛り（-60度から+60度まで30度間隔、進行度に応じて順次表示）
	int maxTicks = static_cast<int>(5 * progress); // 5本の目盛り
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

		lineManager->DrawLine(outerPoint, innerPoint, hudColor_);
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

		// 三角形を順次描画
		if (indicatorProgress > 0.33f) {
			lineManager->DrawLine(rollIndicator, tri1, hudColor_);
		}
		if (indicatorProgress > 0.66f) {
			lineManager->DrawLine(rollIndicator, tri2, hudColor_);
			lineManager->DrawLine(tri1, tri2, hudColor_);
		}
	}
}

///=============================================================================
///                        レーダー高度計（高度計の下、地面までの距離）
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
///                        HUDフレーム（画面四隅のコーナーマーカー）
void HUD::DrawHUDFrame(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float cornerSize = 2.0f;
	float frameSize = 15.0f;

	// 各コーナーを順次展開（左上→右上→左下→右下）
	// 左上
	if (progress > 0.0f) {
		float cornerProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 leftTopH1 = GetHUDPosition(-frameSize, frameSize);
		Vector3 leftTopH2 = GetHUDPosition(-frameSize + cornerSize, frameSize);
		Vector3 leftTopH2Draw = Lerp(leftTopH1, leftTopH2, cornerProgress);
		lineManager->DrawLine(leftTopH1, leftTopH2Draw, hudColor_);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 leftTopV2 = GetHUDPosition(-frameSize, frameSize - cornerSize);
			Vector3 leftTopV2Draw = Lerp(leftTopH1, leftTopV2, vProgress);
			lineManager->DrawLine(leftTopH1, leftTopV2Draw, hudColor_);
		}
	}

	// 右上
	if (progress > 0.25f) {
		float cornerProgress = std::min((progress - 0.25f) / 0.25f, 1.0f);
		Vector3 rightTopH1 = GetHUDPosition(frameSize, frameSize);
		Vector3 rightTopH2 = GetHUDPosition(frameSize - cornerSize, frameSize);
		Vector3 rightTopH2Draw = Lerp(rightTopH1, rightTopH2, cornerProgress);
		lineManager->DrawLine(rightTopH1, rightTopH2Draw, hudColor_);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 rightTopV2 = GetHUDPosition(frameSize, frameSize - cornerSize);
			Vector3 rightTopV2Draw = Lerp(rightTopH1, rightTopV2, vProgress);
			lineManager->DrawLine(rightTopH1, rightTopV2Draw, hudColor_);
		}
	}

	// 左下
	if (progress > 0.5f) {
		float cornerProgress = std::min((progress - 0.5f) / 0.25f, 1.0f);
		Vector3 leftBottomH1 = GetHUDPosition(-frameSize, -frameSize);
		Vector3 leftBottomH2 = GetHUDPosition(-frameSize + cornerSize, -frameSize);
		Vector3 leftBottomH2Draw = Lerp(leftBottomH1, leftBottomH2, cornerProgress);
		lineManager->DrawLine(leftBottomH1, leftBottomH2Draw, hudColor_);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 leftBottomV2 = GetHUDPosition(-frameSize, -frameSize + cornerSize);
			Vector3 leftBottomV2Draw = Lerp(leftBottomH1, leftBottomV2, vProgress);
			lineManager->DrawLine(leftBottomH1, leftBottomV2Draw, hudColor_);
		}
	}

	// 右下
	if (progress > 0.75f) {
		float cornerProgress = (progress - 0.75f) / 0.25f;
		Vector3 rightBottomH1 = GetHUDPosition(frameSize, -frameSize);
		Vector3 rightBottomH2 = GetHUDPosition(frameSize - cornerSize, -frameSize);
		Vector3 rightBottomH2Draw = Lerp(rightBottomH1, rightBottomH2, cornerProgress);
		lineManager->DrawLine(rightBottomH1, rightBottomH2Draw, hudColor_);

		if (cornerProgress > 0.5f) {
			float vProgress = (cornerProgress - 0.5f) / 0.5f;
			Vector3 rightBottomV2 = GetHUDPosition(frameSize, -frameSize + cornerSize);
			Vector3 rightBottomV2Draw = Lerp(rightBottomH1, rightBottomV2, vProgress);
			lineManager->DrawLine(rightBottomH1, rightBottomV2Draw, hudColor_);
		}
	}
}

///=============================================================================
///                        ベロシティベクトル（機体進行方向）
void HUD::DrawVelocityVector(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float size = 1.2f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, 0.0f, boresightOffset_);

	// 円形マーカー（円弧として展開）
	if (progress > 0.0f) {
		float circleProgress = std::min(progress / 0.6f, 1.0f);
		int segments = static_cast<int>(16 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(center, size, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// 左翼（進行度60%以降）
	if (progress > 0.6f) {
		float wingProgress = (progress - 0.6f) / 0.2f;
		Vector3 leftWing = GetPlayerFrontPositionWithOffset(-size * 1.5f, 0.0f, boresightOffset_);
		Vector3 leftWingEnd = GetPlayerFrontPositionWithOffset(-size * 2.5f, 0.0f, boresightOffset_);
		Vector3 leftWingDraw = Lerp(leftWing, leftWingEnd, wingProgress);
		lineManager->DrawLine(leftWing, leftWingDraw, hudColor_, 2.0f);
	}

	// 右翼（進行度80%以降）
	if (progress > 0.8f) {
		float wingProgress = (progress - 0.8f) / 0.2f;
		Vector3 rightWing = GetPlayerFrontPositionWithOffset(size * 1.5f, 0.0f, boresightOffset_);
		Vector3 rightWingEnd = GetPlayerFrontPositionWithOffset(size * 2.5f, 0.0f, boresightOffset_);
		Vector3 rightWingDraw = Lerp(rightWing, rightWingEnd, wingProgress);
		lineManager->DrawLine(rightWing, rightWingDraw, hudColor_, 2.0f);
	}
}

///=============================================================================
///                        フライトパスマーカー（実際の移動方向）
void HUD::DrawFlightPathMarker(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	if (currentSpeed_ < 0.1f) {
		return;
	}

	float velocityOffsetX = playerVelocity_.x * 0.5f;
	float velocityOffsetY = playerVelocity_.y * 0.5f;

	float size = 0.8f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY, boresightOffset_);

	// 円形マーカー（円弧として展開）
	if (progress > 0.0f) {
		float circleProgress = std::min(progress / 0.5f, 1.0f);
		int segments = static_cast<int>(12 * circleProgress);
		if (segments > 0) {
			lineManager->DrawCircle(center, size, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, segments);
		}
	}

	// 上の線（進行度50%以降）
	if (progress > 0.5f) {
		float lineProgress = (progress - 0.5f) / 0.5f;
		Vector3 top = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 1.5f, boresightOffset_);
		Vector3 topEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 0.8f, boresightOffset_);
		Vector3 topDraw = Lerp(top, topEnd, lineProgress);
		lineManager->DrawLine(top, topDraw, hudColor_, 2.0f);
	}

	// 下の線（進行度75%以降）
	if (progress > 0.75f) {
		float lineProgress = (progress - 0.75f) / 0.25f;
		Vector3 bottom = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 1.5f, boresightOffset_);
		Vector3 bottomEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 0.8f, boresightOffset_);
		Vector3 bottomDraw = Lerp(bottom, bottomEnd, lineProgress);
		lineManager->DrawLine(bottom, bottomDraw, hudColor_, 2.0f);
	}
}

///=============================================================================
///                        ピッチラダー（水平線と角度表示）
void HUD::DrawPitchLadder(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float pitchDeg = RadiansToDegrees(playerRotation_.x);

	// -30度から+30度まで10度間隔で水平線を描画（進行度に応じて本数を増やす）
	int maxLines = static_cast<int>(7 * progress); // 最大7本
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
		if (std::abs(offsetY) > 15.0f)
			continue;

		float lineLength = (angle % 20 == 0) ? 4.0f : 3.0f;
		Vector3 left = GetPlayerFrontPositionWithOffset(-lineLength, offsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(lineLength, offsetY, boresightOffset_);

		// 中央から左右に展開
		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, offsetY, boresightOffset_);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		Vector4 lineColor = (angle > 0) ? hudColor_ : hudColor_;
		lineManager->DrawLine(leftDraw, rightDraw, lineColor, 1.5f);

		// 角度表示用の短い縦線（進行度80%以降）
		if (angle % 20 == 0 && progress > 0.8f) {
			Vector3 leftTick = GetPlayerFrontPositionWithOffset(-lineLength - 0.3f, offsetY, boresightOffset_);
			Vector3 leftTickEnd = GetPlayerFrontPositionWithOffset(-lineLength - 0.3f, offsetY + 0.5f, boresightOffset_);
			lineManager->DrawLine(leftTick, leftTickEnd, lineColor, 1.5f);

			Vector3 rightTick = GetPlayerFrontPositionWithOffset(lineLength + 0.3f, offsetY, boresightOffset_);
			Vector3 rightTickEnd = GetPlayerFrontPositionWithOffset(lineLength + 0.3f, offsetY + 0.5f, boresightOffset_);
			lineManager->DrawLine(rightTick, rightTickEnd, lineColor, 1.5f);
		}
	}

	// 0度（水平線）は太く強調表示（常に最初に表示）
	float horizonOffsetY = -pitchDeg * 0.3f;
	if (std::abs(horizonOffsetY) <= 15.0f && progress > 0.0f) {
		Vector3 left = GetPlayerFrontPositionWithOffset(-8.0f, horizonOffsetY, boresightOffset_);
		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, horizonOffsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(8.0f, horizonOffsetY, boresightOffset_);

		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);

		lineManager->DrawLine(leftDraw, center, hudColor_, 3.0f);
		lineManager->DrawLine(center, rightDraw, hudColor_, 3.0f);
	}
}

///=============================================================================
///                        速度テープ（左側）
void HUD::DrawSpeedTape(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeX = -13.0f;
	float tapeY = 0.0f;

	Vector3 topLeft = GetHUDPosition(tapeX - 1.5f, tapeY + 6.0f);
	Vector3 topRight = GetHUDPosition(tapeX + 1.5f, tapeY + 6.0f);
	Vector3 bottomLeft = GetHUDPosition(tapeX - 1.5f, tapeY - 6.0f);
	Vector3 bottomRight = GetHUDPosition(tapeX + 1.5f, tapeY - 6.0f);

	// 枠を順次描画（上→左→下→右）
	if (progress > 0.0f) {
		float lineProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 topRightDraw = Lerp(topLeft, topRight, lineProgress);
		lineManager->DrawLine(topLeft, topRightDraw, hudColor_);
	}
	if (progress > 0.25f) {
		float lineProgress = (progress - 0.25f) / 0.25f;
		Vector3 bottomLeftDraw = Lerp(topLeft, bottomLeft, lineProgress);
		lineManager->DrawLine(topLeft, bottomLeftDraw, hudColor_);
	}
	if (progress > 0.5f) {
		float lineProgress = (progress - 0.5f) / 0.25f;
		Vector3 bottomRightDraw = Lerp(bottomLeft, bottomRight, lineProgress);
		lineManager->DrawLine(bottomLeft, bottomRightDraw, hudColor_);
	}
	if (progress > 0.75f) {
		float lineProgress = (progress - 0.75f) / 0.25f;
		Vector3 bottomRightDraw = Lerp(topRight, bottomRight, lineProgress);
		lineManager->DrawLine(topRight, bottomRightDraw, hudColor_);
	}

	// 現在の速度マーカー（進行度60%以降）
	if (progress > 0.6f) {
		Vector3 speedMarkerLeft = GetHUDPosition(tapeX - 2.0f, tapeY);
		Vector3 speedMarkerRight = GetHUDPosition(tapeX + 2.0f, tapeY);
		Vector3 speedMarkerTop = GetHUDPosition(tapeX, tapeY + 0.8f);
		Vector3 speedMarkerBottom = GetHUDPosition(tapeX, tapeY - 0.8f);

		lineManager->DrawLine(speedMarkerLeft, speedMarkerTop, hudColor_, 2.0f);
		lineManager->DrawLine(speedMarkerTop, speedMarkerRight, hudColor_, 2.0f);
		lineManager->DrawLine(speedMarkerRight, speedMarkerBottom, hudColor_, 2.0f);
		lineManager->DrawLine(speedMarkerBottom, speedMarkerLeft, hudColor_, 2.0f);
	}

	// 速度目盛り（進行度80%以降で表示）
	if (progress > 0.8f) {
		int baseSpeed = static_cast<int>(currentSpeed_ / 10) * 10;
		for (int i = -3; i <= 3; ++i) {
			int speed = baseSpeed + i * 10;
			if (speed < 0)
				continue;

			float offsetY = (currentSpeed_ - speed) * 0.3f;
			Vector3 tickStart = GetHUDPosition(tapeX - 1.5f, tapeY + offsetY);
			Vector3 tickEnd = GetHUDPosition(tapeX - 0.8f, tapeY + offsetY);

			lineManager->DrawLine(tickStart, tickEnd, hudColor_);
		}
	}
}

///=============================================================================
///                        高度テープ（右側）
void HUD::DrawAltitudeTape(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeX = 13.0f;
	float tapeY = 0.0f;

	Vector3 topLeft = GetHUDPosition(tapeX - 1.5f, tapeY + 6.0f);
	Vector3 topRight = GetHUDPosition(tapeX + 1.5f, tapeY + 6.0f);
	Vector3 bottomLeft = GetHUDPosition(tapeX - 1.5f, tapeY - 6.0f);
	Vector3 bottomRight = GetHUDPosition(tapeX + 1.5f, tapeY - 6.0f);

	// 枠を順次描画（上→右→下→左）
	if (progress > 0.0f) {
		float lineProgress = std::min(progress / 0.25f, 1.0f);
		Vector3 topRightDraw = Lerp(topLeft, topRight, lineProgress);
		lineManager->DrawLine(topLeft, topRightDraw, hudColor_);
	}
	if (progress > 0.25f) {
		float lineProgress = (progress - 0.25f) / 0.25f;
		Vector3 bottomRightDraw = Lerp(topRight, bottomRight, lineProgress);
		lineManager->DrawLine(topRight, bottomRightDraw, hudColor_);
	}
	if (progress > 0.5f) {
		float lineProgress = (progress - 0.5f) / 0.25f;
		Vector3 bottomLeftDraw = Lerp(bottomRight, bottomLeft, lineProgress);
		lineManager->DrawLine(bottomRight, bottomLeftDraw, hudColor_);
	}
	if (progress > 0.75f) {
		float lineProgress = (progress - 0.75f) / 0.25f;
		Vector3 topLeftDraw = Lerp(bottomLeft, topLeft, lineProgress);
		lineManager->DrawLine(bottomLeft, topLeftDraw, hudColor_);
	}

	// 現在の高度マーカー（進行度60%以降）
	if (progress > 0.6f) {
		Vector3 altMarkerLeft = GetHUDPosition(tapeX - 2.0f, tapeY);
		Vector3 altMarkerRight = GetHUDPosition(tapeX + 2.0f, tapeY);
		Vector3 altMarkerTop = GetHUDPosition(tapeX, tapeY + 0.8f);
		Vector3 altMarkerBottom = GetHUDPosition(tapeX, tapeY - 0.8f);

		Vector4 altColor = (currentAltitude_ < 20.0f) ? hudColorCritical_ : hudColor_;

		lineManager->DrawLine(altMarkerLeft, altMarkerTop, altColor, 2.0f);
		lineManager->DrawLine(altMarkerTop, altMarkerRight, altColor, 2.0f);
		lineManager->DrawLine(altMarkerRight, altMarkerBottom, altColor, 2.0f);
		lineManager->DrawLine(altMarkerBottom, altMarkerLeft, altColor, 2.0f);
	}

	// 高度目盛り（進行度80%以降で表示）
	if (progress > 0.8f) {
		int baseAlt = static_cast<int>(currentAltitude_ / 10) * 10;
		for (int i = -3; i <= 3; ++i) {
			int alt = baseAlt + i * 10;
			if (alt < 0)
				continue;

			float offsetY = (currentAltitude_ - alt) * 0.3f;
			Vector3 tickStart = GetHUDPosition(tapeX + 1.5f, tapeY + offsetY);
			Vector3 tickEnd = GetHUDPosition(tapeX + 0.8f, tapeY + offsetY);

			lineManager->DrawLine(tickStart, tickEnd, hudColor_);
		}
	}
}

///=============================================================================
///                        方位テープ（上部）
void HUD::DrawHeadingTape(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeY = 8.0f;

	// ベースライン（中央から左右に展開）
	if (progress > 0.0f) {
		Vector3 center = GetHUDPosition(0.0f, tapeY);
		Vector3 left = GetHUDPosition(-6.0f, tapeY);
		Vector3 right = GetHUDPosition(6.0f, tapeY);
		Vector3 leftDraw = Lerp(center, left, progress);
		Vector3 rightDraw = Lerp(center, right, progress);
		lineManager->DrawLine(leftDraw, rightDraw, hudColor_);
	}

	// 中央マーカー（進行度50%以降）
	if (progress > 0.5f) {
		float markerProgress = (progress - 0.5f) / 0.5f;
		Vector3 centerTop = GetHUDPosition(0.0f, tapeY + 1.0f);
		Vector3 centerBottom = GetHUDPosition(0.0f, tapeY);
		Vector3 centerLeft = GetHUDPosition(-0.5f, tapeY + 0.5f);
		Vector3 centerRight = GetHUDPosition(0.5f, tapeY + 0.5f);

		Vector3 centerTopDraw = Lerp(centerBottom, centerTop, markerProgress);
		lineManager->DrawLine(centerTopDraw, centerBottom, hudColor_, 2.0f);

		if (markerProgress > 0.5f) {
			lineManager->DrawLine(centerTop, centerLeft, hudColor_, 2.0f);
			lineManager->DrawLine(centerTop, centerRight, hudColor_, 2.0f);
		}
	}

	// 方位目盛り（進行度70%以降で表示）
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
			lineManager->DrawLine(tickTop, tickBottom, hudColor_);
		}
	}
}

///=============================================================================
///                        G-Force表示（左下）
void HUD::DrawGForceIndicator(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float posX = -13.0f;
	float posY = -8.0f;

	// G-Forceバーの枠（左から右に展開）
	if (progress > 0.0f) {
		Vector3 barLeft = GetHUDPosition(posX, posY);
		Vector3 barRight = GetHUDPosition(posX + 4.0f, posY);
		Vector3 barRightDraw = Lerp(barLeft, barRight, progress);
		lineManager->DrawLine(barLeft, barRightDraw, hudColor_);
	}

	// G-Force値バー（進行度50%以降で表示）
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
		lineManager->DrawLine(barLeft, gBarDraw, gColor, 3.0f);
	}
}

///=============================================================================
///                        ブーストゲージ表示（左下、G-Forceの下）
void HUD::DrawBoostGauge(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float posX = -13.0f;
	float posY = -10.0f;

	// ゲージ枠（左から右に展開）
	if (progress > 0.0f) {
		Vector3 frameLeft = GetHUDPosition(posX, posY);
		Vector3 frameRight = GetHUDPosition(posX + 4.0f, posY);
		Vector3 frameRightDraw = Lerp(frameLeft, frameRight, progress);
		lineManager->DrawLine(frameLeft, frameRightDraw, hudColor_);

		// 上下の枠線
		if (progress > 0.3f) {
			Vector3 frameTop = GetHUDPosition(posX + 4.0f, posY + 0.3f);
			Vector3 frameBottom = GetHUDPosition(posX + 4.0f, posY - 0.3f);
			lineManager->DrawLine(frameTop, frameBottom, hudColor_);
		}
	}

	// ゲージ本体（進行度50%以降）
	if (progress > 0.5f) {
		float gaugeProgress = (progress - 0.5f) / 0.5f;
		float gaugeRatio = currentBoostGauge_ / maxBoostGauge_;
		float gaugeLength = 4.0f * gaugeRatio;

		Vector4 gaugeColor = hudColor_;
		if (gaugeRatio < 0.3f) {
			gaugeColor = hudColorWarning_;
		}

		Vector3 gaugeLeft = GetHUDPosition(posX, posY);
		Vector3 gaugeRight = GetHUDPosition(posX + gaugeLength, posY);
		Vector3 gaugeRightDraw = Lerp(gaugeLeft, gaugeRight, gaugeProgress);
		lineManager->DrawLine(gaugeLeft, gaugeRightDraw, gaugeColor, 4.0f);
	}
}

///=============================================================================
///                        バレルロールインジケーター（画面中央上部）
void HUD::DrawBarrelRollIndicator(float progress) {
	LineManager *lineManager = LineManager::GetInstance();

	float posY = 5.0f;

	// "BARREL ROLL"テキスト風の線（簡易表現）
	Vector3 textCenter = GetHUDPosition(0.0f, posY);

	// 点滅効果
	int blinkCycle = static_cast<int>(barrelRollProgress_ * 20.0f);
	if (blinkCycle % 2 == 0) {
		// 左右の矢印（回転方向を示唆）
		Vector3 leftArrow1 = GetHUDPosition(-3.0f, posY);
		Vector3 leftArrow2 = GetHUDPosition(-2.0f, posY + 0.5f);
		Vector3 leftArrow3 = GetHUDPosition(-2.0f, posY - 0.5f);

		lineManager->DrawLine(leftArrow1, leftArrow2, hudColor_, 2.0f);
		lineManager->DrawLine(leftArrow1, leftArrow3, hudColor_, 2.0f);

		Vector3 rightArrow1 = GetHUDPosition(3.0f, posY);
		Vector3 rightArrow2 = GetHUDPosition(2.0f, posY + 0.5f);
		Vector3 rightArrow3 = GetHUDPosition(2.0f, posY - 0.5f);

		lineManager->DrawLine(rightArrow1, rightArrow2, hudColor_, 2.0f);
		lineManager->DrawLine(rightArrow1, rightArrow3, hudColor_, 2.0f);

		// 中央の円（回転を示唆）
		lineManager->DrawCircle(textCenter, 0.8f, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 16);
	}

	// 進行度バー
	Vector3 progressBarLeft = GetHUDPosition(-2.0f, posY - 1.5f);
	Vector3 progressBarRight = GetHUDPosition(-2.0f + 4.0f * barrelRollProgress_, posY - 1.5f);
	lineManager->DrawLine(progressBarLeft, progressBarRight, hudColor_, 3.0f);
}

///=============================================================================
///                        ImGui描画
void HUD::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("HUD Settings");

	ImGui::Text("HUD Display Control");
	ImGui::Checkbox("Show Boresight", &showBoresight_);
	ImGui::Checkbox("Show Pitch Ladder", &showPitchLadder_);
	ImGui::Checkbox("Show Roll Scale", &showRollScale_);
	ImGui::Checkbox("Show Velocity Vector", &showVelocityVector_);
	ImGui::Checkbox("Show Flight Path", &showFlightPath_);
	ImGui::Checkbox("Show Speed Indicator", &showSpeedIndicator_);
	ImGui::Checkbox("Show Altitude Indicator", &showAltitudeIndicator_);
	ImGui::Checkbox("Show Compass", &showCompass_);
	ImGui::Checkbox("Show G-Force", &showGForce_);
	ImGui::Checkbox("Show Boost Gauge", &showBoostGauge_);					  // 追加
	ImGui::Checkbox("Show Barrel Roll Indicator", &showBarrelRollIndicator_); // 追加

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
	ImGui::SliderFloat("Animation Duration", &animationDuration_, 0.5f, 3.0f);

	ImGui::Separator();
	ImGui::Text("Element Deploy Timing");
	ImGui::SliderFloat("Frame Start", &frameDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Boresight Start", &boresightDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Pitch Ladder Start", &pitchLadderDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Velocity Vector Start", &velocityVectorDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Roll Scale Start", &rollScaleDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Speed Tape Start", &speedTapeDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Altitude Tape Start", &altitudeTapeDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Heading Tape Start", &headingTapeDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("G-Force Start", &gForceDeployStart_, 0.0f, 0.5f);
	ImGui::SliderFloat("Boost Gauge Start", &boostGaugeDeployStart_, 0.0f, 0.5f);					 // 追加
	ImGui::SliderFloat("Barrel Roll Indicator Start", &barrelRollIndicatorDeployStart_, 0.0f, 0.5f); // 追加

	ImGui::Separator();
	ImGui::SliderFloat("HUD Scale", &hudScale_, 0.5f, 2.0f);
	ImGui::SliderFloat("HUD Distance", &hudDistance_, 5.0f, 50.0f);
	ImGui::SliderFloat("HUD Width", &hudSizeX_, 0.1f, 3.0f);
	ImGui::SliderFloat("HUD Height", &hudSizeY_, 0.1f, 3.0f);
	ImGui::ColorEdit4("HUD Color", &hudColor_.x);
	ImGui::ColorEdit4("Warning Color", &hudColorWarning_.x);
	ImGui::ColorEdit4("Critical Color", &hudColorCritical_.x);

	ImGui::Separator();
	ImGui::Text("Player Front HUD Positions:");
	ImGui::DragFloat3("Boresight Offset", &boresightOffset_.x, 0.1f, -20.0f, 20.0f);
	ImGui::DragFloat3("Roll Scale Offset", &rollScaleOffset_.x, 0.1f, -20.0f, 20.0f);

	ImGui::Separator();
	ImGui::Text("Current Values:");
	ImGui::Text("Speed: %.1f m/s", currentSpeed_);
	ImGui::Text("Altitude: %.1f m", currentAltitude_);
	ImGui::Text("Heading: %.1f deg", currentHeading_);
	ImGui::Text("Pitch: %.1f deg", RadiansToDegrees(playerRotation_.x));
	ImGui::Text("Roll: %.1f deg", RadiansToDegrees(playerRotation_.z));
	ImGui::Text("G-Force: %.2f G", currentGForce_);
	ImGui::Text("Boost: %.1f / %.1f", currentBoostGauge_, maxBoostGauge_); // 追加
	ImGui::Text("Barrel Rolling: %s", isBarrelRolling_ ? "Yes" : "No");	   // 追加

	// デバッグ情報追加
	ImGui::Separator();
	ImGui::Text("Debug Info:");

	// 使用中のカメラ情報を表示
	Camera *currentCamera = nullptr;
	std::string cameraSource = "None";

	if (followCamera_) {
		currentCamera = followCamera_->GetCamera();
		cameraSource = "FollowCamera";
	}

	if (!currentCamera) {
		currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
		cameraSource = "CameraManager";
	}

	ImGui::Text("Camera Source: %s", cameraSource.c_str());

	if (currentCamera) {
		Vector3 camPos = currentCamera->GetTransform().translate;
		Vector3 camRot = currentCamera->GetTransform().rotate;
		ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rot: (%.2f, %.2f, %.2f)",
					RadiansToDegrees(camRot.x),
					RadiansToDegrees(camRot.y),
					RadiansToDegrees(camRot.z));

		Vector3 hudCenter = GetHUDPosition(0.0f, 0.0f);
		ImGui::Text("HUD Center: (%.2f, %.2f, %.2f)", hudCenter.x, hudCenter.y, hudCenter.z);
	}

	ImGui::End();
#endif // _DEBUG
}

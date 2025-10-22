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
}

///=============================================================================
///                        初期化
void HUD::Initialize() {
	screenCenter_ = {0.0f, -3.0f, 0.0f};
	hudScale_ = 1.0f;
	hudColor_ = {0.0f, 1.0f, 0.0f, 1.0f};		  // 緑色のHUD
	hudColorWarning_ = {1.0f, 1.0f, 0.0f, 1.0f};  // 黄色（警告）
	hudColorCritical_ = {1.0f, 0.0f, 0.0f, 1.0f}; // 赤色（危険）
	hudDistance_ = 15.0f;						  // カメラから15単位前方にHUDを配置
	hudSizeX_ = 0.4f;							  // HUD横幅サイズ倍率
	hudSizeY_ = 0.250f;							  // HUD縦幅サイズ倍率

	// プレイヤー正面HUD要素の位置調整初期化
	boresightOffset_ = {0.0f, -3.0f, 0.0f};	// ガンボアサイトのオフセット
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

	// データの初期化
	playerPosition_ = {0.0f, 0.0f, 0.0f};
	playerRotation_ = {0.0f, 0.0f, 0.0f};
	playerVelocity_ = {0.0f, 0.0f, 0.0f};
	currentGForce_ = 1.0f;
	currentSpeed_ = 0.0f;
	currentAltitude_ = 0.0f;
	currentHeading_ = 0.0f;
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

	// HUDの中心位置を更新
	screenCenter_ = GetHUDPosition(0.0f, 0.0f);

	// HUDフレームの描画（画面四隅）
	DrawHUDFrame();

	// 【画面中央】ピッチラダー（水平線と角度表示）
	if (showPitchLadder_) {
		DrawPitchLadder();
	}

	// 【画面中央】ガンボアサイト（照準）
	if (showBoresight_) {
		DrawBoresight();
	}

	// 【画面中央】ベロシティベクトル
	if (showVelocityVector_) {
		DrawVelocityVector();
	}

	// 【画面中央】フライトパスマーカー
	if (showFlightPath_) {
		DrawFlightPathMarker();
	}

	// 【画面上部中央】ロールスケール（-60°～+60°の円弧）
	if (showRollScale_) {
		float rollDeg = RadiansToDegrees(playerRotation_.z);
		DrawRollScale(rollDeg);
	}

	// 【画面左側】速度テープ
	if (showSpeedIndicator_) {
		DrawSpeedTape();
	}

	// 【画面右側】高度テープ
	if (showAltitudeIndicator_) {
		DrawAltitudeTape();
		DrawRadarAltitude(currentAltitude_);
	}

	// 【画面上部】方位テープ
	if (showCompass_) {
		DrawHeadingTape();
	}

	// 【画面左下】G-Force表示
	if (showGForce_) {
		DrawGForceIndicator();
	}
}

///=============================================================================
///                        ガンボアサイト（画面中央、プレイヤー機首方向固定）
void HUD::DrawBoresight() {
	LineManager *lineManager = LineManager::GetInstance();

	// 十字線（ボアサイト）
	float size = 2.0f * hudScale_;

	// プレイヤー正面座標系での位置を計算（オフセット適用）
	Vector3 leftPos = GetPlayerFrontPositionWithOffset(-size + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);
	Vector3 rightPos = GetPlayerFrontPositionWithOffset(size + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);
	Vector3 topPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, size + boresightOffset_.y, boresightOffset_);
	Vector3 bottomPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, -size + boresightOffset_.y, boresightOffset_);
	Vector3 centerPos = GetPlayerFrontPositionWithOffset(0.0f + boresightOffset_.x, 0.0f + boresightOffset_.y, boresightOffset_);

	// 水平線
	lineManager->DrawLine(leftPos, rightPos, hudColor_);

	// 垂直線
	lineManager->DrawLine(topPos, bottomPos, hudColor_);

	// 中央の小さな円（縦横サイズの平均を使用）
	float averageSize = (hudSizeX_ + hudSizeY_) * 0.5f;
	lineManager->DrawCircle(centerPos, 0.5f * hudScale_ * averageSize, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);
}

///=============================================================================
///                        ロールスケール（画面上部中央、-60°～+60°の円弧）
void HUD::DrawRollScale(float rollAngle) {
	LineManager *lineManager = LineManager::GetInstance();

	// ロールスケールの円弧
	float radius = 8.0f * hudScale_;
	Vector3 arcCenter = GetPlayerFrontPositionWithOffset(0.0f + rollScaleOffset_.x, radius + rollScaleOffset_.y, rollScaleOffset_);

	// スケール目盛り（-60度から+60度まで30度間隔）
	for (int angle = -60; angle <= 60; angle += 30) {
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

	// 現在のロール角指示器
	float rollRad = DegreesToRadians(rollAngle);
	float indicatorX = sinf(rollRad) * (radius - 0.5f);
	float indicatorY = radius - cosf(rollRad) * (radius - 0.5f);

	Vector3 rollIndicator = GetPlayerFrontPositionWithOffset(indicatorX + rollScaleOffset_.x, indicatorY + rollScaleOffset_.y, rollScaleOffset_);
	Vector3 tri1 = GetPlayerFrontPositionWithOffset(indicatorX - 0.5f + rollScaleOffset_.x, indicatorY - 1.0f + rollScaleOffset_.y, rollScaleOffset_);
	Vector3 tri2 = GetPlayerFrontPositionWithOffset(indicatorX + 0.5f + rollScaleOffset_.x, indicatorY - 1.0f + rollScaleOffset_.y, rollScaleOffset_);

	// 三角形の指示器
	lineManager->DrawLine(rollIndicator, tri1, hudColor_);
	lineManager->DrawLine(rollIndicator, tri2, hudColor_);
	lineManager->DrawLine(tri1, tri2, hudColor_);
}

///=============================================================================
///                        レーダー高度計（高度計の下、地面までの距離）
void HUD::DrawRadarAltitude(float radarAlt) {
	LineManager *lineManager = LineManager::GetInstance();

	// 高度計の下にレーダー高度表示
	Vector3 radarStart = GetHUDPosition(12.0f, -6.0f);

	// レーダー高度の簡易表示
	float radarLength = std::min(radarAlt / 100.0f * 2.0f, 4.0f);
	Vector3 radarEnd = GetHUDPosition(12.0f - radarLength, -6.0f);

	lineManager->DrawLine(radarStart, radarEnd, hudColor_);

	// 危険高度マーカー（50m）
	Vector3 dangerTop = GetHUDPosition(11.0f, -5.8f);
	Vector3 dangerBottom = GetHUDPosition(11.0f, -6.2f);
	lineManager->DrawLine(dangerTop, dangerBottom, {1.0f, 0.0f, 0.0f, 1.0f}); // 赤色
}

///=============================================================================
///                        HUDフレーム（画面四隅のコーナーマーカー）
void HUD::DrawHUDFrame() {
	LineManager *lineManager = LineManager::GetInstance();

	// HUDの外枠（オプション）
	// 角の小さなマーカー
	float cornerSize = 2.0f;
	float frameSize = 15.0f;

	// 左上
	Vector3 leftTopH1 = GetHUDPosition(-frameSize, frameSize);
	Vector3 leftTopH2 = GetHUDPosition(-frameSize + cornerSize, frameSize);
	Vector3 leftTopV1 = GetHUDPosition(-frameSize, frameSize);
	Vector3 leftTopV2 = GetHUDPosition(-frameSize, frameSize - cornerSize);

	lineManager->DrawLine(leftTopH1, leftTopH2, hudColor_);
	lineManager->DrawLine(leftTopV1, leftTopV2, hudColor_);

	// 右上
	Vector3 rightTopH1 = GetHUDPosition(frameSize, frameSize);
	Vector3 rightTopH2 = GetHUDPosition(frameSize - cornerSize, frameSize);
	Vector3 rightTopV1 = GetHUDPosition(frameSize, frameSize);
	Vector3 rightTopV2 = GetHUDPosition(frameSize, frameSize - cornerSize);

	lineManager->DrawLine(rightTopH1, rightTopH2, hudColor_);
	lineManager->DrawLine(rightTopV1, rightTopV2, hudColor_);

	// 左下
	Vector3 leftBottomH1 = GetHUDPosition(-frameSize, -frameSize);
	Vector3 leftBottomH2 = GetHUDPosition(-frameSize + cornerSize, -frameSize);
	Vector3 leftBottomV1 = GetHUDPosition(-frameSize, -frameSize);
	Vector3 leftBottomV2 = GetHUDPosition(-frameSize, -frameSize + cornerSize);

	lineManager->DrawLine(leftBottomH1, leftBottomH2, hudColor_);
	lineManager->DrawLine(leftBottomV1, leftBottomV2, hudColor_);

	// 右下
	Vector3 rightBottomH1 = GetHUDPosition(frameSize, -frameSize);
	Vector3 rightBottomH2 = GetHUDPosition(frameSize - cornerSize, -frameSize);
	Vector3 rightBottomV1 = GetHUDPosition(frameSize, -frameSize);
	Vector3 rightBottomV2 = GetHUDPosition(frameSize, -frameSize + cornerSize);

	lineManager->DrawLine(rightBottomH1, rightBottomH2, hudColor_);
	lineManager->DrawLine(rightBottomV1, rightBottomV2, hudColor_);
}

///=============================================================================
///                        ベロシティベクトル（機体進行方向）
void HUD::DrawVelocityVector() {
	LineManager *lineManager = LineManager::GetInstance();

	// ベロシティベクトルは機体の向きを示す（機首方向）
	float size = 1.2f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, 0.0f, boresightOffset_);

	// 円形マーカー
	lineManager->DrawCircle(center, size, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 16);

	// 左右の小さな翼マーク
	Vector3 leftWing = GetPlayerFrontPositionWithOffset(-size * 1.5f, 0.0f, boresightOffset_);
	Vector3 leftWingEnd = GetPlayerFrontPositionWithOffset(-size * 2.5f, 0.0f, boresightOffset_);
	lineManager->DrawLine(leftWing, leftWingEnd, hudColor_, 2.0f);

	Vector3 rightWing = GetPlayerFrontPositionWithOffset(size * 1.5f, 0.0f, boresightOffset_);
	Vector3 rightWingEnd = GetPlayerFrontPositionWithOffset(size * 2.5f, 0.0f, boresightOffset_);
	lineManager->DrawLine(rightWing, rightWingEnd, hudColor_, 2.0f);
}

///=============================================================================
///                        フライトパスマーカー（実際の移動方向）
void HUD::DrawFlightPathMarker() {
	LineManager *lineManager = LineManager::GetInstance();

	if (currentSpeed_ < 0.1f) {
		return; // 速度が低い場合は表示しない
	}

	// 速度ベクトルに基づいてオフセットを計算
	float velocityOffsetX = playerVelocity_.x * 0.5f;
	float velocityOffsetY = playerVelocity_.y * 0.5f;

	float size = 0.8f * hudScale_;
	Vector3 center = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY, boresightOffset_);

	// 円形マーカー
	lineManager->DrawCircle(center, size, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);

	// 上下左右の短い線
	Vector3 top = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 1.5f, boresightOffset_);
	Vector3 topEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY + size * 0.8f, boresightOffset_);
	lineManager->DrawLine(top, topEnd, hudColor_, 2.0f);

	Vector3 bottom = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 1.5f, boresightOffset_);
	Vector3 bottomEnd = GetPlayerFrontPositionWithOffset(velocityOffsetX, velocityOffsetY - size * 0.8f, boresightOffset_);
	lineManager->DrawLine(bottom, bottomEnd, hudColor_, 2.0f);
}

///=============================================================================
///                        ピッチラダー（水平線と角度表示）
void HUD::DrawPitchLadder() {
	LineManager *lineManager = LineManager::GetInstance();

	float pitchDeg = RadiansToDegrees(playerRotation_.x);

	// -30度から+30度まで10度間隔で水平線を描画
	for (int angle = -30; angle <= 30; angle += 10) {
		if (angle == 0)
			continue; // 0度は別途描画

		float offsetY = (angle - pitchDeg) * 0.3f;
		if (std::abs(offsetY) > 15.0f)
			continue; // 画面外は描画しない

		float lineLength = (angle % 20 == 0) ? 4.0f : 3.0f;
		Vector3 left = GetPlayerFrontPositionWithOffset(-lineLength, offsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(lineLength, offsetY, boresightOffset_);

		Vector4 lineColor = (angle > 0) ? hudColor_ : hudColor_;
		lineManager->DrawLine(left, right, lineColor, 1.5f);

		// 角度表示用の短い縦線
		if (angle % 20 == 0) {
			Vector3 leftTick = GetPlayerFrontPositionWithOffset(-lineLength - 0.3f, offsetY, boresightOffset_);
			Vector3 leftTickEnd = GetPlayerFrontPositionWithOffset(-lineLength - 0.3f, offsetY + 0.5f, boresightOffset_);
			lineManager->DrawLine(leftTick, leftTickEnd, lineColor, 1.5f);

			Vector3 rightTick = GetPlayerFrontPositionWithOffset(lineLength + 0.3f, offsetY, boresightOffset_);
			Vector3 rightTickEnd = GetPlayerFrontPositionWithOffset(lineLength + 0.3f, offsetY + 0.5f, boresightOffset_);
			lineManager->DrawLine(rightTick, rightTickEnd, lineColor, 1.5f);
		}
	}

	// 0度（水平線）は太く強調表示
	float horizonOffsetY = -pitchDeg * 0.3f;
	if (std::abs(horizonOffsetY) <= 15.0f) {
		Vector3 left = GetPlayerFrontPositionWithOffset(-8.0f, horizonOffsetY, boresightOffset_);
		Vector3 center = GetPlayerFrontPositionWithOffset(0.0f, horizonOffsetY, boresightOffset_);
		Vector3 right = GetPlayerFrontPositionWithOffset(8.0f, horizonOffsetY, boresightOffset_);

		lineManager->DrawLine(left, center, hudColor_, 3.0f);
		lineManager->DrawLine(center, right, hudColor_, 3.0f);
	}
}

///=============================================================================
///                        速度テープ（左側）
void HUD::DrawSpeedTape() {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeX = -13.0f;
	float tapeY = 0.0f;

	// 速度テープの枠
	Vector3 topLeft = GetHUDPosition(tapeX - 1.5f, tapeY + 6.0f);
	Vector3 topRight = GetHUDPosition(tapeX + 1.5f, tapeY + 6.0f);
	Vector3 bottomLeft = GetHUDPosition(tapeX - 1.5f, tapeY - 6.0f);
	Vector3 bottomRight = GetHUDPosition(tapeX + 1.5f, tapeY - 6.0f);

	lineManager->DrawLine(topLeft, topRight, hudColor_);
	lineManager->DrawLine(topLeft, bottomLeft, hudColor_);
	lineManager->DrawLine(bottomLeft, bottomRight, hudColor_);
	lineManager->DrawLine(topRight, bottomRight, hudColor_);

	// 現在の速度を中央に表示するためのマーカー
	Vector3 speedMarkerLeft = GetHUDPosition(tapeX - 2.0f, tapeY);
	Vector3 speedMarkerRight = GetHUDPosition(tapeX + 2.0f, tapeY);
	Vector3 speedMarkerTop = GetHUDPosition(tapeX, tapeY + 0.8f);
	Vector3 speedMarkerBottom = GetHUDPosition(tapeX, tapeY - 0.8f);

	lineManager->DrawLine(speedMarkerLeft, speedMarkerTop, hudColor_, 2.0f);
	lineManager->DrawLine(speedMarkerTop, speedMarkerRight, hudColor_, 2.0f);
	lineManager->DrawLine(speedMarkerRight, speedMarkerBottom, hudColor_, 2.0f);
	lineManager->DrawLine(speedMarkerBottom, speedMarkerLeft, hudColor_, 2.0f);

	// 速度目盛り（10単位ごと）
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

///=============================================================================
///                        高度テープ（右側）
void HUD::DrawAltitudeTape() {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeX = 13.0f;
	float tapeY = 0.0f;

	// 高度テープの枠
	Vector3 topLeft = GetHUDPosition(tapeX - 1.5f, tapeY + 6.0f);
	Vector3 topRight = GetHUDPosition(tapeX + 1.5f, tapeY + 6.0f);
	Vector3 bottomLeft = GetHUDPosition(tapeX - 1.5f, tapeY - 6.0f);
	Vector3 bottomRight = GetHUDPosition(tapeX + 1.5f, tapeY - 6.0f);

	lineManager->DrawLine(topLeft, topRight, hudColor_);
	lineManager->DrawLine(topLeft, bottomLeft, hudColor_);
	lineManager->DrawLine(bottomLeft, bottomRight, hudColor_);
	lineManager->DrawLine(topRight, bottomRight, hudColor_);

	// 現在の高度マーカー
	Vector3 altMarkerLeft = GetHUDPosition(tapeX - 2.0f, tapeY);
	Vector3 altMarkerRight = GetHUDPosition(tapeX + 2.0f, tapeY);
	Vector3 altMarkerTop = GetHUDPosition(tapeX, tapeY + 0.8f);
	Vector3 altMarkerBottom = GetHUDPosition(tapeX, tapeY - 0.8f);

	Vector4 altColor = (currentAltitude_ < 20.0f) ? hudColorCritical_ : hudColor_;

	lineManager->DrawLine(altMarkerLeft, altMarkerTop, altColor, 2.0f);
	lineManager->DrawLine(altMarkerTop, altMarkerRight, altColor, 2.0f);
	lineManager->DrawLine(altMarkerRight, altMarkerBottom, altColor, 2.0f);
	lineManager->DrawLine(altMarkerBottom, altMarkerLeft, altColor, 2.0f);

	// 高度目盛り（10単位ごと）
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

///=============================================================================
///                        方位テープ（上部）
void HUD::DrawHeadingTape() {
	LineManager *lineManager = LineManager::GetInstance();

	float tapeY = 8.0f;

	// 方位テープの枠
	Vector3 left = GetHUDPosition(-6.0f, tapeY);
	Vector3 right = GetHUDPosition(6.0f, tapeY);
	lineManager->DrawLine(left, right, hudColor_);

	// 中央マーカー
	Vector3 centerTop = GetHUDPosition(0.0f, tapeY + 1.0f);
	Vector3 centerBottom = GetHUDPosition(0.0f, tapeY);
	Vector3 centerLeft = GetHUDPosition(-0.5f, tapeY + 0.5f);
	Vector3 centerRight = GetHUDPosition(0.5f, tapeY + 0.5f);

	lineManager->DrawLine(centerTop, centerBottom, hudColor_, 2.0f);
	lineManager->DrawLine(centerTop, centerLeft, hudColor_, 2.0f);
	lineManager->DrawLine(centerTop, centerRight, hudColor_, 2.0f);

	// 方位目盛り（30度ごと）
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

///=============================================================================
///                        G-Force表示（左下）
void HUD::DrawGForceIndicator() {
	LineManager *lineManager = LineManager::GetInstance();

	float posX = -13.0f;
	float posY = -8.0f;

	// G-Forceバーの枠
	Vector3 barLeft = GetHUDPosition(posX, posY);
	Vector3 barRight = GetHUDPosition(posX + 4.0f, posY);
	lineManager->DrawLine(barLeft, barRight, hudColor_);

	// G-Force値に応じて色を変える
	Vector4 gColor = hudColor_;
	if (currentGForce_ > 7.0f) {
		gColor = hudColorCritical_;
	} else if (currentGForce_ > 5.0f) {
		gColor = hudColorWarning_;
	}

	// G-Forceバー
	float gBarLength = std::min(std::abs(currentGForce_ - 1.0f) / 8.0f * 4.0f, 4.0f);
	Vector3 gBarEnd = GetHUDPosition(posX + gBarLength, posY);
	lineManager->DrawLine(barLeft, gBarEnd, gColor, 3.0f);
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

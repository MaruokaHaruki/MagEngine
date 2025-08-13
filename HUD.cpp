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
	screenCenter_ = {0.0f, 0.0f, 0.0f};
	hudScale_ = 1.0f;
	hudColor_ = {0.0f, 1.0f, 0.0f, 1.0f}; // 緑色のHUD
	hudDistance_ = 15.0f;				  // カメラから15単位前方にHUDを配置
	hudSize_ = 1.0f;					  // HUD全体のサイズ倍率

	// 表示設定の初期化
	showBoresight_ = true;
	showPitchScale_ = true;
	showRollScale_ = true;
	showSpeedIndicator_ = true;
	showAltitudeIndicator_ = true;
	showCompass_ = true;
	showGForce_ = true;

	// データの初期化
	playerPosition_ = {0.0f, 0.0f, 0.0f};
	playerRotation_ = {0.0f, 0.0f, 0.0f};
	playerVelocity_ = {0.0f, 0.0f, 0.0f};
	currentGForce_ = 1.0f;
	currentSpeed_ = 0.0f;
	currentAltitude_ = 0.0f;
}

///=============================================================================
///                        スクリーン座標変換
Vector3 HUD::GetHUDPosition(float screenX, float screenY) {
	// 現在のカメラを取得
	Camera *currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
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

	// スクリーン座標をワールド座標に変換（HUD全体のサイズを適用）
	float scaledX = screenX * hudSize_;
	float scaledY = screenY * hudSize_;

	Vector3 worldPos = {
		hudCenter.x + right.x * scaledX + up.x * scaledY,
		hudCenter.y + right.y * scaledX + up.y * scaledY,
		hudCenter.z + right.z * scaledX + up.z * scaledY};

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
}

///=============================================================================
///                        描画
void HUD::Draw() {
	// カメラが存在しない場合は描画しない
	Camera *currentCamera = CameraManager::GetInstance()->GetCurrentCamera();
	if (!currentCamera) {
		return;
	}

	// HUDの中心位置を更新
	screenCenter_ = GetHUDPosition(0.0f, 0.0f);

	// HUDフレームの描画
	DrawHUDFrame();

	// 各HUD要素の描画
	if (showBoresight_) {
		DrawBoresight();
	}

	if (showPitchScale_) {
		float pitchDeg = RadiansToDegrees(playerRotation_.x);
		DrawPitchScale(pitchDeg);
	}

	if (showRollScale_) {
		float rollDeg = RadiansToDegrees(playerRotation_.z);
		DrawRollScale(rollDeg);
	}

	if (showGForce_) {
		DrawGForceIndicator(currentGForce_);
	}

	if (showSpeedIndicator_) {
		DrawSpeedIndicator(currentSpeed_);
		DrawMachIndicator(currentSpeed_ / 343.0f); // 音速343m/s
	}

	if (showCompass_) {
		float headingDeg = RadiansToDegrees(playerRotation_.y);
		DrawCompass(headingDeg);
	}

	if (showAltitudeIndicator_) {
		DrawAltitudeIndicator(currentAltitude_);
		DrawRadarAltitude(currentAltitude_); // 簡易版
	}

	// フライトパスマーカー
	DrawFlightPathMarker(playerVelocity_);
}

///=============================================================================
///                        ガンボアサイト
void HUD::DrawBoresight() {
	LineManager *lineManager = LineManager::GetInstance();

	// 十字線（ボアサイト）
	float size = 2.0f * hudScale_;

	// スクリーン座標系での位置を計算
	Vector3 leftPos = GetHUDPosition(-size, 0.0f);
	Vector3 rightPos = GetHUDPosition(size, 0.0f);
	Vector3 topPos = GetHUDPosition(0.0f, size);
	Vector3 bottomPos = GetHUDPosition(0.0f, -size);

	// 水平線
	lineManager->DrawLine(leftPos, rightPos, hudColor_);

	// 垂直線
	lineManager->DrawLine(topPos, bottomPos, hudColor_);

	// 中央の小さな円
	lineManager->DrawCircle(screenCenter_, 0.5f * hudScale_ * hudSize_, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);
}

///=============================================================================
///                        ピッチスケール
void HUD::DrawPitchScale(float pitchAngle) {
	LineManager *lineManager = LineManager::GetInstance();

	// ピッチスケールの描画（-30度から+30度まで10度間隔）
	for (int angle = -30; angle <= 30; angle += 10) {
		if (angle == 0)
			continue; // 水平線は別途描画

		float yOffset = (angle / 10.0f) * 2.0f * hudScale_;
		float lineLength = (angle % 20 == 0) ? 4.0f : 2.0f; // 20度間隔で長い線

		Vector3 leftPoint = GetHUDPosition(-lineLength, yOffset);
		Vector3 rightPoint = GetHUDPosition(lineLength, yOffset);

		// 正の角度は実線、負の角度は破線風に
		if (angle > 0) {
			lineManager->DrawLine(leftPoint, rightPoint, hudColor_);
		} else {
			// 破線風の描画
			for (int i = 0; i < 4; i++) {
				float segmentStart = -lineLength + (i * lineLength / 2.0f);
				float segmentEnd = segmentStart + lineLength / 4.0f;

				Vector3 segStartPos = GetHUDPosition(segmentStart, yOffset);
				Vector3 segEndPos = GetHUDPosition(segmentEnd, yOffset);

				lineManager->DrawLine(segStartPos, segEndPos, hudColor_);
			}
		}
	}

	// 水平線（0度線）を強調表示
	Vector3 horizonLeft = GetHUDPosition(-6.0f, 0.0f);
	Vector3 horizonRight = GetHUDPosition(6.0f, 0.0f);
	lineManager->DrawLine(horizonLeft, horizonRight, hudColor_);
}

///=============================================================================
///                        ロールスケール
void HUD::DrawRollScale(float rollAngle) {
	LineManager *lineManager = LineManager::GetInstance();

	// ロールスケールの円弧
	float radius = 8.0f * hudScale_;
	Vector3 arcCenter = GetHUDPosition(0.0f, radius);

	// スケール目盛り（-60度から+60度まで30度間隔）
	for (int angle = -60; angle <= 60; angle += 30) {
		float radians = DegreesToRadians(angle);
		float tickLength = (angle == 0) ? 1.5f : 1.0f;

		float outerX = sinf(radians) * radius;
		float outerY = radius - cosf(radians) * radius;
		float innerX = sinf(radians) * (radius - tickLength);
		float innerY = radius - cosf(radians) * (radius - tickLength);

		Vector3 outerPoint = GetHUDPosition(outerX, outerY);
		Vector3 innerPoint = GetHUDPosition(innerX, innerY);

		lineManager->DrawLine(outerPoint, innerPoint, hudColor_);
	}

	// 現在のロール角指示器
	float rollRad = DegreesToRadians(rollAngle);
	float indicatorX = sinf(rollRad) * (radius - 0.5f);
	float indicatorY = radius - cosf(rollRad) * (radius - 0.5f);

	Vector3 rollIndicator = GetHUDPosition(indicatorX, indicatorY);
	Vector3 tri1 = GetHUDPosition(indicatorX - 0.5f, indicatorY - 1.0f);
	Vector3 tri2 = GetHUDPosition(indicatorX + 0.5f, indicatorY - 1.0f);

	// 三角形の指示器
	lineManager->DrawLine(rollIndicator, tri1, hudColor_);
	lineManager->DrawLine(rollIndicator, tri2, hudColor_);
	lineManager->DrawLine(tri1, tri2, hudColor_);
}

///=============================================================================
///                        Gフォース計
void HUD::DrawGForceIndicator(float gForce) {
	LineManager *lineManager = LineManager::GetInstance();

	// 左上にG力表示
	Vector3 gPosition = GetHUDPosition(-10.0f, 8.0f);

	// Gメーターの枠（線で表現）
	float frameWidth = 3.0f;
	float frameHeight = 1.0f;

	Vector3 frameTopLeft = GetHUDPosition(-10.0f - frameWidth / 2, 8.0f + frameHeight / 2);
	Vector3 frameTopRight = GetHUDPosition(-10.0f + frameWidth / 2, 8.0f + frameHeight / 2);
	Vector3 frameBottomLeft = GetHUDPosition(-10.0f - frameWidth / 2, 8.0f - frameHeight / 2);
	Vector3 frameBottomRight = GetHUDPosition(-10.0f + frameWidth / 2, 8.0f - frameHeight / 2);

	lineManager->DrawLine(frameTopLeft, frameTopRight, hudColor_);
	lineManager->DrawLine(frameTopRight, frameBottomRight, hudColor_);
	lineManager->DrawLine(frameBottomRight, frameBottomLeft, hudColor_);
	lineManager->DrawLine(frameBottomLeft, frameTopLeft, hudColor_);

	// G力の値を表すバー
	float gRatio = std::min(std::max((gForce - 1.0f) / 8.0f, -1.0f), 1.0f); // -9Gから+9Gの範囲
	Vector3 barEnd = GetHUDPosition(-10.0f + gRatio * 1.5f, 8.0f);

	Vector4 gColor = hudColor_;
	if (gForce > 7.0f)
		gColor = {1.0f, 0.0f, 0.0f, 1.0f}; // 危険域は赤
	else if (gForce < -3.0f)
		gColor = {1.0f, 0.0f, 0.0f, 1.0f};

	lineManager->DrawLine(gPosition, barEnd, gColor);
}

///=============================================================================
///                        速度計
void HUD::DrawSpeedIndicator(float speed) {
	LineManager *lineManager = LineManager::GetInstance();

	// 左側に速度表示（枠を線で描画）
	float frameWidth = 2.0f;
	float frameHeight = 8.0f;

	Vector3 frameTopLeft = GetHUDPosition(-12.0f - frameWidth / 2, frameHeight / 2);
	Vector3 frameTopRight = GetHUDPosition(-12.0f + frameWidth / 2, frameHeight / 2);
	Vector3 frameBottomLeft = GetHUDPosition(-12.0f - frameWidth / 2, -frameHeight / 2);
	Vector3 frameBottomRight = GetHUDPosition(-12.0f + frameWidth / 2, -frameHeight / 2);

	lineManager->DrawLine(frameTopLeft, frameTopRight, hudColor_);
	lineManager->DrawLine(frameTopRight, frameBottomRight, hudColor_);
	lineManager->DrawLine(frameBottomRight, frameBottomLeft, hudColor_);
	lineManager->DrawLine(frameBottomLeft, frameTopLeft, hudColor_);

	// 速度目盛り（0-200の範囲で40間隔）
	for (int spd = 0; spd <= 200; spd += 40) {
		float yOffset = (spd / 200.0f) * 8.0f - 4.0f;
		Vector3 tickLeft = GetHUDPosition(-13.0f, yOffset);
		Vector3 tickRight = GetHUDPosition(-11.0f, yOffset);
		lineManager->DrawLine(tickLeft, tickRight, hudColor_);
	}

	// 現在速度の指示器
	float speedRatio = std::min(speed / 200.0f, 1.0f);
	float currentSpeedY = (speedRatio * 8.0f - 4.0f);

	// 三角形指示器
	Vector3 triBase = GetHUDPosition(-11.0f, currentSpeedY);
	Vector3 triTop = GetHUDPosition(-10.0f, currentSpeedY - 0.3f);
	Vector3 triBottom = GetHUDPosition(-10.0f, currentSpeedY + 0.3f);

	lineManager->DrawLine(triBase, triTop, hudColor_);
	lineManager->DrawLine(triBase, triBottom, hudColor_);
	lineManager->DrawLine(triTop, triBottom, hudColor_);
}

///=============================================================================
///                        マッハ計
void HUD::DrawMachIndicator(float mach) {
	LineManager *lineManager = LineManager::GetInstance();

	// 速度計の下にマッハ表示
	Vector3 machStart = GetHUDPosition(-12.0f, -6.0f);

	// マッハ数の簡易表示（横線の長さで表現）
	float machLength = std::min(mach * 2.0f, 4.0f);
	Vector3 machEnd = GetHUDPosition(-12.0f + machLength, -6.0f);

	lineManager->DrawLine(machStart, machEnd, hudColor_);

	// マッハ1.0のマーカー
	Vector3 mach1Top = GetHUDPosition(-10.0f, -5.8f);
	Vector3 mach1Bottom = GetHUDPosition(-10.0f, -6.2f);
	lineManager->DrawLine(mach1Top, mach1Bottom, hudColor_);
}

///=============================================================================
///                        コンパス
void HUD::DrawCompass(float heading) {
	LineManager *lineManager = LineManager::GetInstance();

	// 上部にコンパス表示
	Vector3 compassCenter = GetHUDPosition(0.0f, 10.0f);

	// コンパスの円
	lineManager->DrawCircle(compassCenter, 3.0f * hudSize_, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 24);

	// 方位目盛り（N, E, S, W）
	for (int dir = 0; dir < 4; dir++) {
		float angle = dir * PI / 2.0f;

		// コンパス座標系での計算
		float compassX = sinf(angle) * 3.0f;
		float compassY = cosf(angle) * 3.0f;

		Vector3 tickEnd = GetHUDPosition(compassX, 10.0f + compassY);
		Vector3 tickStart = GetHUDPosition(compassX * 0.83f, 10.0f + compassY * 0.83f);

		lineManager->DrawLine(tickStart, tickEnd, hudColor_);
	}

	// 現在の方位指示器
	float headingRad = DegreesToRadians(heading);
	float indicatorX = sinf(headingRad) * 2.0f;
	float indicatorY = cosf(headingRad) * 2.0f;

	Vector3 headingIndicator = GetHUDPosition(indicatorX, 10.0f + indicatorY);

	lineManager->DrawLine(compassCenter, headingIndicator, hudColor_);
}

///=============================================================================
///                        高度計
void HUD::DrawAltitudeIndicator(float altitude) {
	LineManager *lineManager = LineManager::GetInstance();

	// 右側に高度表示（枠を線で描画）
	float frameWidth = 2.0f;
	float frameHeight = 8.0f;

	Vector3 frameTopLeft = GetHUDPosition(12.0f - frameWidth / 2, frameHeight / 2);
	Vector3 frameTopRight = GetHUDPosition(12.0f + frameWidth / 2, frameHeight / 2);
	Vector3 frameBottomLeft = GetHUDPosition(12.0f - frameWidth / 2, -frameHeight / 2);
	Vector3 frameBottomRight = GetHUDPosition(12.0f + frameWidth / 2, -frameHeight / 2);

	lineManager->DrawLine(frameTopLeft, frameTopRight, hudColor_);
	lineManager->DrawLine(frameTopRight, frameBottomRight, hudColor_);
	lineManager->DrawLine(frameBottomRight, frameBottomLeft, hudColor_);
	lineManager->DrawLine(frameBottomLeft, frameTopLeft, hudColor_);

	// 高度目盛り（0-1000の範囲で200間隔）
	for (int alt = 0; alt <= 1000; alt += 200) {
		float yOffset = (alt / 1000.0f) * 8.0f - 4.0f;
		Vector3 tickLeft = GetHUDPosition(11.0f, yOffset);
		Vector3 tickRight = GetHUDPosition(13.0f, yOffset);
		lineManager->DrawLine(tickLeft, tickRight, hudColor_);
	}

	// 現在高度の指示器
	float altRatio = std::min(std::max(altitude / 1000.0f, 0.0f), 1.0f);
	float currentAltY = (altRatio * 8.0f - 4.0f);

	// 三角形指示器
	Vector3 triBase = GetHUDPosition(11.0f, currentAltY);
	Vector3 triTop = GetHUDPosition(10.0f, currentAltY - 0.3f);
	Vector3 triBottom = GetHUDPosition(10.0f, currentAltY + 0.3f);

	lineManager->DrawLine(triBase, triTop, hudColor_);
	lineManager->DrawLine(triBase, triBottom, hudColor_);
	lineManager->DrawLine(triTop, triBottom, hudColor_);
}

///=============================================================================
///                        レーダー高度計
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
///                        フライトパスマーカー
void HUD::DrawFlightPathMarker(const Vector3 &velocity) {
	LineManager *lineManager = LineManager::GetInstance();

	// 速度ベクトルに基づいてフライトパスマーカーの位置を計算
	float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
	if (speed < 0.1f)
		return; // 静止時は表示しない

	// 速度方向に基づくオフセット（簡易版）
	Vector3 markerPos = GetHUDPosition(velocity.x * 0.1f, velocity.y * 0.1f);

	// フライトパスマーカー（円と十字）
	lineManager->DrawCircle(markerPos, 1.0f * hudSize_, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);

	// 中央の十字
	Vector3 crossLeft = GetHUDPosition(velocity.x * 0.1f - 0.5f, velocity.y * 0.1f);
	Vector3 crossRight = GetHUDPosition(velocity.x * 0.1f + 0.5f, velocity.y * 0.1f);
	Vector3 crossTop = GetHUDPosition(velocity.x * 0.1f, velocity.y * 0.1f + 0.5f);
	Vector3 crossBottom = GetHUDPosition(velocity.x * 0.1f, velocity.y * 0.1f - 0.5f);

	lineManager->DrawLine(crossLeft, crossRight, hudColor_);
	lineManager->DrawLine(crossTop, crossBottom, hudColor_);
}

///=============================================================================
///                        HUDフレーム
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
///                        ImGui描画
void HUD::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("HUD Settings");

	ImGui::Text("HUD Display Control");
	ImGui::Checkbox("Show Boresight", &showBoresight_);
	ImGui::Checkbox("Show Pitch Scale", &showPitchScale_);
	ImGui::Checkbox("Show Roll Scale", &showRollScale_);
	ImGui::Checkbox("Show Speed Indicator", &showSpeedIndicator_);
	ImGui::Checkbox("Show Altitude Indicator", &showAltitudeIndicator_);
	ImGui::Checkbox("Show Compass", &showCompass_);
	ImGui::Checkbox("Show G-Force", &showGForce_);

	ImGui::Separator();
	ImGui::SliderFloat("HUD Scale", &hudScale_, 0.5f, 2.0f);
	ImGui::SliderFloat("HUD Distance", &hudDistance_, 5.0f, 50.0f);
	ImGui::SliderFloat("HUD Size", &hudSize_, 0.1f, 3.0f);
	ImGui::ColorEdit4("HUD Color", &hudColor_.x);

	ImGui::Separator();
	ImGui::Text("Current Values:");
	ImGui::Text("Speed: %.1f m/s", currentSpeed_);
	ImGui::Text("Altitude: %.1f m", currentAltitude_);
	ImGui::Text("G-Force: %.2f G", currentGForce_);

	// デバッグ情報追加
	ImGui::Separator();
	ImGui::Text("Debug Info:");
	Camera *cam = CameraManager::GetInstance()->GetCurrentCamera();
	if (cam) {
		Vector3 camPos = cam->GetTransform().translate;
		Vector3 camRot = cam->GetTransform().rotate;
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

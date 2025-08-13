#define _USE_MATH_DEFINES
#define NOMINMAX
#include <algorithm>
#include "HUD.h"
#include "ImguiSetup.h"
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

	// 水平線
	lineManager->DrawLine(
		{screenCenter_.x - size, screenCenter_.y, screenCenter_.z},
		{screenCenter_.x + size, screenCenter_.y, screenCenter_.z},
		hudColor_);

	// 垂直線
	lineManager->DrawLine(
		{screenCenter_.x, screenCenter_.y - size, screenCenter_.z},
		{screenCenter_.x, screenCenter_.y + size, screenCenter_.z},
		hudColor_);

	// 中央の小さな円
	lineManager->DrawCircle(screenCenter_, 0.5f * hudScale_, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);
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

		Vector3 leftPoint = {screenCenter_.x - lineLength, screenCenter_.y + yOffset, screenCenter_.z};
		Vector3 rightPoint = {screenCenter_.x + lineLength, screenCenter_.y + yOffset, screenCenter_.z};

		// 正の角度は実線、負の角度は破線風に
		if (angle > 0) {
			lineManager->DrawLine(leftPoint, rightPoint, hudColor_);
		} else {
			// 破線風の描画
			for (int i = 0; i < 4; i++) {
				float segmentStart = -lineLength + (i * lineLength / 2.0f);
				float segmentEnd = segmentStart + lineLength / 4.0f;

				lineManager->DrawLine(
					{screenCenter_.x + segmentStart, screenCenter_.y + yOffset, screenCenter_.z},
					{screenCenter_.x + segmentEnd, screenCenter_.y + yOffset, screenCenter_.z},
					hudColor_);
			}
		}
	}

	// 水平線（0度線）を強調表示
	lineManager->DrawLine(
		{screenCenter_.x - 6.0f, screenCenter_.y, screenCenter_.z},
		{screenCenter_.x + 6.0f, screenCenter_.y, screenCenter_.z},
		hudColor_);
}

///=============================================================================
///                        ロールスケール
void HUD::DrawRollScale(float rollAngle) {
	LineManager *lineManager = LineManager::GetInstance();

	// ロールスケールの円弧
	float radius = 8.0f * hudScale_;
	Vector3 arcCenter = {screenCenter_.x, screenCenter_.y + radius, screenCenter_.z};

	// スケール目盛り（-60度から+60度まで30度間隔）
	for (int angle = -60; angle <= 60; angle += 30) {
		float radians = DegreesToRadians(angle);
		float tickLength = (angle == 0) ? 1.5f : 1.0f;

		Vector3 outerPoint = {
			arcCenter.x + sinf(radians) * radius,
			arcCenter.y - cosf(radians) * radius,
			screenCenter_.z};

		Vector3 innerPoint = {
			arcCenter.x + sinf(radians) * (radius - tickLength),
			arcCenter.y - cosf(radians) * (radius - tickLength),
			screenCenter_.z};

		lineManager->DrawLine(outerPoint, innerPoint, hudColor_);
	}

	// 現在のロール角指示器
	float rollRad = DegreesToRadians(rollAngle);
	Vector3 rollIndicator = {
		arcCenter.x + sinf(rollRad) * (radius - 0.5f),
		arcCenter.y - cosf(rollRad) * (radius - 0.5f),
		screenCenter_.z};

	// 三角形の指示器
	lineManager->DrawLine(rollIndicator,
						  {rollIndicator.x - 0.5f, rollIndicator.y + 1.0f, rollIndicator.z}, hudColor_);
	lineManager->DrawLine(rollIndicator,
						  {rollIndicator.x + 0.5f, rollIndicator.y + 1.0f, rollIndicator.z}, hudColor_);
	lineManager->DrawLine(
		{rollIndicator.x - 0.5f, rollIndicator.y + 1.0f, rollIndicator.z},
		{rollIndicator.x + 0.5f, rollIndicator.y + 1.0f, rollIndicator.z}, hudColor_);
}

///=============================================================================
///                        Gフォース計
void HUD::DrawGForceIndicator(float gForce) {
	LineManager *lineManager = LineManager::GetInstance();

	// 左上にG力表示
	Vector3 gPosition = {screenCenter_.x - 10.0f, screenCenter_.y + 8.0f, screenCenter_.z};

	// Gメーターの枠
	lineManager->DrawBox(gPosition, {3.0f, 1.0f, 0.1f}, hudColor_);

	// G力の値を表すバー
	float gRatio = std::min(std::max((gForce - 1.0f) / 8.0f, -1.0f), 1.0f); // -9Gから+9Gの範囲
	Vector3 barEnd = {gPosition.x + gRatio * 1.5f, gPosition.y, gPosition.z};

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

	// 左側に速度表示
	Vector3 speedPos = {screenCenter_.x - 12.0f, screenCenter_.y, screenCenter_.z};

	// 速度計の枠
	lineManager->DrawBox(speedPos, {2.0f, 8.0f, 0.1f}, hudColor_);

	// 速度目盛り（0-200の範囲で20間隔）
	for (int spd = 0; spd <= 200; spd += 40) {
		float yOffset = (spd / 200.0f) * 8.0f - 4.0f;
		lineManager->DrawLine(
			{speedPos.x - 1.0f, speedPos.y + yOffset, speedPos.z},
			{speedPos.x + 1.0f, speedPos.y + yOffset, speedPos.z},
			hudColor_);
	}

	// 現在速度の指示器
	float speedRatio = std::min(speed / 200.0f, 1.0f);
	float currentSpeedY = speedPos.y + (speedRatio * 8.0f - 4.0f);

	// 三角形指示器
	lineManager->DrawLine(
		{speedPos.x + 1.0f, currentSpeedY, speedPos.z},
		{speedPos.x + 2.0f, currentSpeedY - 0.3f, speedPos.z},
		hudColor_);
	lineManager->DrawLine(
		{speedPos.x + 1.0f, currentSpeedY, speedPos.z},
		{speedPos.x + 2.0f, currentSpeedY + 0.3f, speedPos.z},
		hudColor_);
	lineManager->DrawLine(
		{speedPos.x + 2.0f, currentSpeedY - 0.3f, speedPos.z},
		{speedPos.x + 2.0f, currentSpeedY + 0.3f, speedPos.z},
		hudColor_);
}

///=============================================================================
///                        マッハ計
void HUD::DrawMachIndicator(float mach) {
	LineManager *lineManager = LineManager::GetInstance();

	// 速度計の下にマッハ表示
	Vector3 machPos = {screenCenter_.x - 12.0f, screenCenter_.y - 6.0f, screenCenter_.z};

	// マッハ数の簡易表示（横線の長さで表現）
	float machLength = std::min(mach * 2.0f, 4.0f);
	lineManager->DrawLine(
		machPos,
		{machPos.x + machLength, machPos.y, machPos.z},
		hudColor_);

	// マッハ1.0のマーカー
	lineManager->DrawLine(
		{machPos.x + 2.0f, machPos.y - 0.2f, machPos.z},
		{machPos.x + 2.0f, machPos.y + 0.2f, machPos.z},
		hudColor_);
}

///=============================================================================
///                        コンパス
void HUD::DrawCompass(float heading) {
	LineManager *lineManager = LineManager::GetInstance();

	// 上部にコンパス表示
	Vector3 compassCenter = {screenCenter_.x, screenCenter_.y + 10.0f, screenCenter_.z};

	// コンパスの円
	lineManager->DrawCircle(compassCenter, 3.0f, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 24);

	// 方位目盛り（N, E, S, W）
	for (int dir = 0; dir < 4; dir++) {
		float angle = dir * PI / 2.0f;
		Vector3 tickEnd = {
			compassCenter.x + sinf(angle) * 3.0f,
			compassCenter.y + cosf(angle) * 3.0f,
			compassCenter.z};
		Vector3 tickStart = {
			compassCenter.x + sinf(angle) * 2.5f,
			compassCenter.y + cosf(angle) * 2.5f,
			compassCenter.z};

		lineManager->DrawLine(tickStart, tickEnd, hudColor_);
	}

	// 現在の方位指示器
	float headingRad = DegreesToRadians(heading);
	Vector3 headingIndicator = {
		compassCenter.x + sinf(headingRad) * 2.0f,
		compassCenter.y + cosf(headingRad) * 2.0f,
		compassCenter.z};

	lineManager->DrawLine(compassCenter, headingIndicator, hudColor_);
}

///=============================================================================
///                        高度計
void HUD::DrawAltitudeIndicator(float altitude) {
	LineManager *lineManager = LineManager::GetInstance();

	// 右側に高度表示
	Vector3 altPos = {screenCenter_.x + 12.0f, screenCenter_.y, screenCenter_.z};

	// 高度計の枠
	lineManager->DrawBox(altPos, {2.0f, 8.0f, 0.1f}, hudColor_);

	// 高度目盛り（0-1000の範囲で200間隔）
	for (int alt = 0; alt <= 1000; alt += 200) {
		float yOffset = (alt / 1000.0f) * 8.0f - 4.0f;
		lineManager->DrawLine(
			{altPos.x - 1.0f, altPos.y + yOffset, altPos.z},
			{altPos.x + 1.0f, altPos.y + yOffset, altPos.z},
			hudColor_);
	}

	// 現在高度の指示器
	float altRatio = std::min(std::max(altitude / 1000.0f, 0.0f), 1.0f);
	float currentAltY = altPos.y + (altRatio * 8.0f - 4.0f);

	// 三角形指示器
	lineManager->DrawLine(
		{altPos.x - 1.0f, currentAltY, altPos.z},
		{altPos.x - 2.0f, currentAltY - 0.3f, altPos.z},
		hudColor_);
	lineManager->DrawLine(
		{altPos.x - 1.0f, currentAltY, altPos.z},
		{altPos.x - 2.0f, currentAltY + 0.3f, altPos.z},
		hudColor_);
	lineManager->DrawLine(
		{altPos.x - 2.0f, currentAltY - 0.3f, altPos.z},
		{altPos.x - 2.0f, currentAltY + 0.3f, altPos.z},
		hudColor_);
}

///=============================================================================
///                        レーダー高度計
void HUD::DrawRadarAltitude(float radarAlt) {
	LineManager *lineManager = LineManager::GetInstance();

	// 高度計の下にレーダー高度表示
	Vector3 radarPos = {screenCenter_.x + 12.0f, screenCenter_.y - 6.0f, screenCenter_.z};

	// レーダー高度の簡易表示
	float radarLength = std::min(radarAlt / 100.0f * 2.0f, 4.0f);
	lineManager->DrawLine(
		radarPos,
		{radarPos.x - radarLength, radarPos.y, radarPos.z},
		hudColor_);

	// 危険高度マーカー（50m）
	lineManager->DrawLine(
		{radarPos.x - 1.0f, radarPos.y - 0.2f, radarPos.z},
		{radarPos.x - 1.0f, radarPos.y + 0.2f, radarPos.z},
		{1.0f, 0.0f, 0.0f, 1.0f} // 赤色
	);
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
	Vector3 markerPos = {
		screenCenter_.x + velocity.x * 0.1f,
		screenCenter_.y + velocity.y * 0.1f,
		screenCenter_.z};

	// フライトパスマーカー（円と十字）
	lineManager->DrawCircle(markerPos, 1.0f, hudColor_, 1.0f, {0.0f, 0.0f, 1.0f}, 12);

	// 中央の十字
	lineManager->DrawLine(
		{markerPos.x - 0.5f, markerPos.y, markerPos.z},
		{markerPos.x + 0.5f, markerPos.y, markerPos.z},
		hudColor_);
	lineManager->DrawLine(
		{markerPos.x, markerPos.y - 0.5f, markerPos.z},
		{markerPos.x, markerPos.y + 0.5f, markerPos.z},
		hudColor_);
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
	lineManager->DrawLine(
		{screenCenter_.x - frameSize, screenCenter_.y + frameSize, screenCenter_.z},
		{screenCenter_.x - frameSize + cornerSize, screenCenter_.y + frameSize, screenCenter_.z},
		hudColor_);
	lineManager->DrawLine(
		{screenCenter_.x - frameSize, screenCenter_.y + frameSize, screenCenter_.z},
		{screenCenter_.x - frameSize, screenCenter_.y + frameSize - cornerSize, screenCenter_.z},
		hudColor_);

	// 右上
	lineManager->DrawLine(
		{screenCenter_.x + frameSize, screenCenter_.y + frameSize, screenCenter_.z},
		{screenCenter_.x + frameSize - cornerSize, screenCenter_.y + frameSize, screenCenter_.z},
		hudColor_);
	lineManager->DrawLine(
		{screenCenter_.x + frameSize, screenCenter_.y + frameSize, screenCenter_.z},
		{screenCenter_.x + frameSize, screenCenter_.y + frameSize - cornerSize, screenCenter_.z},
		hudColor_);

	// 左下
	lineManager->DrawLine(
		{screenCenter_.x - frameSize, screenCenter_.y - frameSize, screenCenter_.z},
		{screenCenter_.x - frameSize + cornerSize, screenCenter_.y - frameSize, screenCenter_.z},
		hudColor_);
	lineManager->DrawLine(
		{screenCenter_.x - frameSize, screenCenter_.y - frameSize, screenCenter_.z},
		{screenCenter_.x - frameSize, screenCenter_.y - frameSize + cornerSize, screenCenter_.z},
		hudColor_);

	// 右下
	lineManager->DrawLine(
		{screenCenter_.x + frameSize, screenCenter_.y - frameSize, screenCenter_.z},
		{screenCenter_.x + frameSize - cornerSize, screenCenter_.y - frameSize, screenCenter_.z},
		hudColor_);
	lineManager->DrawLine(
		{screenCenter_.x + frameSize, screenCenter_.y - frameSize, screenCenter_.z},
		{screenCenter_.x + frameSize, screenCenter_.y - frameSize + cornerSize, screenCenter_.z},
		hudColor_);
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
	ImGui::ColorEdit4("HUD Color", &hudColor_.x);

	ImGui::Separator();
	ImGui::Text("Current Values:");
	ImGui::Text("Speed: %.1f m/s", currentSpeed_);
	ImGui::Text("Altitude: %.1f m", currentAltitude_);
	ImGui::Text("G-Force: %.2f G", currentGForce_);

	ImGui::End();
#endif // _DEBUG
}

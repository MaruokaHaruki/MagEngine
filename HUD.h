#pragma once
#include "Camera.h"
#include "CameraManager.h"
#include "FollowCamera.h"
#include "LineManager.h"
#include "Player.h"
#include "Vector3.h"
#include "Vector4.h"
#include <memory>

class HUD {
public:
	void Initialize();
	void Update(const Player *player);
	void Draw();
	void DrawImGui();

	// FollowCameraの設定
	void SetFollowCamera(FollowCamera *followCamera);

private:
	// HUDの各要素を描画する関数
	void DrawBoresight();
	void DrawPitchScale(float pitchAngle);
	void DrawRollScale(float rollAngle);
	void DrawGForceIndicator(float gForce);
	void DrawSpeedIndicator(float speed);
	void DrawMachIndicator(float mach);
	void DrawCompass(float heading);
	void DrawAltitudeIndicator(float altitude);
	void DrawRadarAltitude(float radarAlt);
	void DrawFlightPathMarker(const Vector3 &velocity);
	void DrawHUDFrame();

	// スクリーン座標変換
	Vector3 GetHUDPosition(float screenX, float screenY);

	// HUDの設定値
	Vector3 screenCenter_;
	float hudScale_;
	Vector4 hudColor_;
	float hudDistance_; // カメラからHUDまでの距離
	float hudSize_;		// HUD全体のサイズ

	// カメラ参照
	FollowCamera *followCamera_; // FollowCameraの参照

	// プレイヤーデータ
	Vector3 playerPosition_;
	Vector3 playerRotation_;
	Vector3 playerVelocity_;
	float currentGForce_;
	float currentSpeed_;
	float currentAltitude_;

	// HUD表示制御
	bool showBoresight_;
	bool showPitchScale_;
	bool showRollScale_;
	bool showSpeedIndicator_;
	bool showAltitudeIndicator_;
	bool showCompass_;
	bool showGForce_;
};

#pragma once
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

	// HUDの設定値
	Vector3 screenCenter_;
	float hudScale_;
	Vector4 hudColor_;

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

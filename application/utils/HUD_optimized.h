#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Camera.h"
#include "CameraManager.h"
#include "FollowCamera.h"
#include "LineManager.h"
#include "Player.h"
#include <memory>

class HUD {
public:
	void Initialize();
	void Update(const Player *player);
	void Draw();
	void DrawImGui();
	void SetFollowCamera(FollowCamera *followCamera);
	void StartDeployAnimation(float duration = 1.2f);
	void StartRetractAnimation(float duration = 1.0f);
	bool IsAnimating() const {
		return isAnimating_;
	}

private:
	// 描画関数
	void DrawBoresight(float progress = 1.0f);
	void DrawRollScale(float rollAngle, float progress = 1.0f);
	void DrawHUDFrame(float progress = 1.0f);
	void DrawVelocityVector(float progress = 1.0f);
	void DrawFlightPathMarker(float progress = 1.0f);
	void DrawPitchLadder(float progress = 1.0f);
	void DrawHeadingTape(float progress = 1.0f);
	void DrawGForceIndicator(float progress = 1.0f);
	void DrawBoostBarrel(float progress = 1.0f);

	// 座標変換
	Vector3 GetHUDPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset);
	Vector3 ClampHUDPosition(const Vector3 &worldPos, const Vector3 &cameraPos, const Vector3 &cameraForward);

	// アニメーション
	void UpdateAnimation();
	float GetDeployProgress() const;
	float EaseOutCubic(float t) const;

	// HUD設定
	float hudScale_, hudDistance_, hudSizeX_, hudSizeY_;
	Vector4 hudColor_, hudColorWarning_, hudColorCritical_, hudColorCyan_;
	Vector3 boresightOffset_, rollScaleOffset_;
	FollowCamera *followCamera_;

	// プレイヤーデータ
	Vector3 playerPosition_, playerRotation_, playerVelocity_, bulletFireDirection_;
	float currentGForce_, currentSpeed_, currentAltitude_, currentHeading_;
	float currentBoostGauge_, maxBoostGauge_, barrelRollProgress_;
	bool isBarrelRolling_;

	// 表示制御
	bool showBoresight_, showRollScale_, showCompass_, showGForce_;
	bool showVelocityVector_, showFlightPath_, showPitchLadder_;

	// アニメーション状態
	bool isAnimating_, isDeploying_;
	float animationTime_, animationDuration_, deployProgress_;
	float frameDeployStart_, boresightDeployStart_, pitchLadderDeployStart_;
	float velocityVectorDeployStart_, rollScaleDeployStart_, headingTapeDeployStart_;
	float gForceDeployStart_, boostBarrelDeployStart_;
};

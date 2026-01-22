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

	// FollowCameraの設定
	void SetFollowCamera(FollowCamera *followCamera);

	// アニメーション制御
	void StartDeployAnimation(float duration = 1.5f);
	void StartRetractAnimation(float duration = 1.0f);
	bool IsAnimating() const {
		return isAnimating_;
	}

private:
	// HUDの各要素を描画する関数
	void DrawBoresight(float progress = 1.0f);					   // 画面中央: 照準
	void DrawRollScale(float rollAngle, float progress = 1.0f);	   // 画面上部中央: ロール円弧
	void DrawRadarAltitude(float radarAlt, float progress = 1.0f); // 画面右下: レーダー高度
	void DrawHUDFrame(float progress = 1.0f);					   // 画面四隅: コーナーマーカー
	void DrawVelocityVector(float progress = 1.0f);				   // ベロシティベクトル
	void DrawFlightPathMarker(float progress = 1.0f);			   // フライトパスマーカー
	void DrawPitchLadder(float progress = 1.0f);				   // ピッチラダー
	void DrawHeadingTape(float progress = 1.0f);				   // 上部: 方位テープ
	void DrawGForceIndicator(float progress = 1.0f);			   // G-Force表示
	void DrawBoostBarrel(float progress = 1.0f);				   // 下部: ブースト＆回避統合UI

	// スクリーン座標変換
	Vector3 GetHUDPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset);
	Vector3 ClampHUDPosition(const Vector3 &worldPos, const Vector3 &cameraPos, const Vector3 &cameraForward);

	// HUDの設定値
	Vector3 screenCenter_;
	float hudScale_;
	Vector4 hudColor_;
	Vector4 hudColorWarning_;
	Vector4 hudColorCritical_;
	Vector4 hudColorCyan_;
	float hudDistance_;
	float hudSizeX_;
	float hudSizeY_;
	float viewportMargin_;

	// プレイヤー正面HUD要素の位置調整
	Vector3 boresightOffset_;
	Vector3 rollScaleOffset_;

	// カメラ参照
	FollowCamera *followCamera_;

	// プレイヤーデータ
	Vector3 playerPosition_;
	Vector3 playerRotation_;
	Vector3 playerVelocity_;
	Vector3 bulletFireDirection_; // 追加: 弾発射方向
	float currentGForce_;
	float currentSpeed_;
	float currentAltitude_;
	float currentHeading_;
	float currentBoostGauge_;
	float maxBoostGauge_;
	bool isBarrelRolling_;
	float barrelRollProgress_;

	// HUD表示制御
	bool showBoresight_;
	bool showRollScale_;
	bool showCompass_;
	bool showGForce_;
	bool showVelocityVector_;
	bool showFlightPath_;
	bool showPitchLadder_;

	// アニメーション状態
	bool isAnimating_;
	bool isDeploying_;
	float animationTime_;
	float animationDuration_;
	float deployProgress_;
	float frameDeployStart_;
	float boresightDeployStart_;
	float pitchLadderDeployStart_;
	float velocityVectorDeployStart_;
	float rollScaleDeployStart_;
	float headingTapeDeployStart_;
	float gForceDeployStart_;
	float boostBarrelDeployStart_;

	// アニメーション関連の内部処理
	void UpdateAnimation();
	float GetDeployProgress() const;
	float EaseOutCubic(float t) const;
};

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

	void DrawBoresight(); // 画面中央: 照準
	void DrawRollScale(float rollAngle); // 画面上部中央: ロール円弧（-60°～+60°）
	void DrawRadarAltitude(float radarAlt); // 画面右下: レーダー高度
	void DrawHUDFrame(); // 画面四隅: コーナーマーカー

	// スクリーン座標変換
	Vector3 GetHUDPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPosition(float screenX, float screenY);								   // ガンボアサイト専用
	Vector3 GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset); // オフセット付き

	// HUDの設定値
	Vector3 screenCenter_;
	float hudScale_;
	Vector4 hudColor_;
	float hudDistance_; // カメラからHUDまでの距離
	float hudSizeX_;	// HUD横幅サイズ
	float hudSizeY_;	// HUD縦幅サイズ

	// プレイヤー正面HUD要素の位置調整
	Vector3 boresightOffset_; // ガンボアサイトのオフセット
	Vector3 rollScaleOffset_; // ロールスケールのオフセット

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

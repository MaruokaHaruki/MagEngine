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

	// アニメーション制御
	void StartDeployAnimation(float duration = 1.5f);
	void StartRetractAnimation(float duration = 1.0f);
	bool IsAnimating() const {
		return isAnimating_;
	}

private:
	// HUDの各要素を描画する関数
	void DrawBoresight();					// 画面中央: 照準
	void DrawRollScale(float rollAngle);	// 画面上部中央: ロール円弧（-60°～+60°）
	void DrawRadarAltitude(float radarAlt); // 画面右下: レーダー高度
	void DrawHUDFrame();					// 画面四隅: コーナーマーカー
	void DrawVelocityVector();				// ベロシティベクトル（機体進行方向）
	void DrawFlightPathMarker();			// フライトパスマーカー（実際の移動方向）
	void DrawPitchLadder();					// ピッチラダー（水平線と角度表示）
	void DrawSpeedTape();					// 左側: 速度テープ
	void DrawAltitudeTape();				// 右側: 高度テープ
	void DrawHeadingTape();					// 上部: 方位テープ
	void DrawGForceIndicator();				// G-Force表示

	// スクリーン座標変換
	Vector3 GetHUDPosition(float screenX, float screenY);
	Vector3 GetPlayerFrontPosition(float screenX, float screenY);								   // ガンボアサイト専用
	Vector3 GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset); // オフセット付き

	// HUDの設定値
	Vector3 screenCenter_;
	float hudScale_;
	Vector4 hudColor_;
	Vector4 hudColorWarning_;  // 警告色
	Vector4 hudColorCritical_; // 危険色
	float hudDistance_;		   // カメラからHUDまでの距離
	float hudSizeX_;		   // HUD横幅サイズ
	float hudSizeY_;		   // HUD縦幅サイズ

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
	float currentHeading_; // 方位角（度）

	// HUD表示制御
	bool showBoresight_;
	bool showPitchScale_;
	bool showRollScale_;
	bool showSpeedIndicator_;
	bool showAltitudeIndicator_;
	bool showCompass_;
	bool showGForce_;
	bool showVelocityVector_;
	bool showFlightPath_;
	bool showPitchLadder_;

	// アニメーション状態
	bool isAnimating_;
	bool isDeploying_; // true=展開中, false=格納中
	float animationTime_;
	float animationDuration_;
	float deployProgress_;	 // 0.0f～1.0f の展開進行度
	float frameDeployStart_; // フレーム展開開始時刻
	float boresightDeployStart_;
	float pitchLadderDeployStart_;
	float velocityVectorDeployStart_;
	float rollScaleDeployStart_;
	float speedTapeDeployStart_;
	float altitudeTapeDeployStart_;
	float headingTapeDeployStart_;
	float gForceDeployStart_;

	// アニメーション関連の内部処理
	void UpdateAnimation();
	float GetDeployProgress() const;   // 0.0f～1.0f の展開進行度を返す
	float EaseOutCubic(float t) const; // イージング関数
};

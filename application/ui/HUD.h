#pragma once
#include "EnemyHandle.h"
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
using Vector4 = MagMath::Vector4;
#include "Camera.h"
#include "FollowCamera.h"
#include "Player.h"
#include <memory>

namespace MagEngine {
	class CameraManager;
	class LineManager;
	class RenderWorld;
	class TextRenderer;
}

class HUD {
public:
	/// @brief HUD描画に必要な外部サービスを設定する
	/// @param cameraManager HUD座標変換に使用するカメラ管理
	/// @param lineManager HUD線分を登録する描画管理
	/// @param textRenderer HUD文字列を登録するテキスト描画管理
	void Initialize(MagEngine::CameraManager &cameraManager, MagEngine::LineManager &lineManager, MagEngine::TextRenderer &textRenderer);
	/// @brief プレイヤー状態を取得しHUDの表示・展開アニメーションを更新する
	/// @param player 表示するプレイヤー状態。nullptrの場合は表示用状態を更新しない。
	/// @param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(const Player *player, float unscaledDeltaTime);
	/// @brief HUDの線分描画要素を登録する
	void Draw();
	/// @brief HUDが管理する戦闘状態をゲームUI文字として登録する
	void RegisterText(MagEngine::RenderWorld &renderWorld);
	/// @brief HUDのデバッグ設定を操作するImGuiを描画する
	void DrawImGui();

	/// @brief HUD座標計算に使用するFollowCameraを設定する
	/// @param followCamera 参照するFollowCamera
	void SetFollowCamera(FollowCamera *followCamera);

	/// @brief 現在HUDへ反映しているプレイヤーを取得する
	/// @return 現在のプレイヤー。未更新の場合はnullptr
	const Player *GetCurrentPlayer() const {
		return currentPlayer_;
	}
	/// @brief 現在HUD座標変換に使用しているカメラを取得する
	/// @return 現在のカメラ。未更新の場合はnullptr
	MagEngine::Camera *GetCurrentCamera() const {
		return currentCamera_;
	}

	/// @brief HUD要素を展開するアニメーションを開始する
	/// @param duration 展開時間（秒）
	void StartDeployAnimation(float duration = 1.5f);
	/// @brief HUD要素を収束するアニメーションを開始する
	/// @param duration 収束時間（秒）
	void StartRetractAnimation(float duration = 1.0f);
	/// @brief HUD展開または収束アニメーション中かを取得する
	/// @return アニメーション中の場合はtrue、それ以外はfalse
	bool IsAnimating() const {
		return isAnimating_;
	}

private:
	/// @brief 画面中央へ照準を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawBoresight(float progress = 1.0f);
	/// @brief 機体ロールに追従する目盛りを描画する
	/// @param rollAngle プレイヤーのロール角
	/// @param progress 展開アニメーションの進行度
	void DrawRollScale(float rollAngle, float progress = 1.0f);
	/// @brief レーダー高度を描画する
	/// @param radarAlt 表示する高度
	/// @param progress 展開アニメーションの進行度
	void DrawRadarAltitude(float radarAlt, float progress = 1.0f);
	/// @brief HUD外枠を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawHUDFrame(float progress = 1.0f);
	/// @brief 現在速度に対応するベロシティベクトルを描画する
	/// @param progress 展開アニメーションの進行度
	void DrawVelocityVector(float progress = 1.0f);
	/// @brief 現在の飛行経路マーカーを描画する
	/// @param progress 展開アニメーションの進行度
	void DrawFlightPathMarker(float progress = 1.0f);
	/// @brief ピッチ角の目盛りを描画する
	/// @param progress 展開アニメーションの進行度
	void DrawPitchLadder(float progress = 1.0f);
	/// @brief 現在方位のテープを描画する
	/// @param progress 展開アニメーションの進行度
	void DrawHeadingTape(float progress = 1.0f);
	/// @brief 現在Gを描画する
	/// @param progress 展開アニメーションの進行度
	void DrawGForceIndicator(float progress = 1.0f);
	/// @brief ブーストと回避状態を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawBoostBarrel(float progress = 1.0f);
	/// @brief HPと兵装残量を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawCombatStatus(float progress = 1.0f);
	/// @brief 現在のロックオン照準を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawLockOnReticle(float progress = 1.0f);
	/// @brief ロックオン候補の方向表示を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawEnemyIndicators(float progress = 1.0f);

	/// @brief 画面座標に対応するHUDワールド座標を求める
	/// @param screenX 画面X座標
	/// @param screenY 画面Y座標
	/// @return HUD要素のワールド座標
	Vector3 GetHUDPosition(float screenX, float screenY);
	/// @brief HUD座標をテキスト用2D座標へ変換する
	/// @param hudX HUD上のX座標
	/// @param hudY HUD上のY座標
	/// @return テキスト描画座標
	Vector2 GetTextPosition(float hudX, float hudY);
	/// @brief テキストをHUDの向きへ合わせる回転角を求める
	/// @return テキスト回転角（ラジアン）
	float GetTextRotationRadians();
	/// @brief プレイヤー正面に配置するHUD座標を求める
	/// @param screenX 画面X座標
	/// @param screenY 画面Y座標
	/// @return プレイヤー正面のHUD座標
	Vector3 GetPlayerFrontPosition(float screenX, float screenY);
	/// @brief オフセット付きでプレイヤー正面のHUD座標を求める
	/// @param screenX 画面X座標
	/// @param screenY 画面Y座標
	/// @param offset プレイヤー正面基準の加算オフセット
	/// @return オフセット適用後のHUD座標
	Vector3 GetPlayerFrontPositionWithOffset(float screenX, float screenY, const Vector3 &offset);
	/// @brief HUD座標が表示範囲外に出ないよう位置を補正する
	/// @param worldPos 補正対象のワールド座標
	/// @param cameraPos カメラ位置
	/// @param cameraForward カメラ前方ベクトル
	/// @return 表示範囲内へ補正した座標
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
	MagEngine::CameraManager *cameraManager_;
	MagEngine::LineManager *lineManager_;
	MagEngine::TextRenderer *textRenderer_ = nullptr;

	// プレイヤー参照
	const Player *currentPlayer_;
	MagEngine::Camera *currentCamera_;

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
	float hpRatio_;
	int currentHp_;
	int maxHp_;
	int missileAmmo_;
	int maxMissileAmmo_;
	float missileRecoveryTimer_;
	float missileRecoveryTime_;
	bool isJustAvoidanceWindowActive_;
	float justAvoidanceWindowRatio_;

	// HUD表示制御
	bool showBoresight_;
	bool showRollScale_;
	bool showCompass_;
	bool showGForce_;
	bool showVelocityVector_;
	bool showFlightPath_;
	bool showPitchLadder_;
	bool showLockOnReticle_;
	bool showEnemyIndicators_;

	// ロックオン情報
	EnemyHandle lockOnTargetHandle_{};
	float lockOnRange_;
	float lockOnFOV_;
	int lockedEnemyCount_;
	bool isMissileLockOnMode_ = false;

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
	/// @brief HUD展開・収束アニメーションの時間を更新する
	/// @param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void UpdateAnimation(float unscaledDeltaTime);
	/// @brief 現在の展開進行度を取得する
	/// @return 0.0から1.0の展開進行度
	float GetDeployProgress() const;
	/// @brief 展開演出に用いる三次の減速補間を適用する
	/// @param t 補間率
	/// @return 補間後の値
	float EaseOutCubic(float t) const;

	// ジャスト回避成功演出
	/// @brief ジャスト回避成功通知を描画する
	/// @param progress 展開アニメーションの進行度
	void DrawJustAvoidanceNotification(float progress = 1.0f);
	
	// ジャスト回避状態
	bool justAvoidanceDisplayActive_;			// 表示中フラグ
	float justAvoidanceNotificationTimer_;		// 表示タイマー
	float justAvoidanceNotificationDuration_;	// 表示期間（秒）
	float justAvoidanceSuccessRate_;			// 成功率（0.0～1.0）
	
	public:
	/// @brief ジャスト回避成功時の通知表示を開始する
	/// @param successRate ジャスト回避の成功度（0.0から1.0）
	void PlayJustAvoidanceEffect(float successRate);
};

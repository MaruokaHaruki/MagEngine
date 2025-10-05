#include "TitleCamera.h"
#include "AffineTransformations.h"
#include "Camera.h"
#include "CameraManager.h"
#include "FollowCamera.h"
#include "ImguiSetup.h"
#include "MathFunc4x4.h"
#include "Player.h"
#include <cmath>

namespace {
	const float PI = 3.14159265f;

	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}
}

///=============================================================================
///                        初期化
void TitleCamera::Initialize(const std::string &cameraName) {
	cameraName_ = cameraName;

	// カメラマネージャからカメラを取得
	CameraManager *cameraManager = CameraManager::GetInstance();
	camera_ = cameraManager->GetCamera(cameraName);

	player_ = nullptr;
	currentPhase_ = TitleCameraPhase::Opening;
	phaseTimer_ = 0.0f;
	totalElapsedTime_ = 0.0f;
	loopRotationAngle_ = 0.0f;
	loopTime_ = 0.0f;

	// フェーズ遷移の初期化
	isTransitioning_ = false;
	transitionTimer_ = 0.0f;
	nextPhase_ = TitleCameraPhase::Opening;

	// 滑らかな追従の初期化
	cameraVelocity_ = {0.0f, 0.0f, 0.0f};
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	cameraSmoothTime_ = 0.3f; // 追従の滑らかさ
	lastPlayerPosition_ = {0.0f, 5.0f, 10.0f};

	// 初期カメラパラメータ（プレイヤーが見える位置）
	cameraPosition_ = {0.0f, 3.0f, -10.0f}; // プレイヤーの後方
	cameraTarget_ = {0.0f, 5.0f, 10.0f};	// プレイヤーの初期位置
	cameraFOV_ = 0.45f;
	cameraExposure_ = 0.3f; // 暗めから開始
}

///=============================================================================
///                        更新
void TitleCamera::Update() {
	if (!camera_)
		return;

	const float deltaTime = 1.0f / 60.0f;
	phaseTimer_ += deltaTime;
	totalElapsedTime_ += deltaTime;

	// フェーズ遷移中の処理
	if (isTransitioning_) {
		UpdatePhaseTransition(deltaTime);
	} else {
		// 通常のフェーズ更新
		switch (currentPhase_) {
		case TitleCameraPhase::Opening:
			UpdateOpening(deltaTime);
			if (phaseTimer_ >= OPENING_DURATION) {
				TransitionToNextPhase(TitleCameraPhase::HeroShot);
			}
			break;

		case TitleCameraPhase::HeroShot:
			UpdateHeroShot(deltaTime);
			if (phaseTimer_ >= HEROSHOT_DURATION) {
				TransitionToNextPhase(TitleCameraPhase::TitleDisplay);
			}
			break;

		case TitleCameraPhase::TitleDisplay:
			UpdateTitleDisplay(deltaTime);
			if (phaseTimer_ >= TITLE_DISPLAY_DURATION) {
				TransitionToNextPhase(TitleCameraPhase::Loop);
			}
			break;

		case TitleCameraPhase::Loop:
			UpdateLoop(deltaTime);
			break;
		}
	}

	// カメラにパラメータを適用
	Transform cameraTransform = camera_->GetTransform();
	cameraTransform.translate = cameraPosition_;

	// LookAt処理：カメラをターゲットに向ける
	Vector3 forward = {
		cameraTarget_.x - cameraPosition_.x,
		cameraTarget_.y - cameraPosition_.y,
		cameraTarget_.z - cameraPosition_.z};

	// 正規化
	float length = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
	if (length > 0.001f) {
		forward.x /= length;
		forward.y /= length;
		forward.z /= length;

		// Y軸周りの回転（ヨー）
		cameraTransform.rotate.y = std::atan2(forward.x, forward.z);

		// X軸周りの回転（ピッチ）
		float horizontalLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
		cameraTransform.rotate.x = -std::atan2(forward.y, horizontalLength);
	}

	camera_->SetTransform(cameraTransform);
}

///=============================================================================
///                フェーズ遷移処理
void TitleCamera::TransitionToNextPhase(TitleCameraPhase nextPhase) {
	// 現在の位置とターゲットを保存
	transitionStartPos_ = cameraPosition_;
	transitionStartTarget_ = cameraTarget_;

	// 次のフェーズの開始位置を計算
	nextPhase_ = nextPhase;
	isTransitioning_ = true;
	transitionTimer_ = 0.0f;

	// 次のフェーズの初期位置を設定
	if (player_) {
		Vector3 playerPos = player_->GetPosition();

		switch (nextPhase) {
		case TitleCameraPhase::HeroShot:
			// ヒーローショットの開始位置（プレイヤー中心）
			transitionEndPos_ = {
				playerPos.x + 6.0f,
				playerPos.y + 8.0f,
				playerPos.z - 14.0f};
			transitionEndTarget_ = playerPos; // プレイヤーを直接見る
			break;

		case TitleCameraPhase::TitleDisplay:
			// タイトル表示の開始位置（プレイヤー中心）
			transitionEndPos_ = {
				playerPos.x,
				playerPos.y + 4.0f,
				playerPos.z - 10.0f};
			transitionEndTarget_ = {
				playerPos.x,
				playerPos.y - 0.0f, // プレイヤーの中心
				playerPos.z};
			break;

		case TitleCameraPhase::Loop:
			// ループの開始位置（プレイヤー中心の円軌道）
			float radius = 12.0f;
			float height = 6.0f;
			transitionEndPos_ = {
				playerPos.x + radius,
				playerPos.y + height,
				playerPos.z};
			transitionEndTarget_ = playerPos; // プレイヤーを直接見る
			loopRotationAngle_ = 0.0f;
			loopTime_ = 0.0f;
			break;
		}
	}
}

void TitleCamera::UpdatePhaseTransition(float deltaTime) {
	transitionTimer_ += deltaTime;
	float t = transitionTimer_ / TRANSITION_DURATION;

	if (t >= 1.0f) {
		// 遷移完了
		isTransitioning_ = false;
		currentPhase_ = nextPhase_;
		phaseTimer_ = 0.0f;
		t = 1.0f;
	}

	// イージング適用（滑らかな遷移）
	float easedT = EaseInOut(t);

	// 位置とターゲットを補間
	cameraPosition_ = {
		Lerp(transitionStartPos_.x, transitionEndPos_.x, easedT),
		Lerp(transitionStartPos_.y, transitionEndPos_.y, easedT),
		Lerp(transitionStartPos_.z, transitionEndPos_.z, easedT)};

	cameraTarget_ = {
		Lerp(transitionStartTarget_.x, transitionEndTarget_.x, easedT),
		Lerp(transitionStartTarget_.y, transitionEndTarget_.y, easedT),
		Lerp(transitionStartTarget_.z, transitionEndTarget_.z, easedT)};
}

///=============================================================================
///                【1】オープニング・イントロ
void TitleCamera::UpdateOpening(float deltaTime) {
	float t = phaseTimer_ / OPENING_DURATION;
	t = EaseOutCubic(t);

	if (!player_) {
		Vector3 startPos = {0.0f, 2.0f, -15.0f};
		Vector3 endPos = {0.0f, 8.0f, -12.0f};
		Vector3 controlPoint1 = {2.0f, 4.0f, -14.0f};
		Vector3 controlPoint2 = {-2.0f, 7.0f, -13.0f};
		cameraPosition_ = CubicBezier(startPos, controlPoint1, controlPoint2, endPos, t);
		return;
	}

	Vector3 playerPos = player_->GetPosition();

	// プレイヤーを画面中央に配置（ターゲットはプレイヤー自身）
	Vector3 targetLookAt = {
		playerPos.x,
		playerPos.y, // プレイヤーの中心
		playerPos.z};

	cameraTarget_ = SmoothDamp(cameraTarget_, targetLookAt, targetVelocity_, cameraSmoothTime_ * 0.5f, deltaTime);

	// カメラ位置：プレイヤーの後方から見る
	Vector3 baseOffset = {
		Lerp(0.0f, 0.0f, t),	// 左右中央
		Lerp(3.0f, 5.0f, t),	// やや上方
		Lerp(-18.0f, -14.0f, t) // 後方
	};

	Vector3 desiredCameraPos = {
		playerPos.x + baseOffset.x,
		playerPos.y + baseOffset.y,
		playerPos.z + baseOffset.z};

	cameraPosition_ = SmoothDamp(cameraPosition_, desiredCameraPos, cameraVelocity_, cameraSmoothTime_ * 0.4f, deltaTime);

	cameraExposure_ = Lerp(0.3f, 1.0f, t);
	cameraFOV_ = 0.45f;
}

///=============================================================================
///                【2】戦闘機登場シーン（ヒーローショット）
void TitleCamera::UpdateHeroShot(float deltaTime) {
	float t = phaseTimer_ / HEROSHOT_DURATION;

	if (!player_)
		return;

	Vector3 playerPos = player_->GetPosition();

	// プレイヤーを完全に画面中央に配置
	cameraTarget_ = SmoothDamp(cameraTarget_, playerPos, targetVelocity_, cameraSmoothTime_ * 0.3f, deltaTime);

	if (t < 0.4f) {
		// 前半：やや上方から見下ろす
		float flareT = EaseInCubic(t / 0.4f);
		cameraExposure_ = Lerp(1.0f, 2.0f, flareT);

		Vector3 desiredPos = {
			playerPos.x + Lerp(6.0f, 4.0f, flareT),	 // 側面寄り
			playerPos.y + Lerp(8.0f, 6.0f, flareT),	 // 上方
			playerPos.z - Lerp(14.0f, 12.0f, flareT) // 後方
		};

		cameraPosition_ = SmoothDamp(cameraPosition_, desiredPos, cameraVelocity_, cameraSmoothTime_ * 0.5f, deltaTime);
	} else {
		// 後半：より近づいてダイナミックに
		float passT = (t - 0.4f) / 0.6f;
		passT = EaseInOut(passT);

		cameraExposure_ = Lerp(2.0f, 1.0f, passT);

		Vector3 desiredPos = {
			playerPos.x + Lerp(4.0f, 3.0f, passT),	// 徐々に中央へ
			playerPos.y + Lerp(6.0f, 4.0f, passT),	// やや下へ
			playerPos.z - Lerp(12.0f, 10.0f, passT) // 近づく
		};

		cameraPosition_ = SmoothDamp(cameraPosition_, desiredPos, cameraVelocity_, cameraSmoothTime_ * 0.6f, deltaTime);
	}
}

///=============================================================================
///                【3】タイトル表示カット
void TitleCamera::UpdateTitleDisplay(float deltaTime) {
	float t = phaseTimer_ / TITLE_DISPLAY_DURATION;
	t = EaseOutCubic(t);

	if (!player_)
		return;

	Vector3 playerPos = player_->GetPosition();

	// プレイヤーを画面中央やや下に配置（タイトルロゴスペース確保）
	Vector3 targetLookAt = {
		playerPos.x,
		playerPos.y - Lerp(0.0f, 0.5f, t), // 少しずつ下を見る
		playerPos.z};
	cameraTarget_ = SmoothDamp(cameraTarget_, targetLookAt, targetVelocity_, cameraSmoothTime_ * 0.4f, deltaTime);

	// カメラ位置：プレイヤーの真後ろから
	Vector3 offsetPos = {
		Lerp(0.0f, 0.0f, t),	// 左右中央を維持
		Lerp(4.0f, 8.0f, t),	// 徐々に上昇
		Lerp(-10.0f, -16.0f, t) // 徐々に後退
	};

	Vector3 desiredPos = {
		playerPos.x + offsetPos.x,
		playerPos.y + offsetPos.y,
		playerPos.z + offsetPos.z};

	cameraPosition_ = SmoothDamp(cameraPosition_, desiredPos, cameraVelocity_, cameraSmoothTime_ * 0.5f, deltaTime);
	cameraExposure_ = 1.0f;
}

///=============================================================================
///                【4】ループ演出
void TitleCamera::UpdateLoop(float deltaTime) {
	loopTime_ += deltaTime;
	loopRotationAngle_ += deltaTime * 0.1f; // 旋回速度

	if (!player_)
		return;

	Vector3 playerPos = player_->GetPosition();

	// プレイヤーを常に画面中央に配置
	cameraTarget_ = SmoothDamp(cameraTarget_, playerPos, targetVelocity_, cameraSmoothTime_ * 0.3f, deltaTime);

	// カメラはプレイヤーを中心に円を描く（プレイヤー中心）
	float radius = 12.0f;									  // 距離を近めに
	float height = 6.0f + 2.0f * std::sin(loopTime_ * 0.25f); // 高度変化

	Vector3 desiredPos = {
		playerPos.x + radius * std::cos(loopRotationAngle_),
		playerPos.y + height, // プレイヤーのやや上方
		playerPos.z + radius * std::sin(loopRotationAngle_)};

	cameraPosition_ = SmoothDamp(cameraPosition_, desiredPos, cameraVelocity_, cameraSmoothTime_ * 0.5f, deltaTime);
	cameraExposure_ = 1.0f + 0.1f * std::sin(loopTime_ * 0.15f);
}

///=============================================================================
///                補間関数
Vector3 TitleCamera::InterpolatePosition(float t, const Vector3 &start, const Vector3 &end) {
	return {
		Lerp(start.x, end.x, t),
		Lerp(start.y, end.y, t),
		Lerp(start.z, end.z, t)};
}

Vector3 TitleCamera::CubicBezier(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3, float t) {
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;

	Vector3 result;
	result.x = uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x;
	result.y = uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y;
	result.z = uuu * p0.z + 3.0f * uu * t * p1.z + 3.0f * u * tt * p2.z + ttt * p3.z;

	return result;
}

float TitleCamera::EaseInOut(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float TitleCamera::EaseOutCubic(float t) {
	float u = 1.0f - t;
	return 1.0f - u * u * u;
}

float TitleCamera::EaseInCubic(float t) {
	return t * t * t;
}

void TitleCamera::Reset() {
	currentPhase_ = TitleCameraPhase::Opening;
	phaseTimer_ = 0.0f;
	totalElapsedTime_ = 0.0f;
	loopRotationAngle_ = 0.0f;
	loopTime_ = 0.0f;
	isTransitioning_ = false;
	transitionTimer_ = 0.0f;
}

///=============================================================================
///                        ImGui描画
void TitleCamera::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("TitleCamera");

	const char *phaseNames[] = {"Opening", "HeroShot", "TitleDisplay", "Loop"};
	int currentPhaseIndex = static_cast<int>(currentPhase_);
	ImGui::Text("Current Phase: %s", phaseNames[currentPhaseIndex]);
	ImGui::Text("Phase Timer: %.2f", phaseTimer_);
	ImGui::Text("Total Time: %.2f", totalElapsedTime_);

	ImGui::Separator();
	ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
				cameraPosition_.x, cameraPosition_.y, cameraPosition_.z);
	ImGui::Text("Camera Target: (%.2f, %.2f, %.2f)",
				cameraTarget_.x, cameraTarget_.y, cameraTarget_.z);
	ImGui::Text("Exposure: %.2f", cameraExposure_);

	if (ImGui::Button("Reset")) {
		Reset();
	}

	ImGui::End();
#endif
}

///=============================================================================
///                SmoothDamp（Unity風の滑らかな補間）
Vector3 TitleCamera::SmoothDamp(const Vector3 &current, const Vector3 &target, Vector3 &velocity, float smoothTime, float deltaTime) {
	// 最大速度を制限（急激な変化を防ぐ）
	float maxSpeed = 100.0f;

	// 各軸ごとに計算
	float omega = 2.0f / smoothTime;
	float x = omega * deltaTime;
	float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

	Vector3 change = {
		current.x - target.x,
		current.y - target.y,
		current.z - target.z};

	Vector3 originalTarget = target;

	// 最大変化量を制限
	float maxChange = maxSpeed * smoothTime;
	float changeLength = std::sqrt(change.x * change.x + change.y * change.y + change.z * change.z);

	if (changeLength > maxChange) {
		float ratio = maxChange / changeLength;
		change.x *= ratio;
		change.y *= ratio;
		change.z *= ratio;
	}

	Vector3 clampedTarget = {
		current.x - change.x,
		current.y - change.y,
		current.z - change.z};

	Vector3 temp = {
		(velocity.x + omega * change.x) * deltaTime,
		(velocity.y + omega * change.y) * deltaTime,
		(velocity.z + omega * change.z) * deltaTime};

	velocity.x = (velocity.x - omega * temp.x) * exp;
	velocity.y = (velocity.y - omega * temp.y) * exp;
	velocity.z = (velocity.z - omega * temp.z) * exp;

	Vector3 result = {
		clampedTarget.x + (change.x + temp.x) * exp,
		clampedTarget.y + (change.y + temp.y) * exp,
		clampedTarget.z + (change.z + temp.z) * exp};

	// オーバーシュート防止
	Vector3 origMinusCurrent = {
		originalTarget.x - current.x,
		originalTarget.y - current.y,
		originalTarget.z - current.z};
	Vector3 resultMinusOrig = {
		result.x - originalTarget.x,
		result.y - originalTarget.y,
		result.z - originalTarget.z};

	if (origMinusCurrent.x * resultMinusOrig.x +
			origMinusCurrent.y * resultMinusOrig.y +
			origMinusCurrent.z * resultMinusOrig.z >
		0) {
		result = originalTarget;
		velocity = {
			(result.x - current.x) / deltaTime,
			(result.y - current.y) / deltaTime,
			(result.z - current.z) / deltaTime};
	}

	return result;
}
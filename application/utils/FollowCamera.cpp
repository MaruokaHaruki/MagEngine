#include "FollowCamera.h"
#include "AffineTransformations.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImguiSetup.h"
#include "MathFunc4x4.h"
#include "Player.h"
#include <cmath>

namespace {
	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}

	inline Vector3 LerpVector3(const Vector3 &a, const Vector3 &b, float t) {
		return {
			Lerp(a.x, b.x, t),
			Lerp(a.y, b.y, t),
			Lerp(a.z, b.z, t)};
	}
}

///=============================================================================
///						初期化
void FollowCamera::Initialize(const std::string &cameraName) {
	cameraName_ = cameraName;

	// カメラマネージャからカメラを取得
	CameraManager *cameraManager = CameraManager::GetInstance();
	camera_ = cameraManager->GetCamera(cameraName);

	// 初期パラメータの設定
	target_ = nullptr;
	offset_ = {0.0f, 1.0f, -16.0f}; // プレイヤーの後方上方
	positionSmoothness_ = 0.01f;
	rotationSmoothness_ = 0.01f;

	// 墜落時のパラメータ
	crashRotationSmoothness_ = 0.001f; // 墜落時はさらに滑らかに
	limitCrashRotation_ = true;		   // デフォルトで回転制限を有効化

	// 固定位置モードの初期化
	isFixedPositionMode_ = false;
	fixedPosition_ = {0.0f, 5.0f, -10.0f}; // デフォルトの固定位置

	// 初期位置・回転の設定
	// NOTE: プレイヤー追従開始前の初期位置
	currentPosition_ = {0.0f, 2.0f, 16.0f};
	currentRotation_ = {0.3f, 0.0f, 0.0f};
	targetPosition_ = currentPosition_;
	targetRotation_ = currentRotation_;

	// カメラに初期トランスフォームを設定
	if (camera_) {
		Transform initialTransform;
		initialTransform.scale = {1.0f, 1.0f, 1.0f};
		initialTransform.translate = currentPosition_;
		initialTransform.rotate = currentRotation_;
		camera_->SetTransform(initialTransform);
	}
}

///=============================================================================
///						更新
void FollowCamera::Update() {
	if (!camera_ || !target_) {
		return;
	}

	UpdateCameraTransform();

	// 固定位置モードの場合は位置を固定位置に設定
	if (isFixedPositionMode_) {
		targetPosition_ = fixedPosition_;
		currentPosition_ = fixedPosition_; // 即座に固定位置に移動
	} else {
		// 通常の追従モードでは位置も滑らかに補間
		currentPosition_ = LerpVector3(currentPosition_, targetPosition_, positionSmoothness_);
	}

	// プレイヤーが墜落中かチェック
	bool isCrashing = target_->IsCrashing();
	float currentRotationSmoothness = isCrashing && limitCrashRotation_ ? crashRotationSmoothness_ : rotationSmoothness_;

	// 回転は常に滑らかに補間（墜落中はさらに滑らかに）
	currentRotation_ = LerpVector3(currentRotation_, targetRotation_, currentRotationSmoothness);

	// カメラのトランスフォームを更新
	Transform cameraTransform;
	cameraTransform.scale = {1.0f, 1.0f, 1.0f};
	cameraTransform.translate = currentPosition_;
	cameraTransform.rotate = currentRotation_;
	camera_->SetTransform(cameraTransform);
}

///=============================================================================
///						カメラの位置と回転を計算
void FollowCamera::UpdateCameraTransform() {
	if (!target_) {
		return;
	}

	// プレイヤーの位置を取得
	Vector3 playerPosition = target_->GetPosition();

	// プレイヤーのトランスフォームを取得
	Transform *playerTransform = target_->obj_->GetTransform();
	if (!playerTransform) {
		return;
	}

	// プレイヤーが墜落中かチェック
	bool isCrashing = target_->IsCrashing();

	if (!isFixedPositionMode_) {
		// 通常の追従モード：プレイヤーの回転に応じてオフセット位置を計算
		Matrix4x4 playerRotationMatrix = MakeRotateMatrix(playerTransform->rotate);
		Vector3 rotatedOffset = Conversion(offset_, playerRotationMatrix);
		targetPosition_ = playerPosition + rotatedOffset;
	}
	// 固定位置モードの場合は targetPosition_ は固定位置のまま（Update()で設定される）

	// カメラがプレイヤーを見るように回転を計算（両モード共通）
	Vector3 lookDirection = Normalize(playerPosition - (isFixedPositionMode_ ? fixedPosition_ : targetPosition_));

	// Y軸回転（左右）を計算
	float yaw = std::atan2(lookDirection.x, lookDirection.z);

	// X軸回転（上下）を計算
	float pitch = std::asin(-lookDirection.y);

	// プレイヤーのZ軸回転を取得してカメラに反映
	float roll = playerTransform->rotate.z;

	if (isCrashing && limitCrashRotation_) {
		// 墜落中はロール回転のみ部分的に追従し、ピッチ・ヨーは現在値を維持
		// ロールも完全には追従せず、ゆっくり追従
		targetRotation_ = {
			currentRotation_.x,										// ピッチは現在値維持
			currentRotation_.y,										// ヨーは現在値維持
			currentRotation_.z + (roll - currentRotation_.z) * 0.3f // ロールのみ30%追従
		};
	} else {
		// 通常時：目標回転を設定（プレイヤーの傾きを反映）
		targetRotation_ = {pitch, yaw, roll};
	}
}

///=============================================================================
///						ターゲットの設定
void FollowCamera::SetTarget(Player *target) {
	target_ = target;
}

///=============================================================================
///						ImGui描画
void FollowCamera::DrawImGui() {
	ImGui::Begin("Follow Camera");

	ImGui::Text("Camera: %s", cameraName_.c_str());
	ImGui::Text("Target: %s", target_ ? "Player" : "None");

	ImGui::Separator();
	ImGui::Text("Camera Transform:");
	ImGui::Text("Position: (%.2f, %.2f, %.2f)", currentPosition_.x, currentPosition_.y, currentPosition_.z);
	ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", currentRotation_.x, currentRotation_.y, currentRotation_.z);

	ImGui::Separator();
	ImGui::Text("Follow Mode:");

	// モード切り替えボタン
	if (ImGui::Checkbox("Fixed Position Mode", &isFixedPositionMode_)) {
		if (isFixedPositionMode_) {
			// 固定位置モードに切り替わった時、現在の位置を固定位置として設定
			fixedPosition_ = currentPosition_;
		}
	}

	if (isFixedPositionMode_) {
		ImGui::Text("Mode: Fixed Position + Rotation Tracking");
		ImGui::DragFloat3("Fixed Position", &fixedPosition_.x, 0.1f);

		// 現在の位置を固定位置として設定するボタン
		if (ImGui::Button("Set Current Position as Fixed")) {
			SetCurrentPositionAsFixed();
		}
	} else {
		ImGui::Text("Mode: Full Follow (Position + Rotation)");
		ImGui::DragFloat3("Offset", &offset_.x, 0.1f);
		ImGui::SliderFloat("Position Smoothness", &positionSmoothness_, 0.01f, 1.0f);
	}

	ImGui::SliderFloat("Rotation Smoothness", &rotationSmoothness_, 0.01f, 1.0f);

	ImGui::Separator();
	ImGui::Text("Crash Settings:");
	ImGui::Checkbox("Limit Crash Rotation", &limitCrashRotation_);
	ImGui::SliderFloat("Crash Rotation Smoothness", &crashRotationSmoothness_, 0.0001f, 0.01f);

	if (target_) {
		Vector3 playerPos = target_->GetPosition();
		ImGui::Separator();
		ImGui::Text("Target Info:");
		ImGui::Text("Player Position: (%.2f, %.2f, %.2f)", playerPos.x, playerPos.y, playerPos.z);
		ImGui::Text("Player Crashing: %s", target_->IsCrashing() ? "YES" : "NO");

		// プレイヤーまでの距離を表示
		Vector3 distanceVec = playerPos - currentPosition_;
		float distance = Length(distanceVec);
		ImGui::Text("Distance to Player: %.2f", distance);
	}

	ImGui::End();
}

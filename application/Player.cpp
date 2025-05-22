/*********************************************************************
 * \file   Player.cpp
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#include "Player.h"
#include "ImguiSetup.h"
#include "ModelManager.h"
#include "Object3d.h"
#include <cmath> // std::abs, std::min, std::max のため

namespace { // 無名名前空間でファイルスコープの定数を定義
	const float PI = 3.1415926535f;

	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}
	inline float DegreesToRadians(float degrees) {
		return degrees * (PI / 180.0f);
	}
	inline float RadiansToDegrees(float radians) {
		return radians * (180.0f / PI);
	}
} // namespace

///=============================================================================
///						初期化
void Player::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	// 移動関連の初期化
	currentVelocity_ = {0.0f, 0.0f, 0.0f};
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	moveSpeed_ = 5.0f;	  // 基本移動速度（単位/秒）
	acceleration_ = 0.1f; // 速度変化の滑らかさ（小さいほど滑らか）

	// 回転（傾き）関連の初期化
	targetRotationEuler_ = {0.0f, 0.0f, 0.0f};
	rollSpeed_ = 60.0f;		   // 度/秒
	pitchSpeed_ = 30.0f;	   // 度/秒
	rotationSmoothing_ = 0.1f; // 傾き変化の滑らかさ
	maxRollAngle_ = 30.0f;	   // 度
	maxPitchAngle_ = 15.0f;	   // 度

	if (obj_) {
		Transform *objTransform = obj_->GetTransform(); // GetTransform() を使用
		if (objTransform) {
			objTransform->translate = {0.0f, 0.0f, 0.0f}; // 初期位置
			objTransform->rotate = {0.0f, 0.0f, 0.0f};	  // 初期回転
		}
	}
}

///=============================================================================
///						更新
void Player::Update(float deltaTime, bool pressW, bool pressS, bool pressA, bool pressD) {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform(); // GetTransform() を使用
	if (!objTransform) {
		return;
	}

	// 目標速度と目標回転をリセット
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	Vector3 desiredRotationEuler = {0.0f, 0.0f, 0.0f}; // このフレームでの目標傾き

	// 上下移動 (W/S)
	if (pressW) {
		targetVelocity_.y += moveSpeed_;
		desiredRotationEuler.x = DegreesToRadians(-maxPitchAngle_); // 機首上げ
	}
	if (pressS) {
		targetVelocity_.y -= moveSpeed_;
		desiredRotationEuler.x = DegreesToRadians(maxPitchAngle_); // 機首下げ
	}

	// 左右移動 (A/D)
	if (pressA) {
		targetVelocity_.x -= moveSpeed_;
		desiredRotationEuler.z = DegreesToRadians(maxRollAngle_); // 左ロール
	}
	if (pressD) {
		targetVelocity_.x += moveSpeed_;
		desiredRotationEuler.z = DegreesToRadians(-maxRollAngle_); // 右ロール
	}

	// 現在の速度を目標速度に滑らかに近づける
	currentVelocity_.x = Lerp(currentVelocity_.x, targetVelocity_.x, acceleration_);
	currentVelocity_.y = Lerp(currentVelocity_.y, targetVelocity_.y, acceleration_);
	// Z軸方向の速度は今回は0
	currentVelocity_.z = Lerp(currentVelocity_.z, targetVelocity_.z, acceleration_);

	// 位置を更新
	objTransform->translate.x += currentVelocity_.x * deltaTime;
	objTransform->translate.y += currentVelocity_.y * deltaTime;
	objTransform->translate.z += currentVelocity_.z * deltaTime;

	// 目標の傾きを更新 (入力がない場合は0に戻るように)
	targetRotationEuler_.x = Lerp(targetRotationEuler_.x, desiredRotationEuler.x, rotationSmoothing_);
	targetRotationEuler_.y = Lerp(targetRotationEuler_.y, desiredRotationEuler.y, rotationSmoothing_); // Y軸回転は今回はなし
	targetRotationEuler_.z = Lerp(targetRotationEuler_.z, desiredRotationEuler.z, rotationSmoothing_);

	// 傾きを適用
	objTransform->rotate.x = targetRotationEuler_.x;
	objTransform->rotate.y = targetRotationEuler_.y;
	objTransform->rotate.z = targetRotationEuler_.z;

	// Object3dの更新 (ワールド行列の更新など)
	obj_->Update();
}

///=============================================================================
///                     描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

///=============================================================================
///						ImGui描画
void Player::DrawImGui() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform(); // GetTransform() を使用
	if (objTransform) {
		ImGui::Begin("Player Debug");
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", objTransform->translate.x, objTransform->translate.y, objTransform->translate.z);
		ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity_.x, currentVelocity_.y, currentVelocity_.z);
		ImGui::Text(
			"Rotation (Deg): (%.1f, %.1f, %.1f)",
			RadiansToDegrees(objTransform->rotate.x),
			RadiansToDegrees(objTransform->rotate.y),
			RadiansToDegrees(objTransform->rotate.z));
		ImGui::SliderFloat("Move Speed", &moveSpeed_, 1.0f, 20.0f);
		ImGui::SliderFloat("Acceleration", &acceleration_, 0.01f, 0.5f);
		ImGui::SliderFloat("Max Roll (Deg)", &maxRollAngle_, 5.0f, 90.0f);
		ImGui::SliderFloat("Max Pitch (Deg)", &maxPitchAngle_, 5.0f, 45.0f);
		ImGui::SliderFloat("Rotation Smoothing", &rotationSmoothing_, 0.01f, 0.5f);
		ImGui::End();
	}
}

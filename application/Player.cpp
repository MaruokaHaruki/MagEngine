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

	// Object3dSetupを保存（弾の初期化で使用）
	object3dSetup_ = object3dSetup;

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

	// 弾関連の初期化
	shootCoolTime_ = 0.0f;
	maxShootCoolTime_ = 0.2f; // 0.2秒間隔で発射

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
void Player::Update() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform) {
		return;
	}

	// プレイヤーの動作関係処理を統合
	UpdateMovement();

	// 弾の発射処理
	ProcessShooting();

	// 弾の更新・削除処理
	UpdateBullets();

	// Object3dの更新 (ワールド行列の更新など)
	obj_->Update();
}

///=============================================================================
///						プレイヤーの動作関係処理（入力処理、移動、回転を統合）
void Player::UpdateMovement() {
	// 入力情報を取得
	Input *input = Input::GetInstance();
	bool pressW = input->PushKey(DIK_W);
	bool pressS = input->PushKey(DIK_S);
	bool pressA = input->PushKey(DIK_A);
	bool pressD = input->PushKey(DIK_D);

	// 動作関係処理を順次実行
	ProcessMovementInput(pressW, pressS, pressA, pressD);
	UpdateVelocity();
	UpdatePosition();
	UpdateRotation();
}

///=============================================================================
///						入力に基づいて目標速度と目標回転を設定
void Player::ProcessMovementInput(bool pressW, bool pressS, bool pressA, bool pressD) {
	// 目標速度と目標回転をリセット
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	Vector3 desiredRotationEuler = {0.0f, 0.0f, 0.0f};

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

	// 目標の傾きを更新 (入力がない場合は0に戻るように)
	targetRotationEuler_.x = Lerp(targetRotationEuler_.x, desiredRotationEuler.x, rotationSmoothing_);
	targetRotationEuler_.y = Lerp(targetRotationEuler_.y, desiredRotationEuler.y, rotationSmoothing_);
	targetRotationEuler_.z = Lerp(targetRotationEuler_.z, desiredRotationEuler.z, rotationSmoothing_);
}

///=============================================================================
///						現在の速度を目標速度に向けて更新
void Player::UpdateVelocity() {
	// 現在の速度を目標速度に滑らかに近づける
	currentVelocity_.x = Lerp(currentVelocity_.x, targetVelocity_.x, acceleration_);
	currentVelocity_.y = Lerp(currentVelocity_.y, targetVelocity_.y, acceleration_);
	currentVelocity_.z = Lerp(currentVelocity_.z, targetVelocity_.z, acceleration_);
}

///=============================================================================
///						位置を速度に基づいて更新
void Player::UpdatePosition() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform) {
		return;
	}

	// 60FPS固定での位置更新（1フレーム = 1/60秒）
	const float frameTime = 1.0f / 60.0f;
	objTransform->translate.x += currentVelocity_.x * frameTime;
	objTransform->translate.y += currentVelocity_.y * frameTime;
	objTransform->translate.z += currentVelocity_.z * frameTime;
}

///=============================================================================
///						回転（傾き）を更新
void Player::UpdateRotation() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform) {
		return;
	}

	// 傾きを適用
	objTransform->rotate.x = targetRotationEuler_.x;
	objTransform->rotate.y = targetRotationEuler_.y;
	objTransform->rotate.z = targetRotationEuler_.z;
}

///=============================================================================
///						弾の発射処理
void Player::ProcessShooting() {
	Input *input = Input::GetInstance();

	// クールタイムの更新
	if (shootCoolTime_ > 0.0f) {
		shootCoolTime_ -= 1.0f / 60.0f;
	}

	// スペースキーで弾を発射
	if (input->PushKey(DIK_SPACE) && shootCoolTime_ <= 0.0f) {
		Vector3 playerPos = obj_->GetPosition();
		Vector3 shootDirection = {0.0f, 0.0f, 1.0f}; // 前方向に発射

		// 弾を生成
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(object3dSetup_, "axisPlus.obj", playerPos, shootDirection);
		bullets_.push_back(std::move(bullet));

		// クールタイムをリセット
		shootCoolTime_ = maxShootCoolTime_;
	}
}

///=============================================================================
///						弾の更新・削除処理
void Player::UpdateBullets() {
	// 弾の更新
	for (auto &bullet : bullets_) {
		bullet->Update();
	}

	// 死んだ弾を削除
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
					   [](const std::unique_ptr<PlayerBullet> &bullet) {
						   return !bullet->IsAlive();
					   }),
		bullets_.end());
}

///=============================================================================
///                     描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

///=============================================================================
///						弾の描画
void Player::DrawBullets() {
	for (auto &bullet : bullets_) {
		bullet->Draw();
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
		ImGui::Text("Bullets Count: %zu", bullets_.size());
		ImGui::SliderFloat("Shoot Cool Time", &maxShootCoolTime_, 0.05f, 1.0f);
		ImGui::End();
	}
}

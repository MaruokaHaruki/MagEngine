/*********************************************************************
 * \file   Player.cpp
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note   プレイヤークラス - 移動、射撃、パーティクル、HP管理
 *********************************************************************/
#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "Player.h"
#include "ImguiSetup.h"
#include "ModelManager.h"
#include "Object3d.h"
#include <algorithm>
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

//=======================================================================
// 初期化
void Player::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath) {
	// === コア初期化 ===
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	// === 移動関連の初期化 ===
	currentVelocity_ = {0.0f, 0.0f, 0.0f};
	targetVelocity_ = {0.0f, 0.0f, 0.0f};
	moveSpeed_ = 5.0f;
	acceleration_ = 0.1f;

	// === 回転関連の初期化 ===
	targetRotationEuler_ = {0.0f, 0.0f, 0.0f};
	rotationSmoothing_ = 0.1f;
	maxRollAngle_ = 30.0f;
	maxPitchAngle_ = 15.0f;

	// === 射撃関連の初期化 ===
	shootCoolTime_ = 0.0f;
	maxShootCoolTime_ = 0.2f;

	// === HP関連の初期化 ===
	maxHP_ = 100;
	currentHP_ = maxHP_;
	isInvincible_ = false;
	invincibleTime_ = 0.0f;
	maxInvincibleTime_ = 1.0f; // 1秒間無敵

	// === パーティクル関連の初期化 ===
	particleSystem_ = nullptr;
	particleSetup_ = nullptr;

	// === オブジェクト位置・当たり判定の初期化 ===
	if (obj_) {
		Transform *objTransform = obj_->GetTransform();
		if (objTransform) {
			objTransform->translate = {0.0f, 0.0f, 0.0f};
			objTransform->rotate = {0.0f, 0.0f, 0.0f};

			Vector3 pos = objTransform->translate;
			BaseObject::Initialize(pos, 1.0f);
		}
	}
}

void Player::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particleSystem_ = particle;
	particleSetup_ = particleSetup;

	if (particleSystem_) {
		// ジェット煙用パーティクルグループを作成
		particleSystem_->CreateParticleGroup("JetSmoke", "sandWind.png", ParticleShape::Board);

		// ジェット煙の設定
		particleSystem_->SetBillboard(true);
		particleSystem_->SetCustomTextureSize({5.0f, 5.0f});
		particleSystem_->SetTranslateRange({-0.2f, -0.2f, -0.2f}, {0.2f, 0.2f, 0.2f});
		particleSystem_->SetVelocityRange({-0.5f, -0.5f, -2.0f}, {0.5f, 0.5f, -0.5f});
		particleSystem_->SetColorRange({0.8f, 0.8f, 0.8f, 0.7f}, {1.0f, 1.0f, 1.0f, 0.9f});
		particleSystem_->SetLifetimeRange(1.0f, 2.5f);
		particleSystem_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.6f, 0.6f, 0.6f});
		particleSystem_->SetEndScaleRange({1.2f, 1.2f, 1.2f}, {2.0f, 2.0f, 2.0f});
		particleSystem_->SetFadeInOut(0.1f, 0.6f);

		// エミッターの作成（プレイヤーの初期位置から）
		Vector3 initialPos = obj_->GetPosition();
		Transform emitterTransform = {};
		emitterTransform.translate = {initialPos.x, initialPos.y, initialPos.z - 1.5f}; // プレイヤーの後方

		jetSmokeEmitter_ = std::make_unique<ParticleEmitter>(
			particleSystem_, "JetSmoke", emitterTransform, 3, 0.1f, true);
	}
}

//=============================================================================
// 更新
void Player::Update() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform) {
		return;
	}

	// === 無敵時間の更新 ===
	if (isInvincible_) {
		invincibleTime_ -= 1.0f / 60.0f; // 60FPS想定
		if (invincibleTime_ <= 0.0f) {
			isInvincible_ = false;
		}
	}

	// === プレイヤーの各種更新処理 ===
	UpdateMovement();
	UpdateJetSmoke();
	ProcessShooting();
	UpdateBullets();

	// === 当たり判定・オブジェクト更新 ===
	BaseObject::Update(objTransform->translate);
	obj_->Update();
}

void Player::UpdateJetSmoke() {
	if (jetSmokeEmitter_ && obj_) {
		Vector3 playerPos = obj_->GetPosition();

		// エミッターをプレイヤーの後方に配置
		Vector3 jetSmokePos = {playerPos.x, playerPos.y, playerPos.z - 1.5f};
		jetSmokeEmitter_->SetTranslate(jetSmokePos);

		// エミッターの更新
		jetSmokeEmitter_->Update();
	}
}

//=============================================================================
// プレイヤーの動作関係処理（入力処理、移動、回転を統合）
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

//=============================================================================
// 入力に基づいて目標速度と目標回転を設定
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

//=============================================================================
// 現在の速度を目標速度に向けて更新
void Player::UpdateVelocity() {
	// 現在の速度を目標速度に滑らかに近づける
	currentVelocity_.x = Lerp(currentVelocity_.x, targetVelocity_.x, acceleration_);
	currentVelocity_.y = Lerp(currentVelocity_.y, targetVelocity_.y, acceleration_);
	currentVelocity_.z = Lerp(currentVelocity_.z, targetVelocity_.z, acceleration_);
}

//=============================================================================
// 位置を速度に基づいて更新
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

//=============================================================================
// 回転（傾き）を更新
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

//=============================================================================
// 弾の発射処理
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

//=============================================================================
// 弾の更新・削除処理
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

//=============================================================================
// 描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

//=============================================================================
// 弾の描画
void Player::DrawBullets() {
	for (auto &bullet : bullets_) {
		bullet->Draw();
	}
}

//=============================================================================
// ImGui描画
void Player::DrawImGui() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		ImGui::Begin("Player Debug");

		// === HP表示 ===
		ImGui::Text("=== HP Status ===");
		ImGui::Text("HP: %d / %d", currentHP_, maxHP_);
		ImGui::ProgressBar(GetHPRatio(), ImVec2(200, 20), "");
		ImGui::Text("Invincible: %s", isInvincible_ ? "Yes" : "No");
		if (isInvincible_) {
			ImGui::Text("Invincible Time: %.2fs", invincibleTime_);
		}
		ImGui::SliderInt("Max HP", &maxHP_, 50, 500);
		if (ImGui::Button("Take Damage (10)")) {
			TakeDamage(10);
		}
		ImGui::SameLine();
		if (ImGui::Button("Heal (20)")) {
			Heal(20);
		}

		ImGui::Separator();

		// === 位置・移動情報 ===
		ImGui::Text("=== Movement Status ===");
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", objTransform->translate.x, objTransform->translate.y, objTransform->translate.z);
		ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", currentVelocity_.x, currentVelocity_.y, currentVelocity_.z);
		ImGui::Text(
			"Rotation (Deg): (%.1f, %.1f, %.1f)",
			RadiansToDegrees(objTransform->rotate.x),
			RadiansToDegrees(objTransform->rotate.y),
			RadiansToDegrees(objTransform->rotate.z));

		// === 移動パラメータ調整 ===
		ImGui::Text("=== Movement Parameters ===");
		ImGui::SliderFloat("Move Speed", &moveSpeed_, 1.0f, 20.0f);
		ImGui::SliderFloat("Acceleration", &acceleration_, 0.01f, 0.5f);
		ImGui::SliderFloat("Max Roll (Deg)", &maxRollAngle_, 5.0f, 90.0f);
		ImGui::SliderFloat("Max Pitch (Deg)", &maxPitchAngle_, 5.0f, 45.0f);
		ImGui::SliderFloat("Rotation Smoothing", &rotationSmoothing_, 0.01f, 0.5f);

		ImGui::Separator();

		// === 射撃情報 ===
		ImGui::Text("=== Shooting Status ===");
		ImGui::Text("Bullets Count: %zu", bullets_.size());
		ImGui::SliderFloat("Shoot Cool Time", &maxShootCoolTime_, 0.05f, 1.0f);

		// === ジェット煙制御 ===
		if (jetSmokeEmitter_) {
			ImGui::Separator();
			ImGui::Text("=== Jet Smoke Control ===");
			bool repeat = true;
			if (ImGui::Checkbox("Enable Jet Smoke", &repeat)) {
				jetSmokeEmitter_->SetRepeat(repeat);
			}
		}

		ImGui::End();
	}
}

//=============================================================================
// HP関連処理
void Player::TakeDamage(int damage) {
	// 無敵状態または既に死亡している場合はダメージを受けない
	if (isInvincible_ || !IsAlive()) {
		return;
	}

	// ダメージを適用
	currentHP_ = std::max(0, currentHP_ - damage);

	// ダメージを受けた場合は無敵状態にする
	if (damage > 0) {
		isInvincible_ = true;
		invincibleTime_ = maxInvincibleTime_;
	}
}

void Player::Heal(int healAmount) {
	if (!IsAlive()) {
		return;
	}

	currentHP_ = std::min(maxHP_, currentHP_ + healAmount);
}

//=======================================================================
// 衝突処理
void Player::OnCollisionEnter(BaseObject *other) {
	// 敵との衝突時にダメージを受ける
	TakeDamage(10); // 衝突時のダメージ量
}

void Player::OnCollisionStay(BaseObject *other) {
	// 継続中の衝突処理（必要に応じて実装）
}

void Player::OnCollisionExit(BaseObject *other) {
	// 衝突終了時の処理（必要に応じて実装）
}

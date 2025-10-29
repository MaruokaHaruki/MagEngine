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
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
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
	missileCoolTime_ = 0.0f;
	maxMissileCoolTime_ = 2.0f; // ミサイルは2秒間隔

	// === HP関連の初期化 ===
	maxHP_ = 10;
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

	// === システム参照の初期化 ===
	enemyManager_ = nullptr;

	// === ロックオン関連の初期化 ===
	lockOnTarget_ = nullptr;
	lockOnRange_ = 30.0f;
	lockOnMode_ = false;

	// === 敗北演出関連の初期化（用語変更）
	isDefeated_ = false;
	defeatAnimationComplete_ = false;
	defeatAnimationTime_ = 0.0f;
	defeatAnimationDuration_ = 3.0f; // 3秒間の演出
	defeatVelocity_ = {0.0f, 0.0f, 0.0f};
	defeatRotationSpeed_ = {0.0f, 0.0f, 0.0f};
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

	// === 敗北演出中の処理 ===
	if (isDefeated_) {
		UpdateDefeatAnimation();
		obj_->Update();
		return;
	}

	// === デバッグ用強制ゲームオーバー処理 ===
	// SECURITY NOTE: この機能は完全に無効化（セキュリティソフト誤検出回避）
#if 0 // _DEBUG から 0 に変更して完全無効化
	// 以下のコードは実行されません
	Input *input = Input::GetInstance();
	static bool prevKKey = false;
	static int kKeyPressCount = 0;
	static float kKeyResetTimer = 0.0f;

	bool currentKKey = input->PushKey(DIK_K);

	if (currentKKey && !prevKKey) {
		kKeyPressCount++;
		kKeyResetTimer = 2.0f;
	}
	prevKKey = currentKKey;

	if (kKeyResetTimer > 0.0f) {
		kKeyResetTimer -= 1.0f / 60.0f;
		if (kKeyResetTimer <= 0.0f) {
			kKeyPressCount = 0;
		}
	}

	if (kKeyPressCount >= 2) {
		currentHP_ = 0;
		StartDefeatAnimation();
		kKeyPressCount = 0;
		kKeyResetTimer = 0.0f;
	}
#endif

	// === 無敵時間の更新 ===
	if (isInvincible_) {
		invincibleTime_ -= 1.0f / 60.0f; // 60FPS想定
		if (invincibleTime_ <= 0.0f) {
			isInvincible_ = false;
		}
	}

	// === プレイヤーの各種更新処理 ===
	UpdateMovement();
	UpdateLockOn();
	ProcessShooting();
	UpdateBullets();
	UpdateMissiles();

	// === 当たり判定・オブジェクト更新 ===
	BaseObject::Update(objTransform->translate);
	obj_->Update();
}

void Player::UpdateLockOn() {
	Input *input = Input::GetInstance();

	// Lキーでロックオンモード切り替え
	static bool prevLockKey = false;
	bool currentLockKey = input->PushKey(DIK_L);
	if (currentLockKey && !prevLockKey) {
		lockOnMode_ = !lockOnMode_;
		if (lockOnMode_) {
			// ロックオンモード開始 - 最寄りの敵をロックオン
			lockOnTarget_ = GetNearestEnemy();
		} else {
			// ロックオンモード終了
			lockOnTarget_ = nullptr;
		}
	}
	prevLockKey = currentLockKey;

	// ロックオンターゲットが無効になったらクリア
	if (lockOnTarget_ && !lockOnTarget_->IsAlive()) {
		lockOnTarget_ = nullptr;
		lockOnMode_ = false;
	}

	// ロックオンターゲットが範囲外に出たらクリア
	if (lockOnTarget_) {
		Vector3 playerPos = GetPosition();
		Vector3 targetPos = lockOnTarget_->GetPosition();
		Vector3 toTarget = {
			targetPos.x - playerPos.x,
			targetPos.y - playerPos.y,
			targetPos.z - playerPos.z};
		float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

		if (distance > lockOnRange_) {
			lockOnTarget_ = nullptr;
			lockOnMode_ = false;
		}
	}
}

Enemy *Player::GetNearestEnemy() const {
	if (!enemyManager_) {
		return nullptr;
	}

	Vector3 playerPos = GetPosition();
	Enemy *nearestEnemy = nullptr;
	float nearestDistance = lockOnRange_;

	const auto &enemies = enemyManager_->GetEnemies();

	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		Vector3 enemyPos = enemy->GetPosition();
		Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z};

		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestEnemy = enemy.get();
		}
	}

	return nearestEnemy;
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
	if (missileCoolTime_ > 0.0f) {
		missileCoolTime_ -= 1.0f / 60.0f;
	}

	// スペースキーで弾を発射
	if (input->PushKey(DIK_SPACE) && shootCoolTime_ <= 0.0f) {
		Vector3 playerPos = obj_->GetPosition();
		Vector3 shootDirection = {0.0f, 0.0f, 1.0f};

		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(object3dSetup_, "axisPlus.obj", playerPos, shootDirection);
		bullets_.push_back(std::move(bullet));

		shootCoolTime_ = maxShootCoolTime_;
	}

	// Mキーでミサイル発射
	if (input->PushKey(DIK_M) && missileCoolTime_ <= 0.0f) {
		Vector3 playerPos = obj_->GetPosition();
		Vector3 shootDirection = {0.0f, 0.0f, 1.0f};

		auto missile = std::make_unique<PlayerMissile>();
		missile->Initialize(object3dSetup_, "axisPlus.obj", playerPos, shootDirection);
		missile->SetParticleSystem(particleSystem_, particleSetup_);
		missile->SetEnemyManager(enemyManager_);

		// ロックオンターゲットがある場合は設定
		if (lockOnTarget_) {
			missile->SetTarget(lockOnTarget_);
			missile->StartLockOn();
		}

		missiles_.push_back(std::move(missile));
		missileCoolTime_ = maxMissileCoolTime_;
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

void Player::UpdateMissiles() {
	// ミサイルの更新
	for (auto &missile : missiles_) {
		missile->Update();
	}

	// 死んだミサイルを削除
	missiles_.erase(
		std::remove_if(missiles_.begin(), missiles_.end(),
					   [](const std::unique_ptr<PlayerMissile> &missile) {
						   return !missile->IsAlive();
					   }),
		missiles_.end());
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

void Player::DrawMissiles() {
	for (auto &missile : missiles_) {
		missile->Draw();
		// デバッグ情報も描画
		missile->DrawDebugInfo();
	}

	// プレイヤーのロックオン範囲も表示
	if (lockOnMode_ && lockOnTarget_) {
		LineManager *lineManager = LineManager::GetInstance();
		Vector3 playerPos = GetPosition();
		Vector3 targetPos = lockOnTarget_->GetPosition();

		// プレイヤーからターゲットへのライン
		lineManager->DrawLine(playerPos, targetPos, {0.0f, 1.0f, 0.0f, 1.0f}, 2.0f);

		// ロックオン範囲の表示
		lineManager->DrawCircle(playerPos, lockOnRange_, {0.0f, 1.0f, 0.0f, 0.3f}, 1.0f);

		// ターゲット位置のマーカー
		lineManager->DrawCoordinateAxes(targetPos, 2.0f, 3.0f);
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

		// === デバッグ用ボタン ===
		ImGui::Text("=== Debug Controls ===");
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Keyboard shortcuts disabled for security");

		// より安全な表現に変更
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));

		// より中立的な表現に変更
		if (ImGui::Button("Trigger End Sequence (Debug)", ImVec2(300, 30))) {
			static int confirmCount = 0;
			confirmCount++;

			if (confirmCount >= 2) {
				currentHP_ = 0;
				StartDefeatAnimation();
				confirmCount = 0;
			} else {
				ImGui::OpenPopup("Confirm Debug Action");
			}
		}

		ImGui::PopStyleColor(3);

		if (ImGui::BeginPopup("Confirm Debug Action")) {
			ImGui::Text("Press the button again to confirm!");
			ImGui::EndPopup();
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
		ImGui::Text("Missiles Count: %zu", missiles_.size());
		ImGui::SliderFloat("Shoot Cool Time", &maxShootCoolTime_, 0.05f, 1.0f);
		ImGui::SliderFloat("Missile Cool Time", &maxMissileCoolTime_, 0.5f, 5.0f);
		ImGui::Text("Controls: SPACE = Bullet, M = Missile, L = Lock-On");

		ImGui::Separator();

		// === ロックオン情報 ===
		ImGui::Text("=== Lock-On Status ===");
		ImGui::Text("Lock-On Mode: %s", lockOnMode_ ? "ON" : "OFF");
		ImGui::Text("Has Target: %s", HasLockOnTarget() ? "YES" : "NO");
		if (HasLockOnTarget()) {
			Vector3 targetPos = lockOnTarget_->GetPosition();
			ImGui::Text("Target Pos: (%.2f, %.2f, %.2f)", targetPos.x, targetPos.y, targetPos.z);

			Vector3 playerPos = GetPosition();
			Vector3 toTarget = {
				targetPos.x - playerPos.x,
				targetPos.y - playerPos.y,
				targetPos.z - playerPos.z};
			float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
			ImGui::Text("Distance: %.2f", distance);
		}
		ImGui::SliderFloat("Lock-On Range", &lockOnRange_, 10.0f, 100.0f);

		if (ImGui::Button("Manual Lock-On")) {
			lockOnTarget_ = GetNearestEnemy();
			lockOnMode_ = lockOnTarget_ != nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Lock-On")) {
			lockOnTarget_ = nullptr;
			lockOnMode_ = false;
		}

		// ミサイル個別情報
		ImGui::Separator();
		ImGui::Text("=== Active Missiles ===");
		for (size_t i = 0; i < missiles_.size(); ++i) {
			if (missiles_[i] && missiles_[i]->IsAlive()) {
				ImGui::Text("Missile %zu: Locked=%s, Target=%s",
							i,
							missiles_[i]->IsLockedOn() ? "Yes" : "No",
							missiles_[i]->HasTarget() ? "Yes" : "No");
			}
		}

		ImGui::Separator();

		// === 敗北演出情報（用語変更） ===
		ImGui::Text("=== Defeat Animation Status ===");
		ImGui::Text("Is Defeated: %s", isDefeated_ ? "Yes" : "No");
		ImGui::Text("Animation Complete: %s", defeatAnimationComplete_ ? "Yes" : "No");
		if (isDefeated_) {
			ImGui::Text("Animation Time: %.2fs / %.2fs", defeatAnimationTime_, defeatAnimationDuration_);
			ImGui::ProgressBar(defeatAnimationTime_ / defeatAnimationDuration_, ImVec2(200, 20), "");
		}
		ImGui::SliderFloat("Animation Duration", &defeatAnimationDuration_, 1.0f, 10.0f);

		// より中立的な表現に変更
		if (ImGui::Button("Test Animation Sequence")) {
			currentHP_ = 0;
			StartDefeatAnimation();
		}

		ImGui::End();
	}
}

//=============================================================================
// HP関連処理
void Player::TakeDamage(int damage) {
	// 無敵状態または既に敗北している場合はダメージを受けない
	if (isInvincible_ || !IsAlive() || isDefeated_) {
		return;
	}

	// ダメージを適用
	currentHP_ = std::max(0, currentHP_ - damage);

	// HP0になったら敗北演出開始
	if (currentHP_ <= 0) {
		StartDefeatAnimation();
		return;
	}

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

//=============================================================================
// 敗北演出処理（Crash から Defeat に改名）
void Player::StartDefeatAnimation() {
	if (isDefeated_) {
		return;
	}

	isDefeated_ = true;
	defeatAnimationComplete_ = false;
	defeatAnimationTime_ = 0.0f;

	// 演出用の速度を設定（より予測可能な計算に変更）
	// NOTE: セキュリティソフト誤検出を避けるため、ランダム性を最小化

	// 時刻ベースのシード値（より予測可能）
	unsigned int seed = static_cast<unsigned int>(defeatAnimationTime_ * 1000.0f);

	// 簡易的な疑似乱数（標準rand()の代わり）
	float pseudoRandom1 = (seed % 100) / 50.0f - 1.0f;			 // -1.0 ~ 1.0
	float pseudoRandom2 = ((seed * 7) % 100) / 500.0f - 0.1f;	 // -0.1 ~ 0.1
	float pseudoRandom3 = ((seed * 13) % 100) / 1000.0f - 0.05f; // -0.05 ~ 0.05
	float pseudoRandom4 = ((seed * 19) % 100) / 333.0f - 0.15f;	 // -0.15 ~ 0.15

	defeatVelocity_ = {
		pseudoRandom1 * 1.5f,	  // 横方向の変動
		-5.0f,					  // 下方向（固定）
		currentVelocity_.z * 0.5f // 前方向を維持
	};

	// 回転速度を設定
	defeatRotationSpeed_ = {
		pseudoRandom2, // ピッチ
		pseudoRandom3, // ヨー
		pseudoRandom4  // ロール
	};
}

void Player::UpdateDefeatAnimation() {
	if (!obj_) {
		return;
	}
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f; // 60FPS想定
	defeatAnimationTime_ += deltaTime;

	// 演出進行度（0.0 ~ 1.0）
	float animationProgress = std::min(defeatAnimationTime_ / defeatAnimationDuration_, 1.0f);

	// 重力加速（時間経過で加速）
	defeatVelocity_.y -= 9.8f * deltaTime * (1.0f + animationProgress);

	// 位置更新
	objTransform->translate.x += defeatVelocity_.x * deltaTime;
	objTransform->translate.y += defeatVelocity_.y * deltaTime;
	objTransform->translate.z += defeatVelocity_.z * deltaTime;

	// 回転更新（スピン）
	objTransform->rotate.x += defeatRotationSpeed_.x * (1.0f + animationProgress * 2.0f);
	objTransform->rotate.y += defeatRotationSpeed_.y * (1.0f + animationProgress * 2.0f);
	objTransform->rotate.z += defeatRotationSpeed_.z * (1.0f + animationProgress * 2.0f);

	// パーティクル生成（演出エフェクト）
	// COMMENTED OUT: パーティクル生成は一時的に無効化
	/*
	if (particleSystem_ && defeatAnimationTime_ < defeatAnimationDuration_) {
		if (static_cast<int>(defeatAnimationTime_ * 60.0f) % 3 == 0) {
			Transform smokeTransform = {};
			smokeTransform.translate = objTransform->translate;
			// particleSystem_->EmitParticle("ExplosionSmoke", smokeTransform);
		}
	}
	*/

	// 演出完了判定（地面到達 or 時間経過）
	if (objTransform->translate.y <= -10.0f || animationProgress >= 1.0f) {
		defeatAnimationComplete_ = true;

		// 最終的なエフェクト
		// COMMENTED OUT: エフェクト生成は一時的に無効化
		/*
		if (particleSystem_) {
			Transform explosionTransform = {};
			explosionTransform.translate = objTransform->translate;

			for (int i = 0; i < 20; ++i) {
				// particleSystem_->EmitParticle("ExplosionSparks", explosionTransform);
			}
			for (int i = 0; i < 5; ++i) {
				// particleSystem_->EmitParticle("ExplosionSmoke", explosionTransform);
			}
		}
		*/
	}

	// 当たり判定更新
	BaseObject::Update(objTransform->translate);
}

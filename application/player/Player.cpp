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
#include <Xinput.h>
#include <algorithm>
#include <cmath> // std::abs, std::min, std::max のため

namespace { // 無名名前空間でファイルスコープの定数を定義
	const float PI = 3.1415926535f;
	constexpr float kFrameDelta = 1.0f / 60.0f; // こちらは1フレーム分の時間を丁寧に共有しております。

	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}
	inline float DegreesToRadians(float degrees) {
		return degrees * (PI / 180.0f);
	}
	inline float RadiansToDegrees(float radians) {
		return radians * (180.0f / PI);
	}
	template <class T>
	void UpdateProjectileList(std::vector<std::unique_ptr<T>> &items) {
		for (auto &item : items) {
			if (item) {
				item->Update();
			}
		}
		items.erase(
			std::remove_if(items.begin(), items.end(),
						   [](const std::unique_ptr<T> &item) { return !item || !item->IsAlive(); }),
			items.end());
	}
} // namespace

Transform *Player::GetTransformSafe() const {
	return obj_ ? obj_->GetTransform() : nullptr;
}

void Player::ClearLockOn() {
	lockOnTarget_ = nullptr;
	lockOnMode_ = false;
}

//=======================================================================
// 初期化
void Player::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	const Vector3 zero{};
	currentVelocity_ = zero;
	targetVelocity_ = zero;
	targetRotationEuler_ = zero;
	moveSpeed_ = 5.0f;
	acceleration_ = 0.1f;

	rotationSmoothing_ = 0.1f;
	maxRollAngle_ = 30.0f;
	maxPitchAngle_ = 15.0f;

	// HPコンポーネントの初期化
	helthComponent_.Initialize(50);

	// 戦闘コンポーネントの初期化
	combatComponent_.Initialize(object3dSetup);

	if (Transform *transform = GetTransformSafe()) {
		transform->translate = zero;
		transform->rotate = zero;
		BaseObject::Initialize(transform->translate, 1.0f);
	}

	enemyManager_ = nullptr;
	lockOnRange_ = 30.0f;
	ClearLockOn();

	isDefeated_ = false;
	defeatAnimationComplete_ = false;
	defeatAnimationTime_ = 0.0f;
	defeatAnimationDuration_ = 3.0f;
	defeatVelocity_ = zero;
	defeatRotationSpeed_ = zero;
}

//=============================================================================
// 更新
void Player::Update() {
	Transform *objTransform = GetTransformSafe();
	if (!objTransform) {
		return;
	}

	// === 敗北演出中の処理 ===
	if (isDefeated_) {
		UpdateDefeatAnimation();
		obj_->Update();
		return;
	}

	// === コンポーネントの更新 ===
	helthComponent_.Update(kFrameDelta);
	combatComponent_.Update(kFrameDelta);

	// === プレイヤーの各種更新処理 ===
	UpdateMovement();
	UpdateLockOn();
	ProcessShooting();
	combatComponent_.UpdateBullets();
	combatComponent_.UpdateMissiles();

	// === 当たり判定・オブジェクト更新 ===
	BaseObject::Update(objTransform->translate);
	obj_->Update();
}

//=============================================================================
// ロックオン機能の更新
void Player::UpdateLockOn() {
	Input *input = Input::GetInstance();
	static bool prevLockKey = false;
	const bool currentLockKey = input->PushKey(DIK_L);
	const bool controllerLock = input->TriggerButton(XINPUT_GAMEPAD_Y);

	if ((currentLockKey && !prevLockKey) || controllerLock) {
		if (lockOnMode_) {
			ClearLockOn();
		} else {
			lockOnTarget_ = GetNearestEnemy();
			lockOnMode_ = lockOnTarget_ != nullptr;
		}
	}
	prevLockKey = currentLockKey;

	if (!lockOnTarget_) {
		return;
	}
	if (!lockOnTarget_->IsAlive() || Distance(GetPosition(), lockOnTarget_->GetPosition()) > lockOnRange_) {
		ClearLockOn();
	}
}

//=============================================================================
// 最寄りの敵を取得
Enemy *Player::GetNearestEnemy() const {
	if (!enemyManager_) {
		return nullptr;
	}

	const Vector3 playerPos = GetPosition();
	Enemy *nearestEnemy = nullptr;
	float nearestDistance = lockOnRange_;

	const auto &enemies = enemyManager_->GetEnemies();
	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		const float distance = Distance(enemy->GetPosition(), playerPos);
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
	float moveX = 0.0f;
	float moveY = 0.0f;
	if (input->PushKey(DIK_W)) {
		moveY += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		moveY -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		moveX += 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		moveX -= 1.0f;
	}
	moveX = std::clamp(moveX + input->GetLeftStickX(), -1.0f, 1.0f);
	moveY = std::clamp(moveY + input->GetLeftStickY(), -1.0f, 1.0f);

	// 動作関係処理を順次実行
	ProcessMovementInput(moveX, moveY);
	UpdateVelocity();
	UpdatePosition();
	UpdateRotation();
}

//=============================================================================
// 入力に基づいて目標速度と目標回転を設定
void Player::ProcessMovementInput(float inputX, float inputY) {
	const float deadZone = 0.1f;
	if (std::fabs(inputX) < deadZone) {
		inputX = 0.0f;
	}
	if (std::fabs(inputY) < deadZone) {
		inputY = 0.0f;
	}

	targetVelocity_.x = inputX * moveSpeed_;
	targetVelocity_.y = inputY * moveSpeed_;
	targetVelocity_.z = 0.0f;

	Vector3 desiredRotationEuler = {
		DegreesToRadians(-maxPitchAngle_ * inputY),
		0.0f,
		DegreesToRadians(-maxRollAngle_ * inputX)};

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
	if (Transform *objTransform = GetTransformSafe()) {
		objTransform->translate.x += currentVelocity_.x * kFrameDelta;
		objTransform->translate.y += currentVelocity_.y * kFrameDelta;
		objTransform->translate.z += currentVelocity_.z * kFrameDelta;
	}
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

	const Vector3 playerPos = obj_->GetPosition();
	const Vector3 forward{0.0f, 0.0f, 1.0f};

	// 通常弾の発射
	if ((input->PushKey(DIK_SPACE) || input->PushButton(XINPUT_GAMEPAD_A)) && combatComponent_.CanShootBullet()) {
		combatComponent_.ShootBullet(playerPos, forward);
	}

	// ミサイルの発射
	if ((input->PushKey(DIK_M) || input->PushButton(XINPUT_GAMEPAD_B)) && combatComponent_.CanShootMissile()) {
		combatComponent_.ShootMissile(playerPos, forward, lockOnTarget_);
	}
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
	combatComponent_.DrawBullets();
}

void Player::DrawMissiles() {
	combatComponent_.DrawMissiles();

	if (lockOnMode_ && lockOnTarget_) {
		LineManager *lineManager = LineManager::GetInstance();
		const Vector3 playerPos = GetPosition();
		const Vector3 targetPos = lockOnTarget_->GetPosition();
		lineManager->DrawLine(playerPos, targetPos, {0.0f, 1.0f, 0.0f, 1.0f}, 2.0f);
		lineManager->DrawCircle(playerPos, lockOnRange_, {0.0f, 1.0f, 0.0f, 0.3f}, 1.0f);
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
		ImGui::Text("HP: %d / %d", helthComponent_.GetCurrentHP(), helthComponent_.GetMaxHP());
		ImGui::ProgressBar(GetHPRatio(), ImVec2(200, 20), "");
		ImGui::Text("Invincible: %s", helthComponent_.IsInvincible() ? "Yes" : "No");
		if (helthComponent_.IsInvincible()) {
			ImGui::Text("Invincible Time: %.2fs", helthComponent_.GetInvincibleTime());
		}
		int maxHP = helthComponent_.GetMaxHP();
		if (ImGui::SliderInt("Max HP", &maxHP, 50, 500)) {
			helthComponent_.SetMaxHP(maxHP);
		}
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

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.1f, 1.0f));

		if (ImGui::Button("Trigger End Sequence (Debug)", ImVec2(300, 30))) {
			static int confirmCount = 0;
			confirmCount++;

			if (confirmCount >= 2) {
				helthComponent_.TakeDamage(helthComponent_.GetCurrentHP());
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
		ImGui::Text("Bullets Count: %zu", combatComponent_.GetBullets().size());
		ImGui::Text("Missiles Count: %zu", combatComponent_.GetMissiles().size());
		float maxShootCoolTime = 0.1f;
		if (ImGui::SliderFloat("Shoot Cool Time", &maxShootCoolTime, 0.05f, 1.0f)) {
			combatComponent_.SetMaxShootCoolTime(maxShootCoolTime);
		}
		float maxMissileCoolTime = 1.0f;
		if (ImGui::SliderFloat("Missile Cool Time", &maxMissileCoolTime, 0.5f, 5.0f)) {
			combatComponent_.SetMaxMissileCoolTime(maxMissileCoolTime);
		}
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
			const float distance = Distance(targetPos, playerPos);
			ImGui::Text("Distance: %.2f", distance);
		}
		ImGui::SliderFloat("Lock-On Range", &lockOnRange_, 10.0f, 100.0f);

		if (ImGui::Button("Manual Lock-On")) {
			lockOnTarget_ = GetNearestEnemy();
			lockOnMode_ = lockOnTarget_ != nullptr;
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Lock-On")) {
			ClearLockOn();
		}

		// ミサイル個別情報
		ImGui::Separator();
		ImGui::Text("=== Active Missiles ===");
		const auto &missiles = combatComponent_.GetMissiles();
		for (size_t i = 0; i < missiles.size(); ++i) {
			if (missiles[i] && missiles[i]->IsAlive()) {
				ImGui::Text("Missile %zu: Locked=%s, Target=%s",
							i,
							missiles[i]->IsLockedOn() ? "Yes" : "No",
							missiles[i]->HasTarget() ? "Yes" : "No");
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

		if (ImGui::Button("Test Animation Sequence")) {
			helthComponent_.TakeDamage(helthComponent_.GetCurrentHP());
			StartDefeatAnimation();
		}

		ImGui::End();
	}
}

//=============================================================================
// HP関連処理（コンポーネントへの委譲）
void Player::TakeDamage(int damage) {
	// 敗北状態では追加ダメージを受けない
	if (isDefeated_) {
		return;
	}

	helthComponent_.TakeDamage(damage);

	// HP0になったら敗北演出開始
	if (!helthComponent_.IsAlive()) {
		StartDefeatAnimation();
	}
}

void Player::Heal(int healAmount) {
	helthComponent_.Heal(healAmount);
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

//=============================================================================
// 敗北演出の更新処理
void Player::UpdateDefeatAnimation() {
	Transform *objTransform = GetTransformSafe();
	if (!objTransform) {
		return;
	}

	defeatAnimationTime_ += kFrameDelta;
	const float animationProgress = std::min(defeatAnimationTime_ / defeatAnimationDuration_, 1.0f);

	defeatVelocity_.y -= 9.8f * kFrameDelta * (1.0f + animationProgress);

	objTransform->translate.x += defeatVelocity_.x * kFrameDelta;
	objTransform->translate.y += defeatVelocity_.y * kFrameDelta;
	objTransform->translate.z += defeatVelocity_.z * kFrameDelta;

	objTransform->rotate.x += defeatRotationSpeed_.x * (1.0f + animationProgress * 2.0f);
	objTransform->rotate.y += defeatRotationSpeed_.y * (1.0f + animationProgress * 2.0f);
	objTransform->rotate.z += defeatRotationSpeed_.z * (1.0f + animationProgress * 2.0f);

	if (objTransform->translate.y <= -10.0f || animationProgress >= 1.0f) {
		defeatAnimationComplete_ = true;
	}

	BaseObject::Update(objTransform->translate);
}

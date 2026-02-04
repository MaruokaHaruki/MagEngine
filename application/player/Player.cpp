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
#include "EnemyBase.h"
#include "EnemyBullet.h"
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "ModelManager.h"
#include "Object3d.h"
#include <Xinput.h>
#include <algorithm>
#include <cmath> // std::abs, std::min, std::max のため
using namespace MagEngine;

namespace { // 無名名前空間でファイルスコープの定数を定義
	constexpr float kFrameDelta = 1.0f / 60.0f;
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
void Player::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	// 移動コンポーネントの初期化
	movementComponent_.Initialize();

	// HPコンポーネントの初期化
	healthComponent_.Initialize(100);

	// 戦闘コンポーネントの初期化
	combatComponent_.Initialize(object3dSetup);

	const Vector3 zero{};
	if (Transform *transform = GetTransformSafe()) {
		transform->translate = zero;
		transform->rotate = zero;
		BaseObject::Initialize(transform->translate, 1.0f);
	}

	enemyManager_ = nullptr;
	lockOnRange_ = 100.0f;
	lockOnFOV_ = 30.0f; // 視野角60度（左右30度ずつ）
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
	healthComponent_.Update(kFrameDelta);
	combatComponent_.Update(kFrameDelta);

	// === プレイヤーの各種更新処理 ===
	UpdateMovement();
	UpdateBarrelRollAndBoost(); // 追加
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
EnemyBase *Player::GetNearestEnemy() const { // Enemy* から EnemyBase* に変更
	if (!enemyManager_) {
		return nullptr;
	}

	const Vector3 playerPos = GetPosition();
	const Vector3 playerForward = GetForwardVector();
	EnemyBase *nearestEnemy = nullptr; // Enemy* から EnemyBase* に変更
	float bestScore = -1.0f;

	const auto &enemies = enemyManager_->GetEnemies();
	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		const Vector3 enemyPos = enemy->GetPosition();
		const Vector3 toEnemy = {
			enemyPos.x - playerPos.x,
			enemyPos.y - playerPos.y,
			enemyPos.z - playerPos.z};

		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		// ロックオン範囲外はスキップ
		if (distance > lockOnRange_) {
			continue;
		}

		// 進行方向スコア（前方ほど高い）
		float normalizedX = toEnemy.x / (distance + 0.001f);
		float normalizedY = toEnemy.y / (distance + 0.001f);
		float normalizedZ = toEnemy.z / (distance + 0.001f);

		float dotProduct = normalizedX * playerForward.x +
						   normalizedY * playerForward.y +
						   normalizedZ * playerForward.z;

		// 視野角チェック（コーン形範囲内か）
		float fovRadians = lockOnFOV_ * 0.5f * MagMath::PI / 180.0f;
		float angleRadians = std::acos(std::max(-1.0f, std::min(1.0f, dotProduct)));
		if (angleRadians > fovRadians) {
			// 視野外の敵はスキップ
			continue;
		}

		// スコア = 前方への角度（高いほど前方）- 距離ペナルティ
		float score = dotProduct - (distance / lockOnRange_) * 0.3f;

		if (score > bestScore) {
			bestScore = score;
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

	// キーボード入力
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

	// コントローラー入力（左スティック）
	moveX = std::clamp(moveX + input->GetLeftStickX(), -1.0f, 1.0f);
	moveY = std::clamp(moveY + input->GetLeftStickY(), -1.0f, 1.0f);

	// 移動コンポーネントで処理
	movementComponent_.ProcessInput(moveX, moveY);
	movementComponent_.Update(GetTransformSafe(), kFrameDelta);
}

//=============================================================================
// バレルロールとブーストの更新
void Player::UpdateBarrelRollAndBoost() {
	Input *input = Input::GetInstance();

	// === バレルロール入力（方向キー+Shift または 左スティック+Aボタン） ===
	static float directionKeyHoldTime = 0.0f; // 方向キーを押している時間
	static bool leftKeyHeld = false;
	static bool rightKeyHeld = false;
	static bool wasBarrelRolling = false; // 前フレームでバレルロール中だったか

	bool barrelRollTriggered = false;
	bool barrelRollRight = false;

	// === キーボード入力 ===
	bool currentLeftKey = input->PushKey(DIK_LEFT);
	bool currentRightKey = input->PushKey(DIK_RIGHT);
	bool shiftPressed = input->PushKey(DIK_LSHIFT);
	bool shiftTriggered = input->TriggerKey(DIK_LSHIFT);

	// 方向キーの状態を更新
	if (currentLeftKey || currentRightKey) {
		directionKeyHoldTime += kFrameDelta;
		leftKeyHeld = currentLeftKey;
		rightKeyHeld = currentRightKey;
	} else {
		directionKeyHoldTime = 0.0f;
		leftKeyHeld = false;
		rightKeyHeld = false;
	}

	// バレルロール判定（キーボード）
	if (!movementComponent_.IsBarrelRolling()) {
		// パターン1: 方向キー+Shiftの同時押し
		if (shiftPressed && (input->TriggerKey(DIK_LEFT) || input->TriggerKey(DIK_RIGHT))) {
			barrelRollTriggered = true;
			barrelRollRight = input->TriggerKey(DIK_RIGHT);
		}
		// パターン2: 方向キーを押した後にShiftトリガー（0.5秒以内）
		else if (shiftTriggered && directionKeyHoldTime > 0.0f && directionKeyHoldTime < 0.5f) {
			barrelRollTriggered = true;
			barrelRollRight = rightKeyHeld;
		}
	}

	// === コントローラー入力 ===
	bool aButtonPressed = input->PushButton(XINPUT_GAMEPAD_A);
	bool aButtonTriggered = input->TriggerButton(XINPUT_GAMEPAD_A);
	float stickX = input->GetLeftStickX();

	static float stickDirectionHoldTime = 0.0f;
	static bool stickLeftHeld = false;
	static bool stickRightHeld = false;

	// スティックの方向状態を更新
	bool currentStickTilted = std::abs(stickX) > 0.5f;

	if (currentStickTilted) {
		stickDirectionHoldTime += kFrameDelta;
		stickLeftHeld = stickX < -0.5f;
		stickRightHeld = stickX > 0.5f;
	} else {
		stickDirectionHoldTime = 0.0f;
		stickLeftHeld = false;
		stickRightHeld = false;
	}

	// コントローラーでのバレルロール判定
	if (!movementComponent_.IsBarrelRolling() && !barrelRollTriggered) {
		// パターン1: Aボタン押した瞬間にスティックが倒されている
		if (aButtonTriggered && currentStickTilted) {
			barrelRollTriggered = true;
			barrelRollRight = stickX > 0.0f;
		}
		// パターン2: スティックを倒した後にAボタン（0.5秒以内）
		else if (aButtonTriggered && stickDirectionHoldTime > 0.0f && stickDirectionHoldTime < 0.5f) {
			barrelRollTriggered = true;
			barrelRollRight = stickRightHeld;
		}
	}

	// バレルロール実行
	if (barrelRollTriggered && movementComponent_.CanBarrelRoll()) {
		movementComponent_.StartBarrelRoll(barrelRollRight);
		healthComponent_.SetBarrelRollInvincible(true);
		// 入力状態をリセット
		directionKeyHoldTime = 0.0f;
		stickDirectionHoldTime = 0.0f;
	}

	// バレルロール終了時に無敵解除
	bool currentlyBarrelRolling = movementComponent_.IsBarrelRolling();
	if (wasBarrelRolling && !currentlyBarrelRolling) {
		healthComponent_.SetBarrelRollInvincible(false);
	}
	wasBarrelRolling = currentlyBarrelRolling;

	// === ブースト入力（Shift長押し または Aボタン長押し） ===
	bool boostInput = false;

	// バレルロール中はブースト不可
	if (!currentlyBarrelRolling) {
		// キーボード：Shift長押し（方向キー不要）
		if (shiftPressed) {
			boostInput = true;
		}
		// コントローラー：Aボタン長押し（スティック方向不問）
		else if (aButtonPressed) {
			boostInput = true;
		}
	}

	movementComponent_.ProcessBoost(boostInput, kFrameDelta);
}

//=============================================================================
// 弾の発射処理
void Player::ProcessShooting() {
	Input *input = Input::GetInstance();

	const Vector3 playerPos = obj_->GetPosition();
	const Vector3 forward{0.0f, 0.0f, 1.0f};

	// トリガー入力閾値
	constexpr float kTriggerThreshold = 0.3f;

	// マシンガン発射（キーボード：SPACE または コントローラー：Rトリガー）
	bool shootBullet = input->PushKey(DIK_SPACE) || input->GetRightTrigger() > kTriggerThreshold;
	if (shootBullet && combatComponent_.CanShootBullet()) {
		combatComponent_.ShootBullet(playerPos, forward);
	}

	// ミサイル発射（キーボード：M または コントローラー：Lトリガー/Bボタン）
	bool shootMissile = input->PushKey(DIK_M) || input->GetLeftTrigger() > kTriggerThreshold ||
						input->TriggerButton(XINPUT_GAMEPAD_B);

	if (shootMissile && combatComponent_.CanShootMissile()) {
		// ロックオン対象がある場合はそれをターゲット、ない場合は最寄りの敵を探す
		EnemyBase *targetEnemy = lockOnTarget_ ? lockOnTarget_ : GetNearestEnemy();
		combatComponent_.ShootMissile(playerPos, forward, targetEnemy);
	}
}

//=============================================================================
// 描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}

#ifdef _DEBUG
	// ロックオン範囲の描画（コーン形）
	LineManager *lineManager = LineManager::GetInstance();
	Vector3 playerPos = GetPosition();
	Vector3 playerForward = GetForwardVector();

	// コーン形のロックオン範囲を描画
	Vector4 rangeColor = lockOnMode_ && lockOnTarget_ ? Vector4{1.0f, 0.0f, 0.0f, 0.4f} : // ロックオン中は赤
							 Vector4{0.0f, 1.0f, 1.0f, 0.2f};							  // 待機中はシアン

	// コーン形を描画：正面に向かう円錐
	float fovRadians = lockOnFOV_ * 0.5f * MagMath::PI / 180.0f; // 視野角をラジアンに
	int circleSegments = 16;

	// コーン底面の円を描画
	for (int i = 0; i < circleSegments; ++i) {
		float angle1 = (2.0f * MagMath::PI / circleSegments) * i;
		float angle2 = (2.0f * MagMath::PI / circleSegments) * (i + 1);

		// 円の半径を計算（視野角とロックオン距離から）
		float coneRadius = lockOnRange_ * std::tan(fovRadians);

		// 右ベクトルと上ベクトルを計算
		Vector3 right = {playerForward.z, 0.0f, -playerForward.x};
		float rightLen = std::sqrt(right.x * right.x + right.z * right.z);
		if (rightLen > 0.001f) {
			right.x /= rightLen;
			right.z /= rightLen;
		} else {
			right = {1.0f, 0.0f, 0.0f};
		}

		Vector3 up = {0.0f, 1.0f, 0.0f};

		// 円周上の2点
		Vector3 p1 = playerPos + playerForward * lockOnRange_ +
					 right * std::cos(angle1) * coneRadius +
					 up * std::sin(angle1) * coneRadius;
		Vector3 p2 = playerPos + playerForward * lockOnRange_ +
					 right * std::cos(angle2) * coneRadius +
					 up * std::sin(angle2) * coneRadius;

		// 底面の円の辺
		lineManager->DrawLine(p1, p2, rangeColor, 1.0f);

		// コーンの側面
		lineManager->DrawLine(playerPos, p1, rangeColor, 0.5f);
	}

	// ロックオン中のターゲットマーカー
	if (lockOnTarget_ && lockOnTarget_->IsAlive()) {
		Vector3 targetPos = lockOnTarget_->GetPosition();
		lineManager->DrawSphere(targetPos, 2.0f, {1.0f, 0.0f, 0.0f, 0.8f}, 16, 2.0f); // 赤い円

		// プレイヤーからターゲットへのライン
		lineManager->DrawLine(playerPos, targetPos, {1.0f, 0.5f, 0.0f, 0.8f}, 2.0f);
	}
#endif
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
		ImGui::Text("HP: %d / %d", healthComponent_.GetCurrentHP(), healthComponent_.GetMaxHP());
		ImGui::ProgressBar(GetHPRatio(), ImVec2(200, 20), "");
		ImGui::Text("Invincible: %s", healthComponent_.IsInvincible() ? "Yes" : "No");
		if (healthComponent_.IsInvincible()) {
			ImGui::Text("Invincible Time: %.2fs", healthComponent_.GetInvincibleTime());
		}
		int maxHP = healthComponent_.GetMaxHP();
		if (ImGui::SliderInt("Max HP", &maxHP, 50, 500)) {
			healthComponent_.SetMaxHP(maxHP);
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
				healthComponent_.TakeDamage(healthComponent_.GetCurrentHP());
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
		const Vector3 &velocity = movementComponent_.GetCurrentVelocity();
		ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);
		ImGui::Text(
			"Rotation (Deg): (%.1f, %.1f, %.1f)",
			RadiansToDegrees(objTransform->rotate.x),
			RadiansToDegrees(objTransform->rotate.y),
			RadiansToDegrees(objTransform->rotate.z));

		// === 移動パラメータ調整 ===
		ImGui::Text("=== Movement Parameters ===");
		float moveSpeed = movementComponent_.GetMoveSpeed();
		if (ImGui::SliderFloat("Move Speed", &moveSpeed, 1.0f, 20.0f)) {
			movementComponent_.SetMoveSpeed(moveSpeed);
		}
		float acceleration = movementComponent_.GetAcceleration();
		if (ImGui::SliderFloat("Acceleration", &acceleration, 0.01f, 0.5f)) {
			movementComponent_.SetAcceleration(acceleration);
		}
		float maxRollAngle = 30.0f;
		if (ImGui::SliderFloat("Max Roll (Deg)", &maxRollAngle, 5.0f, 90.0f)) {
			movementComponent_.SetMaxRollAngle(maxRollAngle);
		}
		float maxPitchAngle = 15.0f;
		if (ImGui::SliderFloat("Max Pitch (Deg)", &maxPitchAngle, 5.0f, 45.0f)) {
			movementComponent_.SetMaxPitchAngle(maxPitchAngle);
		}
		float rotationSmoothing = 0.1f;
		if (ImGui::SliderFloat("Rotation Smoothing", &rotationSmoothing, 0.01f, 0.5f)) {
			movementComponent_.SetRotationSmoothing(rotationSmoothing);
		}

		ImGui::Separator();

		// === ブーストゲージ情報 ===
		ImGui::Text("=== Boost Gauge ===");
		ImGui::Text("Boost: %.1f / %.1f", movementComponent_.GetBoostGauge(), movementComponent_.GetMaxBoostGauge());
		ImGui::ProgressBar(movementComponent_.GetBoostGaugeRatio(), ImVec2(200, 20), "");
		ImGui::Text("Boosting: %s", movementComponent_.IsBoosting() ? "Yes" : "No");
		ImGui::Text("Can Boost: %s", movementComponent_.CanBoost() ? "Yes" : "No");
		float boostSpeed = 2.0f;
		if (ImGui::SliderFloat("Boost Speed", &boostSpeed, 1.5f, 3.0f)) {
			movementComponent_.SetBoostSpeed(boostSpeed);
		}
		float boostConsumption = 30.0f;
		if (ImGui::SliderFloat("Boost Consumption", &boostConsumption, 10.0f, 50.0f)) {
			movementComponent_.SetBoostConsumption(boostConsumption);
		}
		float boostRecovery = 15.0f;
		if (ImGui::SliderFloat("Boost Recovery", &boostRecovery, 5.0f, 30.0f)) {
			movementComponent_.SetBoostRecovery(boostRecovery);
		}

		ImGui::Separator();

		// === バレルロール情報 ===
		ImGui::Text("=== Barrel Roll ===");
		ImGui::Text("Is Rolling: %s", movementComponent_.IsBarrelRolling() ? "Yes" : "No");
		ImGui::Text("Can Roll: %s", movementComponent_.CanBarrelRoll() ? "Yes" : "No");
		if (movementComponent_.IsBarrelRolling()) {
			ImGui::ProgressBar(movementComponent_.GetBarrelRollProgress(), ImVec2(200, 20), "");
		}
		float rollDuration = 0.8f;
		if (ImGui::SliderFloat("Roll Duration", &rollDuration, 0.3f, 2.0f)) {
			movementComponent_.SetBarrelRollDuration(rollDuration);
		}
		float rollCooldown = 1.5f;
		if (ImGui::SliderFloat("Roll Cooldown", &rollCooldown, 0.5f, 3.0f)) {
			movementComponent_.SetBarrelRollCooldown(rollCooldown);
		}
		float rollCost = 30.0f;
		if (ImGui::SliderFloat("Roll Cost", &rollCost, 10.0f, 50.0f)) {
			movementComponent_.SetBarrelRollCost(rollCost);
		}
		ImGui::Text("Controls:");
		ImGui::Text("  Keyboard: Arrow(hold) + Shift = Roll, Shift Hold = Boost");
		ImGui::Text("  Controller: L-Stick(hold) + A = Roll/Boost");
		ImGui::Text("  Note: Direction key/stick can be pressed before Roll button");

		ImGui::Separator();

		// === ロックオン情報 ===
		ImGui::Text("=== Lock-On System ===");
		ImGui::Text("Lock-On Mode: %s", lockOnMode_ ? "ACTIVE" : "INACTIVE");
		ImGui::Text("Lock-On Range: %.1f", lockOnRange_);
		if (ImGui::SliderFloat("Lock-On Range (Slider)", &lockOnRange_, 10.0f, 100.0f)) {
			// ロックオン範囲を動的に変更可能
		}
		ImGui::Text("Lock-On FOV: %.1f degrees", lockOnFOV_);
		if (ImGui::SliderFloat("Lock-On FOV (Slider)", &lockOnFOV_, 30.0f, 180.0f)) {
			// ロックオン視野角を動的に変更可能
		}
		if (lockOnTarget_ && lockOnTarget_->IsAlive()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target Locked!");
			Vector3 targetPos = lockOnTarget_->GetPosition();
			ImGui::Text("Target Position: (%.2f, %.2f, %.2f)", targetPos.x, targetPos.y, targetPos.z);
			float distToTarget = Distance(GetPosition(), targetPos);
			ImGui::Text("Distance to Target: %.2f", distToTarget);
		} else {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No Target");
		}

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
		ImGui::Text("Controls:");
		ImGui::Text("  Keyboard: SPACE = Gun, M = Missile, L = Lock-On");
		ImGui::Text("  Controller: R-Trigger = Gun, L-Trigger = Missile, Y = Lock-On");

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
			healthComponent_.TakeDamage(healthComponent_.GetCurrentHP());
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

	healthComponent_.TakeDamage(damage);

	// HP0になったら敗北演出開始
	if (!healthComponent_.IsAlive()) {
		StartDefeatAnimation();
	}
}

void Player::Heal(int healAmount) {
	healthComponent_.Heal(healAmount);
}

//=======================================================================
// 衝突処理
void Player::OnCollisionEnter(BaseObject *other) {
	// 敵の弾との衝突チェック
	if (dynamic_cast<EnemyBullet *>(other)) {
		TakeDamage(15); // 敵の弾のダメージ
		return;
	}

	// 敵本体との衝突時にダメージを受ける
	if (dynamic_cast<EnemyBase *>(other)) {
		TakeDamage(10); // 衝突時のダメージ量
	}
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

	// 演出用の速度を設定（疑似ランダム）
	unsigned int seed = static_cast<unsigned int>(defeatAnimationTime_ * 1000.0f);

	float pseudoRandom1 = (seed % 100) / 50.0f - 1.0f;
	float pseudoRandom2 = ((seed * 7) % 100) / 500.0f - 0.1f;
	float pseudoRandom3 = ((seed * 13) % 100) / 1000.0f - 0.05f;
	float pseudoRandom4 = ((seed * 19) % 100) / 333.0f - 0.15f;

	const Vector3 &currentVelocity = movementComponent_.GetCurrentVelocity();
	defeatVelocity_ = {
		pseudoRandom1 * 1.5f,
		-5.0f,
		currentVelocity.z * 0.5f};

	defeatRotationSpeed_ = {
		pseudoRandom2,
		pseudoRandom3,
		pseudoRandom4};
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

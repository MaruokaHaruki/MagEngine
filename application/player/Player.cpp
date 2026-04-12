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
#include "input.h"
#include <algorithm>
#include <cmath> // std::abs, std::min, std::max のため
using namespace MagEngine;

namespace { // 無名名前空間でファイルスコープの定数を定義
	constexpr float kFrameDelta = 1.0f / 60.0f;
} // namespace

MagMath::Transform *Player::GetTransformSafe() const {
	return obj_ ? obj_->GetTransform() : nullptr;
}

//=======================================================================
// 初期化
//=======================================================================
/// 責務:
/// - 3Dオブジェクト（ビジュアル）の生成と初期化
/// - 全コンポーネント（移動・HP・射撃・ロックオン・敗北演出）の初期化
void Player::Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	// === 各コンポーネント初期化 ===
	movementComponent_.Initialize();
	healthComponent_.Initialize(100);
	combatComponent_.Initialize(object3dSetup);
	lockedOnComponent_.Initialize(nullptr);
	justAvoidanceComponent_.Initialize();
	defeatComponent_.Initialize();
	gameOverAnimation_.Initialize(nullptr); // SpriteSetupはシーン側から設定

	// === ミサイルボタン長押し管理の初期化 ===
	missileButtonHeldTime_ = 0.0f;
	isInLockOnMode_ = false;
	prevMissileButtonPressed_ = false;

	// === 武装設定をコンポーネントに適用 ===
	ApplyWeaponConfigToCombatComponent();

	// === Transform初期化 ===
	const Vector3 zero{};
	if (MagMath::Transform *transform = GetTransformSafe()) {
		transform->translate = zero;
		transform->rotate = zero;
		BaseObject::Initialize(transform->translate, 1.0f);
	}

	// === システム参照初期化 ===
	enemyManager_ = nullptr;
}

//=============================================================================
// 毎フレーム更新
//=============================================================================
/// 責勑:
/// - 蓮北機能を除く各コンポーネントを更新
/// - 入力処理、移動、回転を处理
/// 責務: 各コンポーネントを統合更新する
void Player::Update() {
	MagMath::Transform *objTransform = GetTransformSafe();
	if (!objTransform) {
		return;
	}

	// === ジャスト回避コンポーネントは常に実時間で更新（タイマーカウント用） ===
	justAvoidanceComponent_.Update(kFrameDelta);

	// === スロー倍率を計算し、他のコンポーネントに適用 ===
	float slowMultiplier = justAvoidanceComponent_.GetGameTimeScale();
	float effectiveDeltaTime = kFrameDelta * slowMultiplier;

	// === 強化バフを適用 ===
	// 移動速度倍率をMovementコンポーネントに反映
	// スロー状態でも速度が維持されるように、speedMultiplierを乗算
	float speedMultiplier = justAvoidanceComponent_.GetSpeedMultiplier();
	if (speedMultiplier > 1.0f) {
		// 速度倍率がある場合、deltaTimeを調整して速度を上げる
		effectiveDeltaTime *= speedMultiplier;
	}

	// === コンポーネント更新（スロー適用） ===
	healthComponent_.Update(effectiveDeltaTime);
	combatComponent_.Update(effectiveDeltaTime);

	// === プレイヤー移動関連処理 ===
	UpdateMovement(effectiveDeltaTime);
	UpdateBarrelRollAndBoost(effectiveDeltaTime);

	// === 射撃処理 ===
	ProcessShooting();
	combatComponent_.UpdateBullets();
	combatComponent_.UpdateMissiles();

	// === 敗北演出（敗北中のみ） ===
	if (defeatComponent_.IsDefeated()) {
		defeatComponent_.Update(objTransform, kFrameDelta);
	}

	// === ゲームオーバー演出の更新 ===
	gameOverAnimation_.Update();

	// === 基本的なオブジェクト更新（敗北演出中以外） ===
	if (!defeatComponent_.IsDefeated()) {
		BaseObject::Update(objTransform->translate);
	}
	obj_->Update();

	// === ジャスト回避成功フラグをリセット（毎フレーム末尾） ===
	// NOTE: このフラグは衝突検出時に設定され、1フレームだけ有効である必要がある
	justAvoidanceSuccessThisFrame_ = false;
}

//=============================================================================
// プレイヤーの動作関係処理（入力処理、移動、回転を統合）
void Player::UpdateMovement(float deltaTime) {
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
	movementComponent_.Update(GetTransformSafe(), deltaTime);
}

//=============================================================================
// バレルロールとブーストの更新
void Player::UpdateBarrelRollAndBoost(float deltaTime) {
	Input *input = Input::GetInstance();

	// === バレルロール入力（Shift または Aボタンのみ） ===
	static bool wasBarrelRolling = false; // 前フレームでバレルロール中だったか

	bool barrelRollTriggered = false;

	// === キーボード入力 ===
	bool shiftTriggered = input->TriggerKey(DIK_LSHIFT);

	// バレルロール判定（キーボード）
	if (!movementComponent_.IsBarrelRolling()) {
		// Shiftキートリガーで回避発動
		if (shiftTriggered) {
			barrelRollTriggered = true;
		}
	}

	// === コントローラー入力 ===
	bool aButtonTriggered = input->TriggerButton(XINPUT_GAMEPAD_A);

	// コントローラーでのバレルロール判定
	if (!movementComponent_.IsBarrelRolling() && !barrelRollTriggered) {
		// Aボタントリガーで回避発動
		if (aButtonTriggered) {
			barrelRollTriggered = true;
		}
	}

	// バレルロール実行（移動状況に応じた適応的回避）
	if (barrelRollTriggered && movementComponent_.CanBarrelRoll()) {
		movementComponent_.StartAdaptiveBarrelRoll();
		healthComponent_.SetBarrelRollInvincible(true);
		justAvoidanceComponent_.OnBarrelRollStarted();
	}

	// バレルロール終了時に無敵解除
	bool currentlyBarrelRolling = movementComponent_.IsBarrelRolling();
	if (wasBarrelRolling && !currentlyBarrelRolling) {
		healthComponent_.SetBarrelRollInvincible(false);
	}
	wasBarrelRolling = currentlyBarrelRolling;

	// === ブースト入力（Shift長押し または Aボタン長押し） ===
	bool boostInput = false;
	bool shiftPressed = input->PushKey(DIK_LSHIFT);
	bool aButtonPressed = input->PushButton(XINPUT_GAMEPAD_A);

	// バレルロール中はブースト不可
	if (!currentlyBarrelRolling) {
		// キーボード：Shift長押し
		if (shiftPressed) {
			boostInput = true;
		}
		// コントローラー：Aボタン長押し
		else if (aButtonPressed) {
			boostInput = true;
		}
	}

	movementComponent_.ProcessBoost(boostInput, deltaTime);
}

//=============================================================================
// 弾の発射処理
void Player::ProcessShooting() {
	Input *input = Input::GetInstance();

	const Vector3 playerPos = obj_->GetPosition();
	// プレイヤーの正面は常にZ軸正方向（カメラから見てプレイヤーの前方）
	const Vector3 shootDirection = {0.0f, 0.0f, 1.0f};

	// トリガー入力閾値
	constexpr float kTriggerThreshold = 0.3f;

	// マシンガン発射（キーボード：SPACE または コントローラー：Rトリガー）
	bool shootBullet = input->PushKey(DIK_SPACE) || input->GetRightTrigger() > kTriggerThreshold;
	if (shootBullet && combatComponent_.CanShootBullet()) {
		combatComponent_.ShootBullet(playerPos, shootDirection);
	}

	// ===== ミサイル長押しロジック（新仕様） =====
	// ミサイルボタン入力判定（キーボード：M または コントローラー：Lトリガー/Bボタン）
	bool missileButtonPressed = input->PushKey(DIK_M) || input->GetLeftTrigger() > kTriggerThreshold ||
								input->PushButton(XINPUT_GAMEPAD_B);
	bool missileButtonTriggered = missileButtonPressed && !prevMissileButtonPressed_;

	// ボタンが離された（前フレーム押下 → 今フレーム未押）
	bool missileButtonReleased = prevMissileButtonPressed_ && !missileButtonPressed;

	// 押下開始時にロックオンモードへ遷移
	if (missileButtonTriggered && combatComponent_.CanShootMissile()) {
		isInLockOnMode_ = true;
		missileButtonHeldTime_ = 0.0f;
		lockedOnComponent_.BeginLockOn();
	}

	// ボタン長押し中
	if (missileButtonPressed && isInLockOnMode_) {
		missileButtonHeldTime_ += kFrameDelta;
		lockedOnComponent_.UpdateLockOn(playerPos, shootDirection, kFrameDelta);
	}
	// ボタン離した（リリース）
	if (missileButtonReleased) {
		// ロックオン中の敵にミサイル発射
		const auto &lockOnTargets = lockedOnComponent_.GetAllTargets();
		if (isInLockOnMode_ && !lockOnTargets.empty()) {
			// 複数ロック敵に同時発射
			combatComponent_.ShootMultipleMissiles(playerPos, shootDirection, lockOnTargets);
		}

		// ロックオンモード終了
		lockedOnComponent_.EndLockOn();
		isInLockOnMode_ = false;
		lockedOnComponent_.ClearLockOn();
		missileButtonHeldTime_ = 0.0f;
	}

	// 前フレームの状態を保存
	prevMissileButtonPressed_ = missileButtonPressed;
}

//=============================================================================
// 描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}

#ifdef _DEBUG
	// ロックオン範囲の描画（コンポーネント情報を使用）
	LineManager *lineManager = LineManager::GetInstance();
	Vector3 playerPos = GetPosition();
	Vector3 playerForward = GetForwardVector();

	// コーン形のロックオン範囲を描画
	bool hasLockOn = lockedOnComponent_.HasLockOnTarget();
	Vector4 rangeColor = hasLockOn ? Vector4{1.0f, 0.0f, 0.0f, 0.4f} : // ロックオン中は赤
							 Vector4{0.0f, 1.0f, 1.0f, 0.2f};		   // 待機中はシアン

	// コーン形を描画：正面に向かう円錐
	float fovRadians = lockedOnComponent_.GetLockOnFOV() * 0.5f * MagMath::PI / 180.0f; // 視野角をラジアンに
	int circleSegments = 16;

	// コーン底面の円を描画
	for (int i = 0; i < circleSegments; ++i) {
		float angle1 = (2.0f * MagMath::PI / circleSegments) * i;
		float angle2 = (2.0f * MagMath::PI / circleSegments) * (i + 1);

		// 円の半径を計算（視野角とロックオン距離から）
		float coneRadius = lockedOnComponent_.GetLockOnRange() * std::tan(fovRadians);

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
		Vector3 p1 = playerPos + playerForward * lockedOnComponent_.GetLockOnRange() +
					 right * std::cos(angle1) * coneRadius +
					 up * std::sin(angle1) * coneRadius;
		Vector3 p2 = playerPos + playerForward * lockedOnComponent_.GetLockOnRange() +
					 right * std::cos(angle2) * coneRadius +
					 up * std::sin(angle2) * coneRadius;

		// 底面の円の辺
		lineManager->DrawLine(p1, p2, rangeColor, 1.0f);

		// コーンの側面
		lineManager->DrawLine(playerPos, p1, rangeColor, 0.5f);
	}

	// ロックオン中のターゲットマーカー（削除）
	// NOTE: デバッグ線描の簡潔化により削除
#endif
}

//=============================================================================
// 弾の描画
void Player::DrawBullets() {
	combatComponent_.DrawBullets();
}

void Player::DrawMissiles() {
	combatComponent_.DrawMissiles();
}

void Player::DrawBulletsTrails() {
	combatComponent_.DrawBulletsTrails();
}

void Player::DrawMissilesTrails() {
	combatComponent_.DrawMissilesTrails();
}

//=============================================================================
// ImGui描画
void Player::DrawImGui() {
	if (!obj_) {
		return;
	}
	MagMath::Transform *objTransform = obj_->GetTransform();
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
				defeatComponent_.StartDefeatAnimation();
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
		ImGui::Text("  Keyboard: Shift(Press) = Roll, Shift(Hold) = Boost");
		ImGui::Text("  Controller: A(Press) = Roll, A(Hold) = Boost");
		ImGui::Text("  Note: Roll adapts to movement - Stationary = Spin, Moving = Dash");

		ImGui::Separator();

		// === ロックオン情報（コンポーネント委譲） ===
		ImGui::Text("=== Lock-On System ===");
		ImGui::Text("Lock-On Mode: %s", lockedOnComponent_.HasLockOnTarget() ? "ACTIVE" : "INACTIVE");
		float lockOnRange = lockedOnComponent_.GetLockOnRange();
		ImGui::Text("Lock-On Range: %.1f", lockOnRange);
		if (ImGui::SliderFloat("Lock-On Range (Slider)", &lockOnRange, 10.0f, 100.0f)) {
			lockedOnComponent_.SetLockOnRange(lockOnRange);
		}
		float lockOnFOV = lockedOnComponent_.GetLockOnFOV();
		ImGui::Text("Lock-On FOV: %.1f degrees", lockOnFOV);
		if (ImGui::SliderFloat("Lock-On FOV (Slider)", &lockOnFOV, 30.0f, 180.0f)) {
			lockedOnComponent_.SetLockOnFOV(lockOnFOV);
		}

		EnemyBase *primaryTarget = lockedOnComponent_.GetPrimaryTarget();
		if (primaryTarget && primaryTarget->IsAlive()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target Locked!");
			Vector3 targetPos = primaryTarget->GetPosition();
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
		int maxMissileAmmo = combatComponent_.GetMaxMissileAmmo();
		if (ImGui::SliderInt("Max Missile Ammo", &maxMissileAmmo, 1, 10)) {
			combatComponent_.SetMaxMissileAmmo(maxMissileAmmo);
		}
		float missileRecoveryTime = combatComponent_.GetMissileRecoveryTime();
		if (ImGui::SliderFloat("Missile Recovery Time (sec)", &missileRecoveryTime, 0.5f, 10.0f)) {
			combatComponent_.SetMissileRecoveryTime(missileRecoveryTime);
		}
		ImGui::Text("Missile Ammo: %d / %d", combatComponent_.GetMissileAmmo(), combatComponent_.GetMaxMissileAmmo());
		ImGui::Text("Controls:");
		ImGui::Text("  Keyboard: SPACE = Gun, Hold M = Lock, Release M = Fire");
		ImGui::Text("  Controller: R-Trigger = Gun, Hold LT/B = Lock, Release = Fire");

		// === ロックオン情報（マルチロック） ===
		ImGui::Text("=== Auto Lock-On Status ===");
		ImGui::Text("Lock-On Mode: %s", isInLockOnMode_ ? "ACTIVE" : "INACTIVE");
		ImGui::Text("Locked Enemies: %zu / %d", lockedOnComponent_.GetTargetCount(), lockedOnComponent_.GetMaxLockOnTargets());

		// ロック敵の詳細情報
		const auto &allTargets = lockedOnComponent_.GetAllTargets();
		if (!allTargets.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Locked Targets:");
			Vector3 playerPos = GetPosition();
			for (size_t i = 0; i < allTargets.size(); ++i) {
				if (allTargets[i]) {
					Vector3 targetPos = allTargets[i]->GetPosition();
					float dist = Distance(playerPos, targetPos);
					ImGui::Text("  [%zu] Addr: %p, Dist: %.2f", i, (void *)allTargets[i], dist);
				}
			}
		}

		if (HasLockOnTarget()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Primary Target: Locked");
			Vector3 targetPos = primaryTarget->GetPosition();
			ImGui::Text("Target Pos: (%.2f, %.2f, %.2f)", targetPos.x, targetPos.y, targetPos.z);

			Vector3 playerPos = GetPosition();
			const float distance = Distance(targetPos, playerPos);
			ImGui::Text("Distance: %.2f", distance);

			// ロック敵一覧
			if (allTargets.size() > 1) {
				ImGui::Text("Other Locked Targets:");
				for (size_t i = 1; i < allTargets.size(); ++i) {
					Vector3 pos = allTargets[i]->GetPosition();
					float dist = Distance(playerPos, pos);
					ImGui::Text("  Enemy %zu: Dist=%.2f", i, dist);
				}
			}
		} else {
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No targets in range");
		}

		// ミサイル個別情報
		ImGui::Separator();
		ImGui::Text("=== Active Missiles ===");
		const auto &missiles = combatComponent_.GetMissiles();
		for (size_t i = 0; i < missiles.size(); ++i) {
			if (missiles[i] && missiles[i]->IsAlive()) {
				EnemyBase *targetEnemy = missiles[i]->GetLockedTarget();
				ImGui::Text("Missile %zu: Locked=%s, Target=%s, TargetAddr=%p",
							i,
							missiles[i]->IsLockedOn() ? "Yes" : "No",
							missiles[i]->HasTarget() ? "Yes" : "No",
							(void *)targetEnemy);
			}
		}

		ImGui::Separator();

		// === 敗北演出情報 ===
		ImGui::Text("=== Defeat Animation Status ===");
		ImGui::Text("Is Defeated: %s", defeatComponent_.IsDefeated() ? "Yes" : "No");
		ImGui::Text("Animation Complete: %s", defeatComponent_.IsDefeatAnimationComplete() ? "Yes" : "No");
		if (defeatComponent_.IsDefeated()) {
			ImGui::Text("Animation Progress: %.2f%%", defeatComponent_.GetAnimationProgress() * 100.0f);
			ImGui::ProgressBar(defeatComponent_.GetAnimationProgress(), ImVec2(200, 20), "");
		}

		if (ImGui::Button("Test Animation Sequence")) {
			healthComponent_.TakeDamage(healthComponent_.GetCurrentHP());
			defeatComponent_.StartDefeatAnimation();
		}

		ImGui::End();
	}
}

//=============================================================================
// HP関連処理（コンポーネントへの委譲）
void Player::TakeDamage(int damage) {
	// 敗北状態では追加ダメージを受けない
	if (defeatComponent_.IsDefeated()) {
		return;
	}

	healthComponent_.TakeDamage(damage);

	// HP0になったら敗北演出開始
	if (!healthComponent_.IsAlive()) {
		defeatComponent_.StartDefeatAnimation();
	}
}

void Player::Heal(int healAmount) {
	healthComponent_.Heal(healAmount);
}

//=======================================================================
// 衝突処理
void Player::OnCollisionEnter(BaseObject *other) {
	// 敵の弾との衝突チェック
	if (EnemyBullet *bullet = dynamic_cast<EnemyBullet *>(other)) {
		//! ===== ジャスト回避判定 =====
		//! 敵弾がプレイヤーに接近中か、距離に基づいて判定
		justAvoidanceComponent_.RegisterIncomingBullet(
			bullet->GetPosition(),
			bullet->GetPreviousPosition(),
			bullet->GetRadius(),
			this->GetPosition()  // プレイヤーの現在位置を渡す
		);
		
		// ジャスト回避成功判定
		bool wasJustAvoidanceSuccess = false;
		float successRate = 0.0f;
		float slowStrength = justAvoidanceComponent_.CheckJustAvoidanceSuccess(
			wasJustAvoidanceSuccess,
			successRate
		);
		
		if (wasJustAvoidanceSuccess) {
			//! ===== ジャスト回避成功 =====
			//! - ダメージを受けずにボーストゲージ回復（30.0f）
			//! - スロー演出開始
			//! - 機体強化バフ開始
			//! 詳細は PlayerJustAvoidanceComponent で管理
			movementComponent_.AddBoostGaugeReward(30.0f);
			
			// 演出用フラグを設定
			justAvoidanceSuccessThisFrame_ = true;
			lastJustAvoidanceSuccessRate_ = successRate;
			
		} else {
			//! ===== ジャスト回避失敗 =====
			//! 通常通りダメージを受ける
			TakeDamage(15); // 敵の弾のダメージ
		}
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

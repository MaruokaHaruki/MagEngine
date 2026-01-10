/*********************************************************************
 * \file   PlayerMissile.cpp
 * \brief  プレイヤーミサイルクラス - 追尾機能付きミサイル
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   リアリティのあるミサイル動作（推進力、慣性、追尾）
 *********************************************************************/
#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "PlayerMissile.h"
#include "EnemyManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "ModelManager.h"
#include "Object3d.h"
#include <algorithm>
#include <cmath>

namespace {
	inline Vector3 NormalizeVector(const Vector3 &v) {
		float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (length < 0.001f)
			return {0.0f, 0.0f, 1.0f};
		return {v.x / length, v.y / length, v.z / length};
	}

	inline float DotProduct(const Vector3 &a, const Vector3 &b) { // Dotから名前変更
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	inline Vector3 Cross(const Vector3 &a, const Vector3 &b) {
		return {
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x};
	}
}

//=============================================================================
// 初期化
void PlayerMissile::Initialize(
	Object3dSetup *object3dSetup,
	const std::string &modelPath,
	const Vector3 &position,
	const Vector3 &direction,
	EnemyBase *target) { // Enemy* から EnemyBase* に変更
	//========================================
	// コア初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	//========================================
	// 物理パラメータ初期化（シンプルな一定速度）
	forward_ = NormalizeVector(direction);
	speed_ = 50.0f;		   // 一定速度
	maxTurnRate_ = 120.0f; // 最大旋回速度（度/秒）- 急カーブを防ぐ

	velocity_ = {
		forward_.x * speed_,
		forward_.y * speed_,
		forward_.z * speed_};
	acceleration_ = {0.0f, 0.0f, 0.0f};

	//========================================
	// 追尾パラメータ初期化（段階的な追尾）
	target_ = nullptr;
	lockedTarget_ = nullptr;
	trackingStrength_ = 0.0f; // 初期は追尾なし
	lockOnRange_ = 30.0f;
	trackingStartTime_ = 0.5f; // 0.5秒後から追尾開始
	isTracking_ = false;
	isLockedOn_ = false;
	lockOnTime_ = 0.0f;
	enemyManager_ = nullptr;

	//========================================
	// 回転関連初期化
	targetRotation_ = {0.0f, 0.0f, 0.0f};
	currentRotation_ = {0.0f, 0.0f, 0.0f};
	rotationSpeed_ = 8.0f;

	//========================================
	// 寿命関連初期化
	lifetime_ = 0.0f;
	maxLifetime_ = 10.0f;
	isAlive_ = true;

	//========================================
	// オブジェクト設定
	if (obj_) {
		Transform *objTransform = obj_->GetTransform();
		if (objTransform) {
			objTransform->translate = position;
			objTransform->rotate = {0.0f, 0.0f, 0.0f};
			objTransform->scale = {0.5f, 0.5f, 0.5f};

			Vector3 pos = position;
			BaseObject::Initialize(pos, 1.0f);
		}
	}

	//========================================
	// デバッグ・視覚化関連初期化
	maxTrajectoryPoints_ = 100;
	showDebugInfo_ = true;
	showTrajectory_ = true;
	showTargetLine_ = true;
	showVelocityVector_ = true;
	showForwardVector_ = true;
	trajectoryPoints_.clear();
	trajectoryPoints_.reserve(maxTrajectoryPoints_);
}

//=============================================================================
// 更新
void PlayerMissile::Update() {
	if (!isAlive_ || !obj_)
		return;

	const float deltaTime = 1.0f / 60.0f;
	lifetime_ += deltaTime;

	//========================================
	// 軌跡記録
	Vector3 currentPos = obj_->GetPosition();
	trajectoryPoints_.push_back(currentPos);

	// 軌跡点数制限
	if (trajectoryPoints_.size() > static_cast<size_t>(maxTrajectoryPoints_)) {
		trajectoryPoints_.erase(trajectoryPoints_.begin());
	}

	//========================================
	// ロックオン時間の更新
	if (isLockedOn_) {
		lockOnTime_ += deltaTime;
	}

	//========================================
	// 各種更新処理
	UpdateMovement();
	UpdateTracking();
	UpdatePhysics();
	UpdateRotation();
	UpdateLifetime();

	//========================================
	// オブジェクト更新
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		BaseObject::Update(objTransform->translate);
		obj_->Update();
	}
}

void PlayerMissile::UpdateMovement() {
	if (!obj_)
		return;

	const float deltaTime = 1.0f / 60.0f;
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform)
		return;

	//========================================
	// 一定速度を維持
	velocity_ = {
		forward_.x * speed_,
		forward_.y * speed_,
		forward_.z * speed_};

	//========================================
	// 位置更新
	objTransform->translate.x += velocity_.x * deltaTime;
	objTransform->translate.y += velocity_.y * deltaTime;
	objTransform->translate.z += velocity_.z * deltaTime;
}

void PlayerMissile::UpdateTracking() {
	const float deltaTime = 1.0f / 60.0f;

	//========================================
	// 追尾強度の段階的上昇
	if (lifetime_ < trackingStartTime_) {
		// 発射直後は追尾なし（初期方向を維持）
		trackingStrength_ = 0.0f;
		return;
	}

	// 追尾強度を時間経過で徐々に上げる（3秒かけて最大に）
	float trackingBuildupTime = 3.0f;
	float timeSinceTrackingStart = lifetime_ - trackingStartTime_;
	trackingStrength_ = std::min(timeSinceTrackingStart / trackingBuildupTime, 1.0f);

	// ロックオン時は追尾強度を強化
	if (isLockedOn_) {
		trackingStrength_ = std::min(trackingStrength_ * 1.5f, 1.0f);
	}

	//========================================
	// ターゲット選択
	if (isLockedOn_ && lockedTarget_ && lockedTarget_->IsAlive()) {
		target_ = lockedTarget_;
	} else if (!target_ || !target_->IsAlive()) {
		target_ = FindNearestTarget();
	}

	//========================================
	// ターゲット追尾処理（角度制限付き）
	if (target_ && target_->IsAlive() && trackingStrength_ > 0.01f) {
		Vector3 missilePos = obj_->GetPosition();
		Vector3 targetPos = target_->GetPosition();

		// ターゲットへの方向
		Vector3 toTarget = {
			targetPos.x - missilePos.x,
			targetPos.y - missilePos.y,
			targetPos.z - missilePos.z};

		float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

		if (distance < lockOnRange_ * 2.0f && distance > 0.001f) {
			isTracking_ = true;
			Vector3 targetDirection = NormalizeVector(toTarget);

			//========================================
			// 現在の前方向とターゲット方向の角度差を計算
			float dotProduct = DotProduct(forward_, targetDirection); // 関数名を変更
			dotProduct = std::max(-1.0f, std::min(1.0f, dotProduct)); // クランプ
			float angleToTarget = std::acos(dotProduct) * 180.0f / MagMath::PI;

			//========================================
			// 最大旋回速度による角度制限
			float maxAngleChange = maxTurnRate_ * deltaTime;

			// 角度差が大きすぎる場合は制限
			float turnRatio = 1.0f;
			if (angleToTarget > maxAngleChange) {
				turnRatio = maxAngleChange / angleToTarget;
			}

			// 追尾強度と角度制限を組み合わせる
			float effectiveStrength = trackingStrength_ * turnRatio;

			// 滑らかに前方向を更新
			forward_.x = MagMath::Lerp(forward_.x, targetDirection.x, effectiveStrength);
			forward_.y = MagMath::Lerp(forward_.y, targetDirection.y, effectiveStrength);
			forward_.z = MagMath::Lerp(forward_.z, targetDirection.z, effectiveStrength);
			forward_ = NormalizeVector(forward_);
		}
	}
}

void PlayerMissile::StartLockOn() {
	if (!enemyManager_)
		return;

	EnemyBase *nearestEnemy = FindNearestTarget(); // Enemy* から EnemyBase* に変更
	if (nearestEnemy) {
		lockedTarget_ = nearestEnemy;
		isLockedOn_ = true;
		lockOnTime_ = 0.0f;
	}
}

void PlayerMissile::UpdatePhysics() {
	// シンプルな一定速度システムでは物理演算不要
	// 速度は UpdateMovement で設定される
}

void PlayerMissile::UpdateRotation() {
	if (!obj_)
		return;
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform)
		return;

	//========================================
	// 進行方向に向けて回転
	float yaw = std::atan2(forward_.x, forward_.z);
	float horizontalDistance = std::sqrt(forward_.x * forward_.x + forward_.z * forward_.z);
	float pitch = -std::atan2(forward_.y, horizontalDistance);

	targetRotation_.y = yaw;
	targetRotation_.x = pitch;

	//========================================
	// 滑らかな回転
	const float deltaTime = 1.0f / 60.0f;
	float lerpFactor = std::min(rotationSpeed_ * deltaTime, 0.9f);

	currentRotation_.x = MagMath::Lerp(currentRotation_.x, targetRotation_.x, lerpFactor);
	currentRotation_.y = MagMath::Lerp(currentRotation_.y, targetRotation_.y, lerpFactor);

	objTransform->rotate = currentRotation_;
}

void PlayerMissile::UpdateLifetime() {
	//========================================
	// 寿命チェック
	if (lifetime_ >= maxLifetime_) {
		Explode();
	}
}

EnemyBase *PlayerMissile::FindNearestTarget() { // Enemy* から EnemyBase* に変更
	if (!enemyManager_) {
		return nullptr;
	}

	Vector3 missilePos = obj_->GetPosition();
	EnemyBase *nearestEnemy = nullptr; // Enemy* から EnemyBase* に変更
	float nearestDistance = lockOnRange_;

	// EnemyManagerから敵リストを取得
	const auto &enemies = enemyManager_->GetEnemies();

	for (const auto &enemy : enemies) {
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		Vector3 enemyPos = enemy->GetPosition();
		Vector3 toEnemy = {
			enemyPos.x - missilePos.x,
			enemyPos.y - missilePos.y,
			enemyPos.z - missilePos.z};

		float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestEnemy = enemy.get();
		}
	}

	return nearestEnemy;
}

void PlayerMissile::Explode() {
	isAlive_ = false;
	// 爆発エフェクトの実装（必要に応じて）
}

//=============================================================================
// 描画
void PlayerMissile::Draw() {
	if (obj_ && isAlive_) {
		obj_->Draw();
	}
}

void PlayerMissile::DrawDebugInfo() {
#ifdef _DEBUG
	if (!showDebugInfo_ || !obj_)
		return;

	LineManager *lineManager = LineManager::GetInstance();
	Vector3 missilePos = obj_->GetPosition();

	//========================================
	// 軌跡の描画
	if (showTrajectory_ && trajectoryPoints_.size() > 1) {
		for (size_t i = 1; i < trajectoryPoints_.size(); ++i) {
			// 軌跡の色を時間経過で変化（古い点ほど薄く）
			// 			float alpha = static_cast<float>(i) / trajectoryPoints_.size();
			// 			Vector4 trailColor = {0.0f, 1.0f, 1.0f, alpha * 0.8f}; // シアン色

			// 			lineManager->DrawLine(trajectoryPoints_[i - 1], trajectoryPoints_[i], trailColor, 2.0f);
		}
	}

	//========================================
	// 検知範囲の描画
	if (showTargetLine_) {
		// 検知範囲の球体を表示
		Vector4 detectionColor = isTracking_ ? Vector4{1.0f, 0.5f, 0.0f, 0.3f} : // 追尾中はオレンジ
									 Vector4{0.5f, 0.5f, 1.0f, 0.2f};			 // 待機中は青

		lineManager->DrawSphere(missilePos, lockOnRange_, detectionColor, 16, 1.0f);
	}

	//========================================
	// 検知した敵の表示
	if (enemyManager_) {
		const auto &enemies = enemyManager_->GetEnemies();

		for (const auto &enemy : enemies) {
			if (!enemy || !enemy->IsAlive()) {
				continue;
			}

			Vector3 enemyPos = enemy->GetPosition();
			Vector3 toEnemy = {
				enemyPos.x - missilePos.x,
				enemyPos.y - missilePos.y,
				enemyPos.z - missilePos.z};

			float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

			// 検知範囲内の敵の場合
			if (distance <= lockOnRange_) {
				// ロックオン状態に応じてマーカーを描画
				bool isCurrentTarget = (target_ == enemy.get());
				bool isLockedTarget = (lockedTarget_ == enemy.get());

				Vector4 markerColor;
				float markerSize;

				if (isLockedTarget && isLockedOn_) {
					// ロックオン中のターゲット - 赤色の大きなマーカー
					markerColor = {1.0f, 0.0f, 0.0f, 1.0f};
					markerSize = 3.0f;

					// ロックオン進行度の表示（maxLockOnTime_は削除されたので削除）
					lineManager->DrawCircle(enemyPos, markerSize * 1.5f,
											{1.0f, 0.0f, 0.0f, 1.0f}, 3.0f, {0.0f, 1.0f, 0.0f}, 16);

				} else if (isCurrentTarget) {
					// 現在追尾中のターゲット - 黄色のマーカー
					markerColor = {1.0f, 1.0f, 0.0f, 1.0f};
					markerSize = 2.5f;

				} else {
					// 検知中だが未ロックオンの敵 - 緑色の小さなマーカー
					markerColor = {0.0f, 1.0f, 0.0f, 0.8f};
					markerSize = 1.5f;
				}

				// 距離表示用のライン
				Vector4 distanceLineColor = {0.8f, 0.8f, 0.8f, 0.5f};
				lineManager->DrawLine(missilePos, enemyPos, distanceLineColor, 1.0f);

				// 敵の周りに円を描画
				lineManager->DrawCircle(enemyPos, markerSize * 0.8f, markerColor, 2.0f);
			}
		}
	}

	//========================================
	// ターゲットラインの描画（メインターゲットのみ）
	if (showTargetLine_ && target_ && target_->IsAlive()) {
		Vector3 targetPos = target_->GetPosition();
		Vector4 lineColor = isLockedOn_ ? Vector4{1.0f, 0.0f, 0.0f, 1.0f} : // ロックオン時は赤
								Vector4{1.0f, 1.0f, 0.0f, 1.0f};			// 通常追尾時は黄色

		lineManager->DrawLine(missilePos, targetPos, lineColor, 3.0f);
	}

	//========================================
	// 速度ベクトルの描画
	if (showVelocityVector_) {
		Vector3 velocityEnd = {
			missilePos.x + velocity_.x * 0.1f,
			missilePos.y + velocity_.y * 0.1f,
			missilePos.z + velocity_.z * 0.1f};
		lineManager->DrawArrow(missilePos, velocityEnd, {0.0f, 1.0f, 0.0f, 1.0f}, 0.2f, 3.0f);
	}

	//========================================
	// 前方向ベクトルの描画
	if (showForwardVector_) {
		Vector3 forwardEnd = {
			missilePos.x + forward_.x * 3.0f,
			missilePos.y + forward_.y * 3.0f,
			missilePos.z + forward_.z * 3.0f};
		lineManager->DrawArrow(missilePos, forwardEnd, {1.0f, 0.5f, 0.0f, 1.0f}, 0.15f, 4.0f);
	}

	//========================================
	// ミサイル周辺の情報表示
	// 座標軸表示
	lineManager->DrawCoordinateAxes(missilePos, 1.0f, 2.0f);

	// 当たり判定範囲表示
	lineManager->DrawSphere(missilePos, 0.3f, {1.0f, 0.0f, 1.0f, 0.5f}, 12, 1.0f);
#endif // _DEBUG
}

//=============================================================================
// ImGui描画
void PlayerMissile::DrawImGui() {
	if (!obj_)
		return;

	ImGui::Begin("Missile Debug");

	//========================================
	// 視覚化制御
	ImGui::Text("=== Visualization Controls ===");
	ImGui::Checkbox("Show Debug Info", &showDebugInfo_);
	ImGui::Checkbox("Show Trajectory", &showTrajectory_);
	ImGui::Checkbox("Show Target Detection", &showTargetLine_);
	ImGui::Checkbox("Show Velocity Vector", &showVelocityVector_);
	ImGui::Checkbox("Show Forward Vector", &showForwardVector_);
	ImGui::SliderInt("Max Trajectory Points", &maxTrajectoryPoints_, 10, 500);

	ImGui::Separator();

	//========================================
	// 検知情報
	ImGui::Text("=== Detection Status ===");
	ImGui::Text("Detection Range: %.2f", lockOnRange_);

	int detectedEnemies = 0;
	if (enemyManager_) {
		Vector3 missilePos = GetPosition();
		const auto &enemies = enemyManager_->GetEnemies();

		for (const auto &enemy : enemies) {
			if (!enemy || !enemy->IsAlive())
				continue;

			Vector3 enemyPos = enemy->GetPosition();
			Vector3 toEnemy = {
				enemyPos.x - missilePos.x,
				enemyPos.y - missilePos.y,
				enemyPos.z - missilePos.z};

			float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
			if (distance <= lockOnRange_) {
				detectedEnemies++;
			}
		}
	}

	ImGui::Text("Detected Enemies: %d", detectedEnemies);
	ImGui::Text("Current Target: %s", HasTarget() ? "YES" : "NO");
	ImGui::Text("Locked Target: %s", isLockedOn_ ? "YES" : "NO");

	ImGui::Separator();

	//========================================
	// 基本情報
	ImGui::Text("=== Basic Status ===");
	ImGui::Text("Alive: %s", isAlive_ ? "Yes" : "No");
	ImGui::Text("Lifetime: %.2f / %.2f", lifetime_, maxLifetime_);
	ImGui::ProgressBar(lifetime_ / maxLifetime_, ImVec2(200, 20), "Lifetime");

	ImGui::Separator();

	//========================================
	// 追尾情報
	ImGui::Text("=== Tracking Status ===");
	ImGui::Text("Tracking: %s", isTracking_ ? "Yes" : "No");
	ImGui::Text("Has Target: %s", HasTarget() ? "Yes" : "No");
	ImGui::Text("Locked On: %s", isLockedOn_ ? "Yes" : "No");
	ImGui::Text("Tracking Strength: %.1f%%", trackingStrength_ * 100.0f);
	ImGui::ProgressBar(trackingStrength_, ImVec2(200, 20), "Tracking Strength");

	if (HasTarget()) {
		Vector3 targetPos = target_->GetPosition();
		Vector3 missilePos = GetPosition();
		Vector3 toTarget = {
			targetPos.x - missilePos.x,
			targetPos.y - missilePos.y,
			targetPos.z - missilePos.z};
		float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

		ImGui::Text("Target Distance: %.2f", distance);
		ImGui::Text("Target Pos: (%.2f, %.2f, %.2f)", targetPos.x, targetPos.y, targetPos.z);

		// 角度情報の表示
		Vector3 targetDirection = NormalizeVector(toTarget);
		float dotProduct = DotProduct(forward_, targetDirection); // 関数名を変更
		dotProduct = std::max(-1.0f, std::min(1.0f, dotProduct));
		float angleToTarget = std::acos(dotProduct) * 180.0f / PI;
		ImGui::Text("Angle to Target: %.1f degrees", angleToTarget);
	}

	ImGui::Separator();

	//========================================
	// 移動情報
	ImGui::Text("=== Movement Status ===");
	Vector3 pos = GetPosition();
	ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
	ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
	ImGui::Text("Forward: (%.2f, %.2f, %.2f)", forward_.x, forward_.y, forward_.z);
	ImGui::Text("Speed: %.2f", speed_);

	ImGui::Text("Trajectory Points: %zu / %d", trajectoryPoints_.size(), maxTrajectoryPoints_);

	ImGui::Separator();

	//========================================
	// パラメータ調整
	ImGui::Text("=== Parameters ===");
	ImGui::SliderFloat("Speed", &speed_, 20.0f, 100.0f);
	ImGui::SliderFloat("Max Turn Rate", &maxTurnRate_, 30.0f, 300.0f);
	ImGui::SliderFloat("Lock-On Range", &lockOnRange_, 10.0f, 60.0f);
	ImGui::SliderFloat("Tracking Start Time", &trackingStartTime_, 0.0f, 2.0f);
	ImGui::SliderFloat("Rotation Speed", &rotationSpeed_, 3.0f, 15.0f);

	ImGui::Separator();

	//========================================
	// 制御ボタン
	ImGui::Text("=== Controls ===");
	if (ImGui::Button("Start Lock-On")) {
		StartLockOn();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear Lock-On")) {
		isLockedOn_ = false;
		lockedTarget_ = nullptr;
		lockOnTime_ = 0.0f;
	}
	if (ImGui::Button("Clear Trajectory")) {
		trajectoryPoints_.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Explode Now")) {
		Explode();
	}

	ImGui::End();
}

//=============================================================================
// ゲッター
Vector3 PlayerMissile::GetPosition() const {
	return obj_ ? obj_->GetPosition() : Vector3{0.0f, 0.0f, 0.0f};
}

void PlayerMissile::SetTarget(EnemyBase *target) {
	target_ = target;
}

//=============================================================================
// 衝突処理
void PlayerMissile::OnCollisionEnter(BaseObject *other) {
	// 敵との衝突で爆発（EnemyBase にキャスト）
	if (dynamic_cast<EnemyBase *>(other)) { // Enemy* から EnemyBase* に変更
		Explode();
	}
}

void PlayerMissile::OnCollisionStay(BaseObject *other) {
	// 継続衝突処理
}

void PlayerMissile::OnCollisionExit(BaseObject *other) {
	// 衝突終了処理
}

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
	const float PI = 3.14159265359f;

	inline float Lerp(float a, float b, float t) {
		return a + t * (b - a);
	}

	inline Vector3 NormalizeVector(const Vector3 &v) { // 名前を変更してエンジンのNormalize関数との衝突を回避
		float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		if (length < 0.001f)
			return {0.0f, 0.0f, 1.0f};
		return {v.x / length, v.y / length, v.z / length};
	}

	inline float Dot(const Vector3 &a, const Vector3 &b) {
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
void PlayerMissile::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath,
							   const Vector3 &startPos, const Vector3 &initialDirection) {
	//========================================
	// コア初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
	object3dSetup_ = object3dSetup;

	//========================================
	// 物理パラメータ初期化（リアルな初期値）
	velocity_ = {initialDirection.x * 3.0f, initialDirection.y * 3.0f, initialDirection.z * 3.0f}; // 初期速度を大幅に低下
	acceleration_ = {0.0f, 0.0f, 0.0f};
	forward_ = NormalizeVector(initialDirection);

	// 推進力システム初期化
	initialThrustPower_ = 5.0f; // 低い初期推進力
	maxThrustPower_ = 128.0f;	// 最大推進力
	thrustPower_ = initialThrustPower_;
	thrustAcceleration_ = 32.0f; // 推進力の立ち上がり速度
	thrustBuildupTime_ = 1.5f;	 // 1.5秒で最大推進力に到達

	// 燃料システム初期化
	fuelRemaining_ = 1.0f;	  // 満タン状態
	fuelConsumption_ = 0.08f; // 燃料消費率（秒あたり）

	// ブースター初期化
	isBoosterActive_ = true; // 発射時はブースター段階
	boosterDuration_ = 2.0f; // 2秒間のブースター
	boosterTime_ = 0.0f;

	maxSpeed_ = 128.0f; // 最大速度を増加
	drag_ = 0.01f;		// 空気抵抗を減少（高高度での動作を想定）

	//========================================
	// 追尾パラメータ初期化
	target_ = nullptr;
	lockedTarget_ = nullptr;
	trackingStrength_ = 3.0f; // 追尾強度
	lockOnRange_ = 20.0f;	  // ロックオン範囲
	trackingDelay_ = 0.3f;	  // 0.3秒後から追尾開始
	isTracking_ = false;
	isLockedOn_ = false;
	lockOnTime_ = 0.0f;
	maxLockOnTime_ = 2.0f;	 // 2秒でロックオン完了
	enemyManager_ = nullptr; // EnemyManager参照初期化

	//========================================
	// 回転関連初期化
	targetRotation_ = {0.0f, 0.0f, 0.0f};
	currentRotation_ = {0.0f, 0.0f, 0.0f};
	rotationSpeed_ = 5.0f;

	//========================================
	// 寿命関連初期化
	lifetime_ = 0.0f;
	maxLifetime_ = 8.0f; // 8秒で自爆
	isAlive_ = true;

	//========================================
	// エフェクト関連初期化
	particleSystem_ = nullptr;
	particleSetup_ = nullptr;

	//========================================
	// オブジェクト設定
	if (obj_) {
		Transform *objTransform = obj_->GetTransform();
		if (objTransform) {
			objTransform->translate = startPos;
			objTransform->rotate = {0.0f, 0.0f, 0.0f};
			objTransform->scale = {0.5f, 0.5f, 0.5f}; // ミサイルは小さめ

			// 当たり判定初期化（const参照問題を修正）
			Vector3 pos = startPos; // constを外すためにコピー
			BaseObject::Initialize(pos, 0.3f);
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

void PlayerMissile::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particleSystem_ = particle;
	particleSetup_ = particleSetup;

	if (particleSystem_) {
		// 軌跡用パーティクルグループ
		particleSystem_->CreateParticleGroup("MissileTrail", "sandWind.png", ParticleShape::Board);

		// 推進炎用パーティクルグループ
		particleSystem_->CreateParticleGroup("MissileThrust", "sandWind.png", ParticleShape::Board);

		Vector3 initialPos = obj_->GetPosition();
		Transform emitterTransform = {};
		emitterTransform.translate = initialPos;

		// 軌跡エミッター（白い煙）
		trailEmitter_ = std::make_unique<ParticleEmitter>(
			particleSystem_, "MissileTrail", emitterTransform, 2, 0.05f, true);

		// 推進炎エミッター（オレンジ色の炎）
		thrustEmitter_ = std::make_unique<ParticleEmitter>(
			particleSystem_, "MissileThrust", emitterTransform, 3, 0.03f, true);
	}
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
	UpdateTrailEffect();

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
	// 燃料消費処理
	if (fuelRemaining_ > 0.0f) {
		fuelRemaining_ -= fuelConsumption_ * deltaTime;
		fuelRemaining_ = std::max(0.0f, fuelRemaining_);
	}

	//========================================
	// ブースター段階の処理
	if (isBoosterActive_) {
		boosterTime_ += deltaTime;
		if (boosterTime_ >= boosterDuration_) {
			isBoosterActive_ = false;
		}
	}

	//========================================
	// 推進力の時間変化（リアルな加速カーブ）
	float currentThrustPower = 0.0f;

	if (fuelRemaining_ > 0.0f) {
		// 推進力の立ち上がり（指数関数的な増加）
		float thrustRatio = std::min(lifetime_ / thrustBuildupTime_, 1.0f);
		float smoothRatio = 1.0f - std::pow(1.0f - thrustRatio, 2.0f); // イーズアウト曲線

		currentThrustPower = Lerp(initialThrustPower_, maxThrustPower_, smoothRatio);

		// ブースター段階では推進力を増幅
		if (isBoosterActive_) {
			float boosterMultiplier = 1.5f - (boosterTime_ / boosterDuration_) * 0.3f; // 徐々に減衰
			currentThrustPower *= boosterMultiplier;
		}

		// 燃料残量による推進力減衰
		if (fuelRemaining_ < 0.2f) {
			float fuelRatio = fuelRemaining_ / 0.2f;
			currentThrustPower *= fuelRatio;
		}

		thrustPower_ = currentThrustPower;
	} else {
		// 燃料切れ時は慣性のみ
		thrustPower_ = 0.0f;
	}

	//========================================
	// 推進力を前方向に適用
	Vector3 thrust = {
		forward_.x * thrustPower_,
		forward_.y * thrustPower_,
		forward_.z * thrustPower_};

	acceleration_.x = thrust.x;
	acceleration_.y = thrust.y;
	acceleration_.z = thrust.z;

	//========================================
	// 位置更新
	objTransform->translate.x += velocity_.x * deltaTime;
	objTransform->translate.y += velocity_.y * deltaTime;
	objTransform->translate.z += velocity_.z * deltaTime;
}

void PlayerMissile::UpdateTracking() {
	// 追尾開始遅延チェック
	if (lifetime_ < trackingDelay_)
		return;

	//========================================
	// ロックオンターゲットが優先
	if (isLockedOn_ && lockedTarget_ && lockedTarget_->IsAlive()) {
		target_ = lockedTarget_;
	}
	// ロックオンターゲットが無効なら最寄りの敵を探す
	else if (!target_ || !target_->IsAlive()) {
		target_ = FindNearestTarget();
	}

	//========================================
	// ターゲット追尾処理
	if (target_ && target_->IsAlive()) {
		Vector3 missilePos = obj_->GetPosition();
		Vector3 targetPos = target_->GetPosition();

		// ターゲットへの方向ベクトル
		Vector3 toTarget = {
			targetPos.x - missilePos.x,
			targetPos.y - missilePos.y,
			targetPos.z - missilePos.z};

		float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

		if (distance < lockOnRange_) {
			isTracking_ = true;
			Vector3 targetDirection = NormalizeVector(toTarget);

			// ロックオン時は追尾強度を上げる
			float currentTrackingStrength = isLockedOn_ ? trackingStrength_ * 2.0f : trackingStrength_;

			// 追尾強度に応じて前方向を調整
			const float deltaTime = 1.0f / 60.0f;
			float trackingFactor = currentTrackingStrength * deltaTime;

			// ロックオン時間が長いほど追尾精度を上げる
			if (isLockedOn_) {
				float lockOnFactor = std::min(lockOnTime_ / maxLockOnTime_, 1.0f);
				trackingFactor = trackingFactor * (1.0f + lockOnFactor);
			}

			forward_.x = Lerp(forward_.x, targetDirection.x, trackingFactor);
			forward_.y = Lerp(forward_.y, targetDirection.y, trackingFactor);
			forward_.z = Lerp(forward_.z, targetDirection.z, trackingFactor);
			forward_ = NormalizeVector(forward_);
		}
	}
}

void PlayerMissile::StartLockOn() {
	if (!enemyManager_)
		return;

	// 最寄りの敵をロックオン
	Enemy *nearestEnemy = FindNearestTarget();
	if (nearestEnemy) {
		lockedTarget_ = nearestEnemy;
		isLockedOn_ = true;
		lockOnTime_ = 0.0f;
	}
}

void PlayerMissile::UpdatePhysics() {
	const float deltaTime = 1.0f / 60.0f;

	//========================================
	// 速度に加速度を適用（リアルな物理）
	velocity_.x += acceleration_.x * deltaTime;
	velocity_.y += acceleration_.y * deltaTime;
	velocity_.z += acceleration_.z * deltaTime;

	//========================================
	// 空気抵抗を適用（速度の二乗に比例）
	float currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	float dragForce = drag_ * currentSpeed * currentSpeed;

	if (currentSpeed > 0.001f) {
		Vector3 dragDirection = {
			-velocity_.x / currentSpeed,
			-velocity_.y / currentSpeed,
			-velocity_.z / currentSpeed};

		velocity_.x += dragDirection.x * dragForce * deltaTime;
		velocity_.y += dragDirection.y * dragForce * deltaTime;
		velocity_.z += dragDirection.z * dragForce * deltaTime;
	}

	//========================================
	// 最大速度制限（燃料がある場合のみ）
	if (fuelRemaining_ > 0.0f) {
		currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
		if (currentSpeed > maxSpeed_) {
			float speedRatio = maxSpeed_ / currentSpeed;
			velocity_.x *= speedRatio;
			velocity_.y *= speedRatio;
			velocity_.z *= speedRatio;
		}
	}
}

void PlayerMissile::UpdateRotation() {
	if (!obj_)
		return;
	Transform *objTransform = obj_->GetTransform();
	if (!objTransform)
		return;

	//========================================
	// 進行方向に向けて回転
	// Yaw（Y軸回転）
	float yaw = std::atan2(forward_.x, forward_.z);

	// Pitch（X軸回転）
	float horizontalDistance = std::sqrt(forward_.x * forward_.x + forward_.z * forward_.z);
	float pitch = -std::atan2(forward_.y, horizontalDistance);

	targetRotation_.y = yaw;
	targetRotation_.x = pitch;

	//========================================
	// 滑らかな回転適用
	const float deltaTime = 1.0f / 60.0f;
	float lerpFactor = rotationSpeed_ * deltaTime;

	currentRotation_.x = Lerp(currentRotation_.x, targetRotation_.x, lerpFactor);
	currentRotation_.y = Lerp(currentRotation_.y, targetRotation_.y, lerpFactor);

	objTransform->rotate = currentRotation_;
}

void PlayerMissile::UpdateLifetime() {
	//========================================
	// 寿命チェック
	if (lifetime_ >= maxLifetime_) {
		Explode();
	}
}

void PlayerMissile::UpdateTrailEffect() {
	if (!trailEmitter_ || !thrustEmitter_)
		return;

	Vector3 missilePos = obj_->GetPosition();

	//========================================
	// 推進炎の強度を燃料と推進力に応じて調整
	float thrustIntensity = (thrustPower_ / maxThrustPower_) * fuelRemaining_;

	//========================================
	// 軌跡エミッター更新（ミサイルの後方）
	Vector3 trailPos = {
		missilePos.x - forward_.x * 0.5f,
		missilePos.y - forward_.y * 0.5f,
		missilePos.z - forward_.z * 0.5f};
	trailEmitter_->SetTranslate(trailPos);
	trailEmitter_->Update();

	//========================================
	// 推進炎エミッター更新（強度に応じた距離調整）
	float thrustDistance = 0.3f + thrustIntensity * 0.8f; // 推進力に応じて炎の長さを変更
	Vector3 thrustPos = {
		missilePos.x - forward_.x * thrustDistance,
		missilePos.y - forward_.y * thrustDistance,
		missilePos.z - forward_.z * thrustDistance};
	thrustEmitter_->SetTranslate(thrustPos);

	// 燃料切れ時は推進炎を停止
	if (fuelRemaining_ > 0.0f) {
		thrustEmitter_->Update();
	}
}

Enemy *PlayerMissile::FindNearestTarget() {
	if (!enemyManager_) {
		return nullptr;
	}

	Vector3 missilePos = obj_->GetPosition();
	Enemy *nearestEnemy = nullptr;
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

		// 検知範囲の境界円を描画（水平面）
		// 		lineManager->DrawCircle(missilePos, lockOnRange_, detectionColor, 2.0f, {0.0f, 1.0f, 0.0f}, 32);
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

					// ロックオンマーカー（十字）
					// 					lineManager->DrawLine(
					// 						{enemyPos.x - markerSize, enemyPos.y, enemyPos.z},
					// 						{enemyPos.x + markerSize, enemyPos.y, enemyPos.z},
					// 						markerColor, 4.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x, enemyPos.y - markerSize, enemyPos.z},
					// 						{enemyPos.x, enemyPos.y + markerSize, enemyPos.z},
					// 						markerColor, 4.0f);

					// ロックオン進行度の表示
					float lockOnProgress = std::min(lockOnTime_ / maxLockOnTime_, 1.0f);
					lineManager->DrawCircle(enemyPos, markerSize * 1.5f,
											{1.0f, 0.0f, 0.0f, lockOnProgress}, 3.0f, {0.0f, 1.0f, 0.0f}, 16);

				} else if (isCurrentTarget) {
					// 現在追尾中のターゲット - 黄色のマーカー
					markerColor = {1.0f, 1.0f, 0.0f, 1.0f};
					markerSize = 2.5f;

					// 追尾マーカー（ダイヤモンド）
					// 					lineManager->DrawLine(
					// 						{enemyPos.x, enemyPos.y + markerSize, enemyPos.z},
					// 						{enemyPos.x + markerSize, enemyPos.y, enemyPos.z},
					// 						markerColor, 3.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x + markerSize, enemyPos.y, enemyPos.z},
					// 						{enemyPos.x, enemyPos.y - markerSize, enemyPos.z},
					// 						markerColor, 3.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x, enemyPos.y - markerSize, enemyPos.z},
					// 						{enemyPos.x - markerSize, enemyPos.y, enemyPos.z},
					// 						markerColor, 3.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x - markerSize, enemyPos.y, enemyPos.z},
					// 						{enemyPos.x, enemyPos.y + markerSize, enemyPos.z},
					// 						markerColor, 3.0f);

				} else {
					// 検知中だが未ロックオンの敵 - 緑色の小さなマーカー
					markerColor = {0.0f, 1.0f, 0.0f, 0.8f};
					markerSize = 1.5f;

					// 検知マーカー（三角）
					// 					lineManager->DrawLine(
					// 						{enemyPos.x, enemyPos.y + markerSize, enemyPos.z},
					// 						{enemyPos.x + markerSize * 0.866f, enemyPos.y - markerSize * 0.5f, enemyPos.z},
					// 						markerColor, 2.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x + markerSize * 0.866f, enemyPos.y - markerSize * 0.5f, enemyPos.z},
					// 						{enemyPos.x - markerSize * 0.866f, enemyPos.y - markerSize * 0.5f, enemyPos.z},
					// 						markerColor, 2.0f);
					// 					lineManager->DrawLine(
					// 						{enemyPos.x - markerSize * 0.866f, enemyPos.y - markerSize * 0.5f, enemyPos.z},
					// 						{enemyPos.x, enemyPos.y + markerSize, enemyPos.z},
					// 						markerColor, 2.0f);
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

	//========================================
	// 状態表示のためのテキスト情報（ミサイル上部に）
	// 	Vector3 textPos = {missilePos.x, missilePos.y + 2.0f, missilePos.z};

	// 	if (isLockedOn_) {
	// 		// ロックオン状態の表示
	// 		lineManager->DrawLine(
	// 			{textPos.x - 1.0f, textPos.y, textPos.z},
	// 			{textPos.x + 1.0f, textPos.y, textPos.z},
	// 			{1.0f, 0.0f, 0.0f, 1.0f}, 3.0f);
	// 		lineManager->DrawText3D(textPos, "LOCKED", {1.0f, 0.0f, 0.0f, 1.0f});
	// 	} else if (isTracking_) {
	// 		// 追尾状態の表示
	// 		lineManager->DrawLine(
	// 			{textPos.x - 0.8f, textPos.y, textPos.z},
	// 			{textPos.x + 0.8f, textPos.y, textPos.z},
	// 			{1.0f, 1.0f, 0.0f, 1.0f}, 2.0f);
	// 		lineManager->DrawText3D(textPos, "TRACKING", {1.0f, 1.0f, 0.0f, 1.0f});
	// 	} else {
	// 		// 検索状態の表示
	// 		lineManager->DrawLine(
	// 			{textPos.x - 0.6f, textPos.y, textPos.z},
	// 			{textPos.x + 0.6f, textPos.y, textPos.z},
	// 			{0.0f, 1.0f, 0.0f, 1.0f}, 1.0f);
	// 		lineManager->DrawText3D(textPos, "SEARCH", {0.0f, 1.0f, 0.0f, 1.0f});
	// 	}
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

	// 検知した敵の数をカウント
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
	}

	if (isLockedOn_) {
		ImGui::Text("Lock-On Time: %.2f / %.2f", lockOnTime_, maxLockOnTime_);
		ImGui::ProgressBar(lockOnTime_ / maxLockOnTime_, ImVec2(200, 20), "Lock-On");
	}

	ImGui::Separator();

	//========================================
	// 位置・移動情報
	ImGui::Text("=== Movement Status ===");
	Vector3 pos = GetPosition();
	ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
	ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
	ImGui::Text("Forward: (%.2f, %.2f, %.2f)", forward_.x, forward_.y, forward_.z);

	float currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
	ImGui::Text("Current Speed: %.2f / %.2f", currentSpeed, maxSpeed_);
	ImGui::ProgressBar(currentSpeed / maxSpeed_, ImVec2(200, 20), "Speed");

	// 軌跡情報
	ImGui::Text("Trajectory Points: %zu / %d", trajectoryPoints_.size(), maxTrajectoryPoints_);

	ImGui::Separator();

	//========================================
	// 推進力システム情報
	ImGui::Text("=== Propulsion System ===");
	ImGui::Text("Current Thrust: %.2f / %.2f", thrustPower_, maxThrustPower_);
	ImGui::ProgressBar(thrustPower_ / maxThrustPower_, ImVec2(200, 20), "Thrust Power");

	ImGui::Text("Fuel Remaining: %.1f%%", fuelRemaining_ * 100.0f);
	ImGui::ProgressBar(fuelRemaining_, ImVec2(200, 20), "Fuel");

	ImGui::Text("Booster Active: %s", isBoosterActive_ ? "YES" : "NO");
	if (isBoosterActive_) {
		ImGui::Text("Booster Time: %.2f / %.2f", boosterTime_, boosterDuration_);
		ImGui::ProgressBar(boosterTime_ / boosterDuration_, ImVec2(200, 20), "Booster");
	}

	// 推進力立ち上がり進行度
	float thrustBuildupProgress = std::min(lifetime_ / thrustBuildupTime_, 1.0f);
	ImGui::Text("Thrust Buildup: %.1f%%", thrustBuildupProgress * 100.0f);
	ImGui::ProgressBar(thrustBuildupProgress, ImVec2(200, 20), "Thrust Buildup");

	ImGui::Separator();

	//========================================
	// パラメータ調整
	ImGui::Text("=== Propulsion Parameters ===");
	ImGui::SliderFloat("Initial Thrust", &initialThrustPower_, 1.0f, 15.0f);
	ImGui::SliderFloat("Max Thrust", &maxThrustPower_, 20.0f, 60.0f);
	ImGui::SliderFloat("Thrust Buildup Time", &thrustBuildupTime_, 0.5f, 3.0f);
	ImGui::SliderFloat("Fuel Consumption", &fuelConsumption_, 0.02f, 0.2f);
	ImGui::SliderFloat("Booster Duration", &boosterDuration_, 1.0f, 5.0f);

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

void PlayerMissile::SetTarget(Enemy *target) {
	target_ = target;
}

//=============================================================================
// 衝突処理
void PlayerMissile::OnCollisionEnter(BaseObject *other) {
	// 敵との衝突で爆発
	if (dynamic_cast<Enemy *>(other)) {
		Explode();
	}
}

void PlayerMissile::OnCollisionStay(BaseObject *other) {
	// 継続衝突処理
}

void PlayerMissile::OnCollisionExit(BaseObject *other) {
	// 衝突終了処理
}

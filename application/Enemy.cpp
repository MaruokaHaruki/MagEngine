#include "Enemy.h"
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include "ParticleEmitter.h"
#include <cmath>
///=============================================================================
///                        初期化
void Enemy::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	//========================================
	// 3Dオブジェクトの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	//========================================
	// メイントランスフォームの初期設定
	transform_.translate = position;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f}; // プレイヤーの方向を向く（180度回転）

	// Object3dのトランスフォームに反映
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}

	//========================================
	// 移動パラメータの設定
	speed_ = 0.0f;					   // 敵の移動速度
	velocity_ = {0.0f, 0.0f, -speed_}; // プレイヤーに向かって移動（Z軸負方向）
	rotationSpeed_ = 1.0f;			   // 回転速度（ラジアン/秒）
	hasTarget_ = false;				   // 目標位置なし

	//========================================
	// 状態パラメータの設定
	isAlive_ = true; // 生存状態
	radius_ = 1.0f;	 // 当たり判定の半径

	//========================================
	// パーティクル関連の初期化
	particle_ = nullptr;
	particleSetup_ = nullptr;
	particleCreated_ = false;

	//========================================
	// 破壊演出関連の初期化
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = 2.0f; // 2秒間破壊演出を表示

	//========================================
	// BaseObjectの初期化（当たり判定）
	BaseObject::Initialize(transform_.translate, radius_);
}
///=============================================================================
///                        パーティクルシステムの設定
void Enemy::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;
}
///=============================================================================
///                        移動パラメータの設定
void Enemy::SetMovementParams(float speed, const Vector3 &targetPosition) {
	speed_ = speed;
	targetPosition_ = targetPosition;
	hasTarget_ = true;

	// 目標位置への方向ベクトルを計算
	Vector3 direction = {
		targetPosition_.x - transform_.translate.x,
		targetPosition_.y - transform_.translate.y,
		targetPosition_.z - transform_.translate.z};

	// 正規化
	float length = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (length > 0.0f) {
		direction.x /= length;
		direction.y /= length;
		direction.z /= length;
	}

	// 速度ベクトルを設定
	velocity_ = {
		direction.x * speed_,
		direction.y * speed_,
		direction.z * speed_};
}
///=============================================================================
///                        更新
void Enemy::Update() {
	// 死亡状態またはオブジェクトが無効な場合は処理しない
	if (destroyState_ == DestroyState::Dead || !obj_) {
		return;
	}

	// 破壊演出の更新
	if (destroyState_ == DestroyState::Destroying) {
		UpdateDestruction();
		return;
	}

	// 通常状態の更新
	if (destroyState_ == DestroyState::Alive) {
		if (hasTarget_) {
			UpdateAIMovement();
		} else {
			UpdateMovement();
		}
		CheckOutOfBounds();

		// BaseObjectのコライダー位置を更新
		BaseObject::Update(transform_.translate);
	}

	// オブジェクトの更新
	UpdateObject();
}
///=============================================================================
///                        破壊演出の更新
void Enemy::UpdateDestruction() {
	const float frameTime = 1.0f / 60.0f;
	destroyTimer_ += frameTime;

	// 破壊演出時間が経過したら完全に消滅
	if (destroyTimer_ >= destroyDuration_) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
	}
}
///=============================================================================
///                        通常状態の更新（移動・回転）
void Enemy::UpdateMovement() {
	const float frameTime = 1.0f / 60.0f;

	// 位置の更新
	transform_.translate.x += velocity_.x * frameTime;
	transform_.translate.y += velocity_.y * frameTime;
	transform_.translate.z += velocity_.z * frameTime;

	// 回転の更新
	transform_.rotate.z += rotationSpeed_ * frameTime;

	// Object3dのトランスフォームに反映
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}
}
///=============================================================================
///                        AI移動の更新
void Enemy::UpdateAIMovement() {
	const float frameTime = 1.0f / 60.0f;

	// 目標位置への距離を計算
	Vector3 toTarget = {
		targetPosition_.x - transform_.translate.x,
		targetPosition_.y - transform_.translate.y,
		targetPosition_.z - transform_.translate.z};

	float distanceToTarget = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

	// 目標位置に近づいたら通常移動に切り替え
	if (distanceToTarget < 2.0f) {
		hasTarget_ = false;
		velocity_ = {0.0f, 0.0f, -speed_}; // プレイヤー方向への直進
	}

	// 位置の更新
	transform_.translate.x += velocity_.x * frameTime;
	transform_.translate.y += velocity_.y * frameTime;
	transform_.translate.z += velocity_.z * frameTime;

	// 移動方向に向けて回転
	if (velocity_.x != 0.0f || velocity_.z != 0.0f) {
		float targetAngle = atan2f(velocity_.x, velocity_.z);
		transform_.rotate.y = targetAngle;
	}

	// 回転の更新
	transform_.rotate.z += rotationSpeed_ * frameTime;

	// Object3dのトランスフォームに反映
	Transform *objTransform = obj_->GetTransform();
	if (objTransform) {
		*objTransform = transform_;
	}
}
///=============================================================================
///                        画面外判定
void Enemy::CheckOutOfBounds() {
	// 画面外に出たら削除
	if (transform_.translate.z < -20.0f) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
	}
}
///=============================================================================
///                        オブジェクトの更新
void Enemy::UpdateObject() {
	obj_->Update();
}
///=============================================================================
///                        描画
void Enemy::Draw() {
	// 生存中のみ描画
	if (destroyState_ == DestroyState::Alive && obj_) {
		obj_->Draw();
	}
}
///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	if (!obj_) {
		return;
	}

	ImGui::Begin("Enemy Debug");
	ImGui::Text("Position: (%.2f, %.2f, %.2f)",
				transform_.translate.x, transform_.translate.y, transform_.translate.z);
	ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity_.x, velocity_.y, velocity_.z);
	ImGui::Text("Is Alive: %s", isAlive_ ? "Yes" : "No");
	ImGui::SliderFloat("Speed", &speed_, 0.5f, 10.0f);
	ImGui::SliderFloat("Rotation Speed", &rotationSpeed_, 0.1f, 5.0f);
	ImGui::SliderFloat("Radius", &radius_, 0.5f, 3.0f);
	ImGui::End();
}
///=============================================================================
///                        位置取得
Vector3 Enemy::GetPosition() const {
	return transform_.translate;
}
///=============================================================================
///                        衝突処理関数
void Enemy::OnCollisionEnter(BaseObject *other) {
	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	//========================================
	// パーティクルエフェクトの生成
	if (particle_ && !particleCreated_) {
		Vector3 enemyPos = transform_.translate; // メイントランスフォームから位置取得

		//========================================
		// 1. 火花エフェクト（Board形状）- メインの爆発
		particle_->SetVelocityRange({-10.0f, -5.0f, -10.0f}, {10.0f, 10.0f, 10.0f});
		particle_->SetTranslateRange({-0.2f, -0.2f, -0.2f}, {0.2f, 0.2f, 0.2f});
		particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f}); // オレンジ～黄色
		particle_->SetLifetimeRange(0.5f, 1.5f);
		particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.8f, 0.8f, 0.8f});
		particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f}); // 最小値を0.0fから変更
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {3.14f, 3.14f, 3.14f});
		particle_->SetEndRotationRange({3.14f, 3.14f, 3.14f}, {6.28f, 6.28f, 6.28f}); // min < max確保
		particle_->SetGravity({0.0f, -8.0f, 0.0f});
		particle_->SetFadeInOut(0.02f, 0.8f);
		particle_->Emit("ExplosionSparks", enemyPos, 30); // 30個の火花

		//========================================
		// 2. 衝撃波エフェクト（Ring形状）
		particle_->SetVelocityRange({-2.0f, -1.0f, -2.0f}, {2.0f, 1.0f, 2.0f});
		particle_->SetTranslateRange({-0.1f, -0.1f, -0.1f}, {0.1f, 0.1f, 0.1f});
		particle_->SetColorRange({1.0f, 0.8f, 0.4f, 0.6f}, {1.0f, 1.0f, 0.8f, 0.8f}); // alpha値調整
		particle_->SetLifetimeRange(0.8f, 1.2f);
		particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
		particle_->SetEndScaleRange({3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f}); // 大きく広がる
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {3.14f, 3.14f, 3.14f});
		particle_->SetEndRotationRange({3.14f, 3.14f, 3.14f}, {6.28f, 6.28f, 6.28f}); // min < max確保
		particle_->SetGravity({0.0f, 0.0f, 0.0f});									  // 重力なし
		particle_->SetFadeInOut(0.1f, 0.6f);
		particle_->Emit("ExplosionRing", enemyPos, 3); // 3つの衝撃波リング

		//========================================
		// 3. 煙柱エフェクト（Cylinder形状）
		particle_->SetVelocityRange({-3.0f, 2.0f, -3.0f}, {3.0f, 8.0f, 3.0f}); // 上向きに強く
		particle_->SetTranslateRange({-0.3f, 0.0f, -0.3f}, {0.3f, 0.5f, 0.3f});
		particle_->SetColorRange({0.4f, 0.4f, 0.4f, 0.3f}, {0.8f, 0.8f, 0.8f, 0.6f}); // alpha値調整
		particle_->SetLifetimeRange(1.5f, 3.0f);									  // 長く残る
		particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
		particle_->SetEndScaleRange({1.5f, 2.0f, 1.5f}, {2.5f, 3.0f, 2.5f}); // 徐々に大きく
		particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {1.57f, 1.57f, 1.57f});
		particle_->SetEndRotationRange({1.57f, 1.57f, 1.57f}, {4.71f, 4.71f, 4.71f}); // min < max確保
		particle_->SetGravity({0.0f, -1.0f, 0.0f});									  // 軽い重力
		particle_->SetFadeInOut(0.2f, 0.7f);
		particle_->Emit("ExplosionSmoke", enemyPos, 8); // 8個の煙柱

		particleCreated_ = true;
	}

	//========================================
	// 破壊状態に移行（すぐには消さない）
	destroyState_ = DestroyState::Destroying;
	destroyTimer_ = 0.0f;
}
///=============================================================================
///                        衝突継続処理
void Enemy::OnCollisionStay(BaseObject *other) {
	// 継続中の衝突処理（必要に応じて実装）
}
///=============================================================================
///                        衝突終了処理
void Enemy::OnCollisionExit(BaseObject *other) {
	// 衝突終了時の処理（必要に応じて実装）
}

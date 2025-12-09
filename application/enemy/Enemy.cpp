#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include <algorithm>
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
	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}

	//========================================
	// 状態の初期化
	isAlive_ = true;	 // 生存状態
	radius_ = 1.0f;		 // 当たり判定の半径
	speed_ = 10.0f;		 // 移動速度
	lifeTimer_ = 0.0f;	 // 生存時間タイマー初期化
	maxLifeTime_ = 5.0f; // 5秒後に自動消滅
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = 2.0f; // 2秒間破壊演出を表示
	maxHP_ = 3;				 // 最大HP（3回ヒットで撃破）
	currentHP_ = maxHP_;	 // 現在のHPを最大値で初期化

	//========================================
	// パーティクル関連の初期化
	particle_ = nullptr;
	particleCreated_ = false;

	//========================================
	// ヒットリアクション関連の初期化
	isHitReacting_ = false;
	hitReactionTimer_ = 0.0f;
	hitReactionDuration_ = 0.15f; // 0.15秒間のヒットリアクション
	hitFlashCount_ = 0;
	originalScale_ = transform_.scale;
	hitScale_ = {1.3f, 1.3f, 1.3f}; // ヒット時に1.3倍に拡大
	shouldRenderThisFrame_ = true;	// 初期状態は描画する

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
///                        更新
void Enemy::Update() {
	// 死亡状態またはオブジェクトが無効な場合は処理しない
	if (destroyState_ == DestroyState::Dead || !obj_) {
		return;
	}

	// 破壊演出の更新
	if (destroyState_ == DestroyState::Destroying) {
		destroyTimer_ += 1.0f / 60.0f;
		if (destroyTimer_ >= destroyDuration_) {
			destroyState_ = DestroyState::Dead;
			isAlive_ = false;
		}
		return;
	}

	//========================================
	// ヒットリアクションの更新
	if (isHitReacting_) {
		hitReactionTimer_ += 1.0f / 60.0f;

		// 点滅効果（0.05秒ごとに切り替え）
		int flashInterval = static_cast<int>(hitReactionTimer_ / 0.05f);
		shouldRenderThisFrame_ = (flashInterval % 2 == 0);

		// スケール変化（徐々に元に戻る）
		float t = hitReactionTimer_ / hitReactionDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		// イージングアウト（二次関数）
		float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
		transform_.scale = {
			hitScale_.x + (originalScale_.x - hitScale_.x) * easeOut,
			hitScale_.y + (originalScale_.y - hitScale_.y) * easeOut,
			hitScale_.z + (originalScale_.z - hitScale_.z) * easeOut};

		// ヒットリアクション終了
		if (hitReactionTimer_ >= hitReactionDuration_) {
			isHitReacting_ = false;
			hitReactionTimer_ = 0.0f;
			transform_.scale = originalScale_;
			shouldRenderThisFrame_ = true;
		}
	} else {
		// 通常状態では常に描画
		shouldRenderThisFrame_ = true;
	}

	// Z方向（正面）へ移動
	transform_.translate.z += speed_ * (1.0f / 60.0f);

	// 生存時間の更新
	lifeTimer_ += 1.0f / 60.0f;

	// 最大生存時間を超えたら消滅
	if (lifeTimer_ >= maxLifeTime_) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
		return;
	}

	// Object3dのトランスフォームを更新
	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}

	BaseObject::Update(transform_.translate);
	obj_->Update();
}
///=============================================================================
///                        描画
void Enemy::Draw() {
	// 生存中かつ描画フラグが立っている場合のみ描画
	if (destroyState_ == DestroyState::Alive && obj_ && shouldRenderThisFrame_) {
		obj_->Draw();
	}
}
///=============================================================================
///                        ImGui描画
void Enemy::DrawImGui() {
	if (!obj_)
		return;
	ImGui::Begin("Enemy Debug");
	ImGui::Text("Position: (%.2f, %.2f, %.2f)", transform_.translate.x, transform_.translate.y, transform_.translate.z);
	ImGui::Text("Is Alive: %s", isAlive_ ? "Yes" : "No");
	ImGui::Text("HP: %d / %d", currentHP_, maxHP_);
	ImGui::SliderFloat("Speed", &speed_, 5.0f, 30.0f);
	ImGui::SliderInt("Max HP", &maxHP_, 1, 10);
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
	// FIXME: otherが使用されていないので修正すること
	other;

	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	// ダメージを与える（1ダメージ）
	TakeDamage(1);
}
///=============================================================================
///                        衝突継続処理
void Enemy::OnCollisionStay(BaseObject *other) {
	// FIXME: otherが使用されていないので修正すること
	other;
	// 継続中の衝突処理（必要に応じて実装）
}
///=============================================================================
///                        衝突終了処理
void Enemy::OnCollisionExit(BaseObject *other) {
	// FIXME: otherが使用されていないので修正すること
	other;
	// 衝突終了時の処理（必要に応じて実装）
}

void Enemy::StartHitReaction() {
	// 既に破壊中の場合は実行しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	isHitReacting_ = true;
	hitReactionTimer_ = 0.0f;
	hitFlashCount_ = 0;
}

///=============================================================================
///                        ダメージ処理
void Enemy::TakeDamage(int damage) {
	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	// HPを減らす
	currentHP_ -= damage;

	// ヒットリアクション開始
	StartHitReaction();

	// ヒット時のパーティクルエフェクト（軽量版）
	if (particle_) {
		Vector3 enemyPos = transform_.translate;

		//========================================
		// ヒット時の衝撃波エフェクト（Ring形状）
		particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
		particle_->SetColorRange({1.0f, 0.8f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f}); // 黄～オレンジ
		particle_->SetLifetimeRange(0.2f, 0.3f);
		particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.5f, 0.5f, 0.5f});
		particle_->SetEndScaleRange({1.5f, 1.5f, 1.5f}, {2.0f, 2.0f, 2.0f}); // 急速に拡大
		particle_->SetGravity({0.0f, 0.0f, 0.0f});
		particle_->SetFadeInOut(0.0f, 1.0f);
		particle_->Emit("ExplosionRing", enemyPos, 1); // 衝撃波1個
	}

	// HPが0以下になったら破壊
	if (currentHP_ <= 0) {
		// 破壊時の大規模なエフェクト
		if (particle_ && !particleCreated_) {
			Vector3 enemyPos = transform_.translate;

			//========================================
			// 1. 破壊時の衝撃波エフェクト（Ring形状）
			particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
			particle_->SetColorRange({1.0f, 0.8f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f});
			particle_->SetLifetimeRange(0.2f, 0.3f);
			particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.8f});
			particle_->SetEndScaleRange({2.5f, 2.5f, 2.5f}, {3.5f, 3.5f, 3.5f});
			particle_->SetGravity({0.0f, 0.0f, 0.0f});
			particle_->SetFadeInOut(0.0f, 1.0f);
			particle_->Emit("ExplosionRing", enemyPos, 2);

			//========================================
			// 2. 火花エフェクト（Board形状）
			particle_->SetVelocityRange({-10.0f, -5.0f, -10.0f}, {10.0f, 10.0f, 10.0f});
			particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f});
			particle_->SetLifetimeRange(0.5f, 1.5f);
			particle_->SetInitialScaleRange({0.3f, 0.3f, 0.3f}, {0.8f, 0.8f, 0.8f});
			particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f}); // 最小値が最大値以下であることを確認
			particle_->SetGravity({0.0f, -8.0f, 0.0f});
			particle_->SetFadeInOut(0.02f, 0.8f);
			particle_->Emit("ExplosionSparks", enemyPos, 30);

			particleCreated_ = true;
		}

		// 破壊状態に移行
		destroyState_ = DestroyState::Destroying;
		destroyTimer_ = 0.0f;
	}
}
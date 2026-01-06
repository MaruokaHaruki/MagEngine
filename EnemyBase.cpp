#define _USE_MATH_DEFINES
#define NOMINMAX
#include "EnemyBase.h"
#include "ImguiSetup.h"
#include "Particle.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

// 定数定義
namespace {
	constexpr float kDefaultSpeed = 10.0f;
	constexpr float kDefaultRadius = 1.0f;
	constexpr float kDefaultLifeTime = 60.0f;
	constexpr float kDestroyDuration = 2.0f;
	constexpr float kHitReactionDuration = 0.3f;
	constexpr float kShakeAmplitude = 0.2f;
	constexpr float kShakeFrequency = 25.0f;
	constexpr float kKnockbackStrength = 3.0f;
	constexpr int kDefaultMaxHP = 3;
}

///=============================================================================
///                        基本初期化
void EnemyBase::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) {
	// 3Dオブジェクトの初期化
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);

	// トランスフォームの初期設定
	transform_.translate = position;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};

	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}

	// 基本パラメータの初期化
	isAlive_ = true;
	radius_ = kDefaultRadius;
	speed_ = kDefaultSpeed;
	lifeTimer_ = 0.0f;
	maxLifeTime_ = kDefaultLifeTime;
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = kDestroyDuration;
	maxHP_ = kDefaultMaxHP;
	currentHP_ = maxHP_;

	// パーティクル関連の初期化
	particle_ = nullptr;
	particleCreated_ = false;

	// ヒットリアクション関連の初期化
	isHitReacting_ = false;
	hitReactionTimer_ = 0.0f;
	hitReactionDuration_ = kHitReactionDuration;
	hitFlashCount_ = 0;
	originalScale_ = transform_.scale;
	hitScale_ = {1.5f, 1.5f, 1.5f};
	shouldRenderThisFrame_ = true;
	knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
	shakeAmplitude_ = kShakeAmplitude;
	shakeFrequency_ = kShakeFrequency;
	hitStartPosition_ = {0.0f, 0.0f, 0.0f};
	isInvincible_ = false;

	// プレイヤー参照
	player_ = nullptr;

	// BaseObjectの初期化
	BaseObject::Initialize(transform_.translate, radius_);
}

///=============================================================================
///                        パーティクルシステムの設定
void EnemyBase::SetParticleSystem(Particle *particle, ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;
}

///=============================================================================
///                        更新
void EnemyBase::Update() {
	if (destroyState_ == DestroyState::Dead || !obj_) {
		return;
	}

	// 破壊演出の更新
	if (destroyState_ == DestroyState::Destroying) {
		if (UpdateDestroy()) {
			destroyState_ = DestroyState::Dead;
			isAlive_ = false;
		}
		return;
	}

	// ヒットリアクションの更新
	if (isHitReacting_) {
		UpdateHitReaction();
	} else {
		shouldRenderThisFrame_ = true;
	}

	// 生存時間の更新
	lifeTimer_ += 1.0f / 60.0f;
	if (lifeTimer_ >= maxLifeTime_) {
		destroyState_ = DestroyState::Dead;
		isAlive_ = false;
		return;
	}

	// トランスフォームを反映
	if (Transform *objTransform = obj_->GetTransform()) {
		*objTransform = transform_;
	}

	BaseObject::Update(transform_.translate);
	obj_->Update();
}

///=============================================================================
///                        描画
void EnemyBase::Draw() {
	if (destroyState_ == DestroyState::Alive && obj_ && shouldRenderThisFrame_) {
		obj_->Draw();
	}
}

///=============================================================================
///                        ImGui描画
void EnemyBase::DrawImGui() {
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
///                        衝突処理
void EnemyBase::OnCollisionEnter(BaseObject *other) {
	other; // 警告回避
	if (isInvincible_ || destroyState_ != DestroyState::Alive) {
		return;
	}
	// ミサイルとマシンガン弾を区別してダメージ処理
	if (dynamic_cast<PlayerMissile *>(other)) {
		TakeDamage(2);
	} else if (dynamic_cast<PlayerBullet *>(other)) {
		TakeDamage(1);
	}
}

void EnemyBase::OnCollisionStay(BaseObject *other) {
	other; // 警告回避
}

void EnemyBase::OnCollisionExit(BaseObject *other) {
	other; // 警告回避
}

///=============================================================================
///                        ヒットリアクション開始
void EnemyBase::StartHitReaction() {
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	isHitReacting_ = true;
	isInvincible_ = true;
	hitReactionTimer_ = 0.0f;
	hitFlashCount_ = 0;
	hitStartPosition_ = transform_.translate;

	// ランダムなノックバック方向を生成
	knockbackVelocity_ = {
		(static_cast<float>(rand() % 200) - 100.0f) / 100.0f * kKnockbackStrength,
		(static_cast<float>(rand() % 100)) / 100.0f * kKnockbackStrength * 0.3f,
		-kKnockbackStrength * 1.5f};
}

///=============================================================================
///                        ヒットリアクション更新
void EnemyBase::UpdateHitReaction() {
	hitReactionTimer_ += 1.0f / 60.0f;

	// 点滅効果
	int flashInterval = static_cast<int>(hitReactionTimer_ / 0.03f);
	shouldRenderThisFrame_ = (flashInterval % 2 == 0);

	float t = std::clamp(hitReactionTimer_ / hitReactionDuration_, 0.0f, 1.0f);
	const float knockbackPhase = 0.5f;

	if (t < knockbackPhase) {
		// ノックバックフェーズ
		float knockbackT = t / knockbackPhase;
		float easeOut = 1.0f - std::pow(1.0f - knockbackT, 2.0f);

		Vector3 knockbackOffset = {
			knockbackVelocity_.x * easeOut * (1.0f - knockbackT),
			knockbackVelocity_.y * easeOut * (1.0f - knockbackT),
			knockbackVelocity_.z * easeOut * (1.0f - knockbackT)};

		transform_.translate = {
			hitStartPosition_.x + knockbackOffset.x,
			hitStartPosition_.y + knockbackOffset.y,
			hitStartPosition_.z + knockbackOffset.z};

		float scaleEase = 1.0f - std::pow(1.0f - knockbackT, 3.0f);
		transform_.scale = {
			hitScale_.x + (originalScale_.x - hitScale_.x) * scaleEase,
			hitScale_.y + (originalScale_.y - hitScale_.y) * scaleEase,
			hitScale_.z + (originalScale_.z - hitScale_.z) * scaleEase};
	} else {
		// 復帰フェーズ
		float returnT = (t - knockbackPhase) / (1.0f - knockbackPhase);
		float easeIn = returnT * returnT;

		Vector3 knockbackEndPos = {
			hitStartPosition_.x + knockbackVelocity_.x,
			hitStartPosition_.y + knockbackVelocity_.y,
			hitStartPosition_.z + knockbackVelocity_.z};

		float timeElapsed = hitReactionTimer_;
		Vector3 targetPos = {
			hitStartPosition_.x,
			hitStartPosition_.y,
			hitStartPosition_.z + speed_ * timeElapsed};

		transform_.translate = {
			knockbackEndPos.x + (targetPos.x - knockbackEndPos.x) * easeIn,
			knockbackEndPos.y + (targetPos.y - knockbackEndPos.y) * easeIn,
			knockbackEndPos.z + (targetPos.z - knockbackEndPos.z) * easeIn};

		float scaleEase = 1.0f - std::pow(1.0f - returnT, 3.0f);
		transform_.scale = {
			hitScale_.x + (originalScale_.x - hitScale_.x) * scaleEase,
			hitScale_.y + (originalScale_.y - hitScale_.y) * scaleEase,
			hitScale_.z + (originalScale_.z - hitScale_.z) * scaleEase};
	}

	// 揺れ効果
	float shakeFade = 1.0f - t;
	float shakeOffset = std::sin(hitReactionTimer_ * shakeFrequency_) * shakeAmplitude_ * shakeFade;
	transform_.translate.x += shakeOffset;
	transform_.translate.y += shakeOffset * 0.5f;

	// ヒットリアクション終了
	if (hitReactionTimer_ >= hitReactionDuration_) {
		isHitReacting_ = false;
		isInvincible_ = false;
		hitReactionTimer_ = 0.0f;
		transform_.scale = originalScale_;
		shouldRenderThisFrame_ = true;
	}
}

///=============================================================================
///                        破壊演出更新
bool EnemyBase::UpdateDestroy() {
	destroyTimer_ += 1.0f / 60.0f;
	return destroyTimer_ >= destroyDuration_;
}

///=============================================================================
///                        ダメージ処理
void EnemyBase::TakeDamage(int damage, std::function<void()> onDefeatCallback) {
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	// コールバックが渡された場合は更新（初回設定用）
	if (onDefeatCallback) {
		onDefeatCallback_ = onDefeatCallback;
	}

	currentHP_ -= damage;
	StartHitReaction();
	CreateHitParticle();

	if (currentHP_ <= 0) {
		// 保持しているコールバックを実行
		if (onDefeatCallback_) {
			onDefeatCallback_();
		}
		CreateDestroyParticle();
		StartDestroy();
	}
}

///=============================================================================
///                        破壊開始
void EnemyBase::StartDestroy() {
	destroyState_ = DestroyState::Destroying;
	destroyTimer_ = 0.0f;
}

///=============================================================================
///                        ヒット時パーティクル
void EnemyBase::CreateHitParticle() {
	if (!particle_)
		return;

	Vector3 enemyPos = transform_.translate;

	// 衝撃波
	particle_->SetBillboard(false);
	particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	particle_->SetColorRange({1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f});
	particle_->SetLifetimeRange(0.25f, 0.35f);
	particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.8f});
	particle_->SetEndScaleRange({2.5f, 2.5f, 2.5f}, {3.5f, 3.5f, 3.5f});
	particle_->SetGravity({0.0f, 0.0f, 0.0f});
	particle_->SetFadeInOut(0.0f, 1.0f);
	particle_->Emit("ExplosionRing", enemyPos, 2);

	// 火花
	particle_->SetBillboard(true);
	particle_->SetVelocityRange({-5.0f, -3.0f, -5.0f}, {5.0f, 5.0f, 5.0f});
	particle_->SetColorRange({1.0f, 0.9f, 0.3f, 1.0f}, {1.0f, 0.5f, 0.1f, 1.0f});
	particle_->SetLifetimeRange(0.2f, 0.4f);
	particle_->SetInitialScaleRange({0.4f, 0.4f, 0.4f}, {0.7f, 0.7f, 0.7f});
	particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
	particle_->SetGravity({0.0f, -5.0f, 0.0f});
	particle_->SetFadeInOut(0.0f, 0.8f);
	particle_->Emit("ExplosionSparks", enemyPos, 20);
}

///=============================================================================
///                        破壊時パーティクル
void EnemyBase::CreateDestroyParticle() {
	if (!particle_ || particleCreated_)
		return;

	Vector3 enemyPos = transform_.translate;

	// 大規模な衝撃波
	particle_->SetBillboard(false);
	particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	particle_->SetColorRange({1.0f, 0.9f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f});
	particle_->SetLifetimeRange(0.3f, 0.5f);
	particle_->SetInitialScaleRange({1.0f, 1.0f, 1.0f}, {1.5f, 1.5f, 1.5f});
	particle_->SetEndScaleRange({4.0f, 4.0f, 4.0f}, {6.0f, 6.0f, 6.0f});
	particle_->SetGravity({0.0f, 0.0f, 0.0f});
	particle_->SetFadeInOut(0.0f, 1.0f);
	particle_->Emit("ExplosionRing", enemyPos, 3);

	// 大量の火花
	particle_->SetBillboard(true);
	particle_->SetVelocityRange({-15.0f, -10.0f, -15.0f}, {15.0f, 15.0f, 15.0f});
	particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f});
	particle_->SetLifetimeRange(0.5f, 1.5f);
	particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.2f, 1.2f, 1.2f});
	particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
	particle_->SetGravity({0.0f, -8.0f, 0.0f});
	particle_->SetFadeInOut(0.02f, 0.8f);
	particle_->Emit("ExplosionSparks", enemyPos, 60);

	particleCreated_ = true;
}

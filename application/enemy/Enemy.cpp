#define _USE_MATH_DEFINES
#define NOMINMAX
#include "Enemy.h"
#include "EnemyManager.h" // EnemyTypeの定義のため
#include "ImguiSetup.h"
#include "Object3d.h"
#include "Particle.h"
#include "Player.h" // プレイヤークラスの実体が必要
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
	isAlive_ = true;
	radius_ = EnemyConstants::kDefaultRadius;
	speed_ = EnemyConstants::kDefaultSpeed;
	lifeTimer_ = 0.0f;
	maxLifeTime_ = EnemyConstants::kDefaultLifeTime;
	destroyState_ = DestroyState::Alive;
	destroyTimer_ = 0.0f;
	destroyDuration_ = EnemyConstants::kDestroyDuration;
	maxHP_ = EnemyConstants::kDefaultMaxHP;
	currentHP_ = maxHP_;

	//========================================
	// パーティクル関連の初期化
	particle_ = nullptr;
	particleCreated_ = false;

	//========================================
	// ヒットリアクション関連の初期化
	isHitReacting_ = false;
	hitReactionTimer_ = 0.0f;
	hitReactionDuration_ = EnemyConstants::kHitReactionDuration;
	hitFlashCount_ = 0;
	originalScale_ = transform_.scale;
	hitScale_ = {1.5f, 1.5f, 1.5f};
	shouldRenderThisFrame_ = true;
	knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
	shakeAmplitude_ = EnemyConstants::kShakeAmplitude;
	shakeFrequency_ = EnemyConstants::kShakeFrequency;
	hitStartPosition_ = {0.0f, 0.0f, 0.0f};
	isInvincible_ = false; // 無敵フラグ初期化

	//========================================
	// 行動ステート関連の初期化
	behaviorState_ = BehaviorState::Approach;
	combatTimer_ = 0.0f;
	combatDuration_ = EnemyConstants::kCombatDuration;
	combatCenter_ = {0.0f, 0.0f, 0.0f};
	circleAngle_ = 0.0f;
	player_ = nullptr;

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

		// 点滅効果（0.03秒ごとに切り替え）
		int flashInterval = static_cast<int>(hitReactionTimer_ / 0.03f);
		shouldRenderThisFrame_ = (flashInterval % 2 == 0);

		// 時間の正規化（0.0～1.0）
		float t = hitReactionTimer_ / hitReactionDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		//========================================
		// フェーズ分割：前半50%でノックバック、後半50%で復帰
		const float knockbackPhase = 0.5f; // 前半50%

		if (t < knockbackPhase) {
			// ノックバックフェーズ（0.0～0.5）
			float knockbackT = t / knockbackPhase; // 0.0～1.0に正規化

			// イージングアウト（スムーズに減速）
			float easeOut = 1.0f - std::pow(1.0f - knockbackT, 2.0f);

			// ノックバック移動（滑らかに減速）
			Vector3 knockbackOffset = {
				knockbackVelocity_.x * easeOut * (1.0f - knockbackT),
				knockbackVelocity_.y * easeOut * (1.0f - knockbackT),
				knockbackVelocity_.z * easeOut * (1.0f - knockbackT)};

			transform_.translate = {
				hitStartPosition_.x + knockbackOffset.x,
				hitStartPosition_.y + knockbackOffset.y,
				hitStartPosition_.z + knockbackOffset.z};

			// スケール変化（最大まで拡大）
			float scaleEase = 1.0f - std::pow(1.0f - knockbackT, 3.0f);
			transform_.scale = {
				hitScale_.x + (originalScale_.x - hitScale_.x) * scaleEase,
				hitScale_.y + (originalScale_.y - hitScale_.y) * scaleEase,
				hitScale_.z + (originalScale_.z - hitScale_.z) * scaleEase};

		} else {
			// 復帰フェーズ（0.5～1.0）
			float returnT = (t - knockbackPhase) / (1.0f - knockbackPhase); // 0.0～1.0に正規化

			// イージングイン（スムーズに加速して元の位置へ）
			float easeIn = returnT * returnT;

			// 現在のノックバック後の位置を計算
			Vector3 knockbackEndPos = {
				hitStartPosition_.x + knockbackVelocity_.x,
				hitStartPosition_.y + knockbackVelocity_.y,
				hitStartPosition_.z + knockbackVelocity_.z};

			// 元の軌道上の位置（通常移動を考慮）
			float timeElapsed = hitReactionTimer_;
			Vector3 targetPos = {
				hitStartPosition_.x,
				hitStartPosition_.y,
				hitStartPosition_.z + speed_ * timeElapsed};

			// ノックバック終了位置から目標位置へ補間
			transform_.translate = {
				knockbackEndPos.x + (targetPos.x - knockbackEndPos.x) * easeIn,
				knockbackEndPos.y + (targetPos.y - knockbackEndPos.y) * easeIn,
				knockbackEndPos.z + (targetPos.z - knockbackEndPos.z) * easeIn};

			// スケール復帰
			float scaleEase = 1.0f - std::pow(1.0f - returnT, 3.0f);
			transform_.scale = {
				hitScale_.x + (originalScale_.x - hitScale_.x) * scaleEase,
				hitScale_.y + (originalScale_.y - hitScale_.y) * scaleEase,
				hitScale_.z + (originalScale_.z - hitScale_.z) * scaleEase};
		}

		//========================================
		// 揺れ効果（全体を通して）
		float shakeFade = 1.0f - t;
		float shakeOffset = std::sin(hitReactionTimer_ * shakeFrequency_) * shakeAmplitude_ * shakeFade;
		transform_.translate.x += shakeOffset;
		transform_.translate.y += shakeOffset * 0.5f;

		// ヒットリアクション終了
		if (hitReactionTimer_ >= hitReactionDuration_) {
			isHitReacting_ = false;
			isInvincible_ = false; // 無敵時間終了
			hitReactionTimer_ = 0.0f;
			transform_.scale = originalScale_;
			shouldRenderThisFrame_ = true;
		}
	} else {
		// 通常状態では常に描画
		shouldRenderThisFrame_ = true;
	}

	// === ステートマシンによる行動制御 ===
	if (!isHitReacting_) {
		const float deltaTime = 1.0f / 60.0f;

		switch (behaviorState_) {
		case BehaviorState::Approach: {
			// 接近フェーズ: プレイヤーの前方へ高速接近
			if (player_) {
				Vector3 playerPos = player_->GetPosition();
				combatCenter_ = playerPos;

				// プレイヤーの前方25m地点を目指す
				Vector3 targetPos = {
					playerPos.x,
					playerPos.y,
					playerPos.z + EnemyConstants::kCombatRadius};

				Vector3 direction = {
					targetPos.x - transform_.translate.x,
					targetPos.y - transform_.translate.y,
					targetPos.z - transform_.translate.z};

				float distance = std::sqrt(
					direction.x * direction.x +
					direction.y * direction.y +
					direction.z * direction.z);

				// 目標地点に到達したら戦闘フェーズへ
				if (distance < 5.0f) {
					behaviorState_ = BehaviorState::Combat;
					combatTimer_ = 0.0f;
					circleAngle_ = std::atan2(
						transform_.translate.x - playerPos.x,
						transform_.translate.z - playerPos.z);
				} else {
					// 正規化して移動
					direction.x /= distance;
					direction.y /= distance;
					direction.z /= distance;

					transform_.translate.x += direction.x * EnemyConstants::kApproachSpeed * deltaTime;
					transform_.translate.y += direction.y * EnemyConstants::kApproachSpeed * deltaTime;
					transform_.translate.z += direction.z * EnemyConstants::kApproachSpeed * deltaTime;
				}
			} else {
				// プレイヤーがいない場合は直進
				transform_.translate.z += speed_ * deltaTime;
			}
			break;
		}

		case BehaviorState::Combat: {
			// 戦闘フェーズ: プレイヤーの周囲を旋回
			combatTimer_ += deltaTime;

			if (player_) {
				combatCenter_ = player_->GetPosition();
			}

			// 戦闘時間を超えたら退却へ
			if (combatTimer_ >= combatDuration_) {
				behaviorState_ = BehaviorState::Retreat;
				break;
			}

			// 円運動 + 8の字運動
			circleAngle_ += EnemyConstants::kCircleFrequency * deltaTime;
			float radius = EnemyConstants::kCombatRadius;

			// 8の字運動を追加
			float verticalOffset = std::sin(circleAngle_ * 2.0f) * 4.0f;

			// 目標位置を計算
			Vector3 targetPos = {
				combatCenter_.x + std::sin(circleAngle_) * radius,
				combatCenter_.y + verticalOffset,
				combatCenter_.z + std::cos(circleAngle_) * radius};

			// スムーズに目標位置へ移動
			Vector3 direction = {
				targetPos.x - transform_.translate.x,
				targetPos.y - transform_.translate.y,
				targetPos.z - transform_.translate.z};

			float distance = std::sqrt(
				direction.x * direction.x +
				direction.y * direction.y +
				direction.z * direction.z);

			if (distance > 0.1f) {
				direction.x /= distance;
				direction.y /= distance;
				direction.z /= distance;

				transform_.translate.x += direction.x * EnemyConstants::kCombatSpeed * deltaTime;
				transform_.translate.y += direction.y * EnemyConstants::kCombatSpeed * deltaTime;
				transform_.translate.z += direction.z * EnemyConstants::kCombatSpeed * deltaTime;
			}
			break;
		}

		case BehaviorState::Retreat: {
			// 退却フェーズ: 高速で画面外へ
			transform_.translate.z += EnemyConstants::kRetreatSpeed * deltaTime;
			transform_.translate.y += 8.0f * deltaTime; // 上昇しながら退却
			break;
		}
		}
	}

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
	other; // 警告回避

	// 無敵時間中または破壊中は処理しない
	if (isInvincible_ || destroyState_ != DestroyState::Alive) {
		return;
	}

	// ダメージを与える（1ダメージ）
	TakeDamage(1);
}
///=============================================================================
///                        衝突継続処理
void Enemy::OnCollisionStay(BaseObject *other) {
	other; // 警告回避
		   // 継続中の衝突処理（必要に応じて実装）
}
///=============================================================================
///                        衝突終了処理
void Enemy::OnCollisionExit(BaseObject *other) {
	other; // 警告回避
		   // 衝突終了時の処理（必要に応じて実装）
}

void Enemy::StartHitReaction() {
	// 既に破壊中の場合は実行しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	isHitReacting_ = true;
	isInvincible_ = true; // 無敵時間開始
	hitReactionTimer_ = 0.0f;
	hitFlashCount_ = 0;

	// 現在の位置を保存（復帰用）
	hitStartPosition_ = transform_.translate;

	//========================================
	// ランダムなノックバック方向を生成
	float knockbackStrength = EnemyConstants::kKnockbackStrength;
	knockbackVelocity_ = {
		(static_cast<float>(rand() % 200) - 100.0f) / 100.0f * knockbackStrength,
		(static_cast<float>(rand() % 100)) / 100.0f * knockbackStrength * 0.3f,
		-knockbackStrength * 1.5f};
}
///=============================================================================
///                        ダメージ処理
void Enemy::TakeDamage(int damage, std::function<void()> onDefeatCallback) {
	// 既に破壊中または死亡している場合は処理しない
	if (destroyState_ != DestroyState::Alive) {
		return;
	}

	// HPを減らす
	currentHP_ -= damage;

	// ヒットリアクション開始
	StartHitReaction();

	// ヒット時のパーティクルエフェクト
	if (particle_) {
		Vector3 enemyPos = transform_.translate;

		//========================================
		// 1. ヒット時の衝撃波エフェクト（Ring形状 - より大きく）
		particle_->SetBillboard(false); // リングはビルボード無効
		particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
		particle_->SetColorRange({1.0f, 0.9f, 0.2f, 1.0f}, {1.0f, 0.6f, 0.0f, 1.0f}); // 明るい黄色～オレンジ
		particle_->SetLifetimeRange(0.25f, 0.35f);									  // 少し長めに
		particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.8f});
		particle_->SetEndScaleRange({2.5f, 2.5f, 2.5f}, {3.5f, 3.5f, 3.5f}); // より大きく拡大
		particle_->SetGravity({0.0f, 0.0f, 0.0f});
		particle_->SetFadeInOut(0.0f, 1.0f);
		particle_->Emit("ExplosionRing", enemyPos, 2); // 衝撃波2個

		//========================================
		// 2. ヒット時の火花エフェクト（Board形状）
		particle_->SetBillboard(true); // Board形状はビルボード有効
		particle_->SetVelocityRange({-5.0f, -3.0f, -5.0f}, {5.0f, 5.0f, 5.0f});
		particle_->SetColorRange({1.0f, 0.9f, 0.3f, 1.0f}, {1.0f, 0.5f, 0.1f, 1.0f}); // 明るい黄色～オレンジ
		particle_->SetLifetimeRange(0.2f, 0.4f);
		particle_->SetInitialScaleRange({0.4f, 0.4f, 0.4f}, {0.7f, 0.7f, 0.7f});
		particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.2f, 0.2f, 0.2f});
		particle_->SetGravity({0.0f, -5.0f, 0.0f});
		particle_->SetFadeInOut(0.0f, 0.8f);
		particle_->Emit("ExplosionSparks", enemyPos, 20); // ヒット時も火花20個
	}

	// HPが0以下になったら破壊
	if (currentHP_ <= 0) {
		// 撃破コールバックを実行（撃破数カウント）
		if (onDefeatCallback) {
			onDefeatCallback();
		}

		// 破壊時の大規模なエフェクト
		if (particle_ && !particleCreated_) {
			Vector3 enemyPos = transform_.translate;

			//========================================
			// 1. 破壊時の衝撃波エフェクト（Ring形状 - さらに大きく）
			particle_->SetBillboard(false);
			particle_->SetVelocityRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
			particle_->SetColorRange({1.0f, 0.9f, 0.0f, 1.0f}, {1.0f, 0.5f, 0.0f, 1.0f});
			particle_->SetLifetimeRange(0.3f, 0.5f);
			particle_->SetInitialScaleRange({1.0f, 1.0f, 1.0f}, {1.5f, 1.5f, 1.5f});
			particle_->SetEndScaleRange({4.0f, 4.0f, 4.0f}, {6.0f, 6.0f, 6.0f}); // 非常に大きく
			particle_->SetGravity({0.0f, 0.0f, 0.0f});
			particle_->SetFadeInOut(0.0f, 1.0f);
			particle_->Emit("ExplosionRing", enemyPos, 3); // 衝撃波3個

			//========================================
			// 2. 破壊時の火花エフェクト（Board形状 - 大量）
			particle_->SetBillboard(true);
			particle_->SetVelocityRange({-15.0f, -10.0f, -15.0f}, {15.0f, 15.0f, 15.0f}); // より激しく
			particle_->SetColorRange({1.0f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.3f, 1.0f});
			particle_->SetLifetimeRange(0.5f, 1.5f);
			particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.2f, 1.2f, 1.2f}); // より大きく
			particle_->SetEndScaleRange({0.1f, 0.1f, 0.1f}, {0.3f, 0.3f, 0.3f});
			particle_->SetGravity({0.0f, -8.0f, 0.0f});
			particle_->SetFadeInOut(0.02f, 0.8f);
			particle_->Emit("ExplosionSparks", enemyPos, 60); // 60個の火花

			particleCreated_ = true;
		}

		// 破壊状態に移行
		destroyState_ = DestroyState::Destroying;
		destroyTimer_ = 0.0f;
	}
}
///=============================================================================
///                        敵タイプ設定
void Enemy::SetEnemyType(EnemyType type) {
	switch (type) {
	case EnemyType::Normal:
		maxHP_ = EnemyConstants::kNormalEnemyHP;
		currentHP_ = maxHP_;
		speed_ = EnemyConstants::kNormalEnemySpeed;
		break;
	case EnemyType::Fast:
		maxHP_ = EnemyConstants::kFastEnemyHP;
		currentHP_ = maxHP_;
		speed_ = EnemyConstants::kFastEnemySpeed;
		break;
	}
}
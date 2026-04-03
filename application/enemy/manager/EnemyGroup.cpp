/*********************************************************************
 * \file   EnemyGroup.cpp
 * \brief  敵のグループ管理クラス実装
 *
 * \author Harukichimaru
 * \date   March 2026
 *********************************************************************/
#include "EnemyGroup.h"
#include "EnemyBase.h"
#include "Enemy.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

///=============================================================================
/// コンストラクタ
EnemyGroup::EnemyGroup()
	: groupId_(-1),
	  leaderEnemy_(nullptr),
	  groupState_(GroupState::Approaching),
	  stateTimer_(0.0f),
	  formationUpdateTimer_(0.0f),
	  minFormationUpdateInterval_(0.1f) {
}

///=============================================================================
/// グループ初期化
void EnemyGroup::Initialize(EnemyBase *leaderEnemy, FormationType formationType) {
	leaderEnemy_ = leaderEnemy;
	memberEnemies_.clear();
	memberEnemies_.push_back(leaderEnemy);

	currentFormation_ = CreateFormationConfig(formationType);
	memberTargetPositions_.resize(currentFormation_.maxMemberCount);
	groupState_ = GroupState::Approaching;
}

///=============================================================================
/// グループにメンバを追加
void EnemyGroup::AddMember(EnemyBase *member, int positionIndex) {
	if (!member || positionIndex < 0 || positionIndex >= currentFormation_.maxMemberCount) {
		return;
	}

	memberEnemies_.push_back(member);
}

///=============================================================================
/// グループ更新（編隊制御ロジック）
void EnemyGroup::Update(const Vector3 &playerPosition) {
	if (!leaderEnemy_ || !leaderEnemy_->IsAlive()) {
		return;
	}

	// フォーメーション更新タイマー
	formationUpdateTimer_ += (1.0f / 60.0f); // 60FPS想定

	// 定期的にフォーメーション更新
	if (formationUpdateTimer_ >= minFormationUpdateInterval_) {
		formationUpdateTimer_ = 0.0f;

		// 動的フォーメーションの場合は更新
		if (currentFormation_.type == FormationType::DynamicFormation) {
			currentFormation_ = CalculateDynamicFormation(playerPosition);
		}

		// メンバの目標位置を計算
		CalculateMemberTargetPositions(leaderEnemy_->GetPosition(), playerPosition);
	}

	// メンバの位置を更新
	UpdateMemberPositions();

	// グループ状態の更新
	stateTimer_ += (1.0f / 60.0f);
}

///=============================================================================
/// グループ内の敵削除処理
void EnemyGroup::RemoveDeadMembers() {
	// nullptr チェック付きで死敵を削除
	// NOTE: ダングリングポインタの参照を防ぐため、IsAlive() 呼び出し前に nullptr チェック
	memberEnemies_.erase(
		std::remove_if(memberEnemies_.begin(), memberEnemies_.end(),
					   [](EnemyBase *enemy) { 
					       return !enemy || !enemy->IsAlive();
					   }),
		memberEnemies_.end());
}

///=============================================================================
/// グループの活性状態確認
bool EnemyGroup::IsActive() const {
	return leaderEnemy_ != nullptr && leaderEnemy_->IsAlive() && !memberEnemies_.empty();
}

///=============================================================================
/// グループ内の生存敵数
size_t EnemyGroup::GetAliveCount() const {
	size_t count = 0;
	for (auto *member : memberEnemies_) {
		if (member && member->IsAlive()) {
			count++;
		}
	}
	return count;
}

///=============================================================================
/// フォーメーション設定の生成
FormationConfig EnemyGroup::CreateFormationConfig(FormationType type) {
	switch (type) {
	case FormationType::VFormation:
		return CreateVFormation();
	case FormationType::LineFormation:
		return CreateLineFormation();
	case FormationType::CircleFormation:
		return CreateCircleFormation();
	case FormationType::DiamondFormation:
		return CreateDiamondFormation();
	case FormationType::DynamicFormation:
		return CreateVFormation(); // デフォルトはV字
	default:
		return CreateVFormation();
	}
}

///=============================================================================
/// V字フォーメーション設定
FormationConfig EnemyGroup::CreateVFormation() {
	FormationConfig config;
	config.type = FormationType::VFormation;
	config.spacing = 30.0f;
	config.cohesionStrength = 0.8f;
	config.separationStrength = 0.5f;
	config.alignmentStrength = 0.3f;
	config.maxMemberCount = 5; // V字は最大5敵

	// V字配置: 中央のリーダーから左右に広がる
	config.offsets[0] = Vector3(0.0f, 0.0f, 0.0f);        // リーダー（先端）
	config.offsets[1] = Vector3(-30.0f, 0.0f, -30.0f);    // 左前
	config.offsets[2] = Vector3(30.0f, 0.0f, -30.0f);     // 右前
	config.offsets[3] = Vector3(-50.0f, 0.0f, -50.0f);    // 左後ろ
	config.offsets[4] = Vector3(50.0f, 0.0f, -50.0f);     // 右後ろ

	return config;
}

///=============================================================================
/// 直線フォーメーション設定
FormationConfig EnemyGroup::CreateLineFormation() {
	FormationConfig config;
	config.type = FormationType::LineFormation;
	config.spacing = 25.0f;
	config.cohesionStrength = 0.7f;
	config.separationStrength = 0.4f;
	config.alignmentStrength = 0.4f;
	config.maxMemberCount = 8; // 直線は最大8敵

	// 縦一列配置: リーダーから後ろに続く
	for (int i = 0; i < 8; i++) {
		config.offsets[i] = Vector3(0.0f, 0.0f, -config.spacing * (i + 1));
	}

	return config;
}

///=============================================================================
/// 円形フォーメーション設定
FormationConfig EnemyGroup::CreateCircleFormation() {
	FormationConfig config;
	config.type = FormationType::CircleFormation;
	config.spacing = 35.0f;
	config.cohesionStrength = 0.6f;
	config.separationStrength = 0.6f;
	config.alignmentStrength = 0.2f;
	config.maxMemberCount = 6; // 円形は最大6敵

	// 円形配置: リーダーを中心に周囲に配置
	float angleStep = 360.0f / 6.0f;
	for (int i = 0; i < 6; i++) {
		float angle = angleStep * i * 3.14159f / 180.0f; // ラジアン変換
		config.offsets[i] = Vector3(
			std::cos(angle) * config.spacing,
			0.0f,
			std::sin(angle) * config.spacing);
	}

	return config;
}

///=============================================================================
/// 菱形フォーメーション設定
FormationConfig EnemyGroup::CreateDiamondFormation() {
	FormationConfig config;
	config.type = FormationType::DiamondFormation;
	config.spacing = 32.0f;
	config.cohesionStrength = 0.75f;
	config.separationStrength = 0.55f;
	config.alignmentStrength = 0.35f;
	config.maxMemberCount = 4; // 菱形は4敵

	// 菱形配置: 上下左右に配置
	config.offsets[0] = Vector3(0.0f, 0.0f, 0.0f);           // 中央（リーダー）
	config.offsets[1] = Vector3(0.0f, 0.0f, -config.spacing);  // 前方
	config.offsets[2] = Vector3(-config.spacing, 0.0f, -config.spacing / 2.0f); // 左
	config.offsets[3] = Vector3(config.spacing, 0.0f, -config.spacing / 2.0f);  // 右

	return config;
}

///=============================================================================
/// 動的フォーメーション計算
FormationConfig EnemyGroup::CalculateDynamicFormation(const Vector3 &playerPosition) {
	// プレイヤーとの距離に応じてフォーメーションを変更
	Vector3 toPlayer = playerPosition - leaderEnemy_->GetPosition();
	float distance = Length(toPlayer);

	if (distance < 50.0f) {
		// 接近中：円形で全方位対応
		return CreateCircleFormation();
	} else if (distance < 100.0f) {
		// 中距離：菱形で安定性重視
		return CreateDiamondFormation();
	} else {
		// 遠距離：V字で機動性重視
		return CreateVFormation();
	}
}

///=============================================================================
/// メンバの目標位置計算
void EnemyGroup::CalculateMemberTargetPositions(const Vector3 &leaderPos, const Vector3 &playerPos) {
	for (int i = 0; i < static_cast<int>(memberEnemies_.size()); i++) {
		if (i >= currentFormation_.maxMemberCount) break;
		// NOTE: nullptr チェック追加（RemoveDeadMembers の前に呼ばれる可能性）
		if (!memberEnemies_[i]) continue;

		memberTargetPositions_[i] = leaderPos + currentFormation_.offsets[i];
	}
}

///=============================================================================
/// メンバの相対位置追尾更新
void EnemyGroup::UpdateMemberPositions() {
	for (int i = 1; i < static_cast<int>(memberEnemies_.size()); i++) {
		if (!memberEnemies_[i] || !memberEnemies_[i]->IsAlive()) {
			continue;
		}

		EnemyBase *member = memberEnemies_[i];
		Vector3 targetPos = memberTargetPositions_[i];

		// Boid的な群動作で目標位置に移動
		Vector3 boidForce = CalculateBoidForce(member, targetPos);

		// 移動速度計算（相対位置への移動）
		Vector3 toTarget = targetPos - member->GetPosition();
		float distanceToTarget = Length(toTarget);

		if (distanceToTarget > 1.0f) {
			// 目標位置への移動（スムージング付き）
			Vector3 moveDirection = Normalize(toTarget);
			Vector3 finalDirection = Normalize(moveDirection + boidForce * 0.3f);

			// Enemy型にキャストして編隊目標位置と追尾フラグを設定
			if (Enemy *enemy = dynamic_cast<Enemy *>(member)) {
				enemy->SetFormationTargetPosition(targetPos);
				enemy->SetFormationFollowing(true);
			}
		}
	}
}

///=============================================================================
/// Boid的な群制御（分離・結合・整列）
Vector3 EnemyGroup::CalculateBoidForce(EnemyBase *member, const Vector3 &targetPos) {
	Vector3 separation = CalculateSeparation(member);
	Vector3 cohesion = CalculateCohesion(member, targetPos);
	Vector3 alignment = CalculateAlignment(member);

	Vector3 result = separation * currentFormation_.separationStrength +
					 cohesion * currentFormation_.cohesionStrength +
					 alignment * currentFormation_.alignmentStrength;

	float resultLength = Length(result);
	return resultLength > 0.0f ? Normalize(result) : Vector3{0.0f, 0.0f, 0.0f};
}

///=============================================================================
/// 分離処理（敵同士が近づきすぎないようにする）
Vector3 EnemyGroup::CalculateSeparation(EnemyBase *member) {
	Vector3 steer{0.0f, 0.0f, 0.0f};
	int count = 0;

	for (auto *other : memberEnemies_) {
		if (!other || other == member || !other->IsAlive()) {
			continue;
		}

		Vector3 diff = member->GetPosition() - other->GetPosition();
		float distance = Length(diff);

		if (distance > 0.0f && distance < 40.0f) { // 分離の半径
			diff = Normalize(diff) / distance;
			steer = steer + diff;
			count++;
		}
	}

	if (count > 0) {
		steer = steer / static_cast<float>(count);
		float steerLength = Length(steer);
		steer = steerLength > 0.0f ? Normalize(steer) : Vector3{0.0f, 0.0f, 0.0f};
	}

	return steer;
}

///=============================================================================
/// 結合処理（敵が集団中心に寄る）
Vector3 EnemyGroup::CalculateCohesion(EnemyBase *member, const Vector3 &targetPos) {
	Vector3 toTarget = targetPos - member->GetPosition();
	float distance = Length(toTarget);

	if (distance > 1.0f) {
		return Normalize(toTarget);
	}
	return Vector3{0.0f, 0.0f, 0.0f};
}

///=============================================================================
/// 方向整列処理（敵の向きを揃える）
Vector3 EnemyGroup::CalculateAlignment(EnemyBase *member) {
	// リーダー敵から他敵への方向を計算
	// NOTE: nullptr と IsAlive チェック追加（敵削除時の安全性確保）
	if (!leaderEnemy_ || !leaderEnemy_->IsAlive()) {
		return Vector3{0.0f, 0.0f, 0.0f};
	}

	Vector3 toLeader = leaderEnemy_->GetPosition() - member->GetPosition();
	float distance = Length(toLeader);

	if (distance > 1.0f) {
		return Normalize(toLeader);
	}
	return Vector3{0.0f, 0.0f, 0.0f};
}

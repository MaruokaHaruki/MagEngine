#include "Enemy.h"
#include "component/TransformComponent.h"
#include "component/HealthComponent.h"

Enemy::Enemy() 
	: enemyTypeId_(""), enemyId_(-1), groupId_(-1), groupRole_(0), player_(nullptr) {
}

Enemy::~Enemy() {
	components_.clear();
}

void Enemy::Initialize(const std::string& enemyTypeId, const Vector3& position) {
	enemyTypeId_ = enemyTypeId;
	SetPosition(position);

	// すべてのコンポーネントの初期化完了コールバック実行
	for (auto& pair : components_) {
		pair.second->OnInitializeComplete();
	}
}

void Enemy::Update(float deltaTime) {
	// すべてのコンポーネント更新
	for (auto& pair : components_) {
		pair.second->Update(deltaTime);
	}
}

void Enemy::Draw() {
	// すべてのコンポーネント描画
	for (auto& pair : components_) {
		pair.second->Draw();
	}
}

void Enemy::DrawImGui() {
	// すべてのコンポーネントImGui表示
	for (auto& pair : components_) {
		pair.second->DrawImGui();
	}
}

Vector3 Enemy::GetPosition() const {
	auto transform = GetComponent<TransformComponent>();
	if (transform) {
		return transform->GetPosition();
	}
	return Vector3(0, 0, 0);
}

void Enemy::SetPosition(const Vector3& pos) {
	auto transform = GetComponent<TransformComponent>();
	if (transform) {
		transform->SetPosition(pos);
	}
}

Vector3 Enemy::GetVelocity() const {
	auto transform = GetComponent<TransformComponent>();
	if (transform) {
		return transform->GetVelocity();
	}
	return Vector3(0, 0, 0);
}

void Enemy::SetVelocity(const Vector3& vel) {
	auto transform = GetComponent<TransformComponent>();
	if (transform) {
		transform->SetVelocity(vel);
	}
}

int Enemy::GetCurrentHP() const {
	auto health = GetComponent<HealthComponent>();
	if (health) {
		return health->GetCurrentHP();
	}
	return 0;
}

int Enemy::GetMaxHP() const {
	auto health = GetComponent<HealthComponent>();
	if (health) {
		return health->GetMaxHP();
	}
	return 0;
}

bool Enemy::IsAlive() const {
	auto health = GetComponent<HealthComponent>();
	if (health) {
		return health->IsAlive();
	}
	return false;
}

void Enemy::TakeDamage(int damage) {
	auto health = GetComponent<HealthComponent>();
	if (health) {
		health->TakeDamage(damage);
	}
}

float Enemy::GetRadius() const {
	auto transform = GetComponent<TransformComponent>();
	if (transform) {
		return transform->GetRadius();
	}
	return 1.0f;
}

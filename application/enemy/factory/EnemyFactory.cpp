/*********************************************************************
 * \file   EnemyFactory.cpp
 * \brief  JSON設定から敵を生成するファクトリクラス
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#include "EnemyFactory.h"
#include "../Enemy.h"
#include "../component/TransformComponent.h"
#include "../component/HealthComponent.h"
#include "../component/MovementComponent.h"
#include "../component/CombatComponent.h"
#include "../component/GroupComponent.h"
#include "../component/VisualComponent.h"
#include "../component/AIComponent.h"
#include "../behavior/IAIBehavior.h"
#include "../behavior/ApproachBehavior.h"
#include "../behavior/AIBehaviors.h"
#include "../../../externals/json.hpp"
#include <fstream>
#include <sstream>

using json = nlohmann::json;

bool EnemyFactory::LoadDefinitions(const std::string& jsonPath) {
	std::ifstream file(jsonPath);
	if (!file.is_open()) {
		return false;
	}

	try {
		json data;
		file >> data;

		// JSON フォーマット: { "enemies": [ { "id": "...", "displayName": "...", ... }, ... ] }
		if (!data.contains("enemies")) {
			return false;
		}

		for (const auto& enemy : data["enemies"]) {
			EnemyDefinition def;
			def.id = enemy.value("id", "unknown");
			def.displayName = enemy.value("displayName", "Enemy");
			def.modelPath = enemy.value("modelPath", "");
			def.maxHP = enemy.value("maxHP", 3);
			def.baseSpeed = enemy.value("baseSpeed", 18.0f);
			def.scale = enemy.value("scale", 1.0f);

			definitions_[def.id] = def;
		}

		return true;
	} catch (const json::exception&) {
		return false;
	}
}

Enemy* EnemyFactory::CreateEnemy(
	const std::string& typeId,
	const Vector3& position,
	MagEngine::Object3dSetup* object3dSetup) {

	// 敵定義を取得
	const EnemyDefinition* def = GetDefinition(typeId);
	if (!def) {
		return nullptr;
	}

	// 敵インスタンスを生成
	auto enemy = new Enemy();

	// 初期化
	enemy->SetEnemyTypeId(typeId);
	enemy->SetPosition(position);

	// === コンポーネント組み立て ===

	// 1. TransformComponent
	auto transform = enemy->AddComponent<TransformComponent>();
	if (transform) {
		ComponentConfig config;
		config.SetFloat("scale_x", def->scale);
		config.SetFloat("scale_y", def->scale);
		config.SetFloat("scale_z", def->scale);
		config.SetFloat("radius", def->scale * 2.0f);
		transform->Initialize(config, enemy);
		transform->SetPosition(position);
	}

	// 2. HealthComponent
	auto health = enemy->AddComponent<HealthComponent>();
	if (health) {
		ComponentConfig config;
		config.SetInt("maxHP", def->maxHP);
		config.SetInt("currentHP", def->maxHP);
		health->Initialize(config, enemy);
	}

	// 3. MovementComponent
	auto movement = enemy->AddComponent<MovementComponent>();
	if (movement) {
		ComponentConfig config;
		config.SetFloat("baseSpeed", def->baseSpeed);
		config.SetFloat("acceleration", 10.0f);
		movement->Initialize(config, enemy);
	}

	// 4. CombatComponent
	auto combat = enemy->AddComponent<CombatComponent>();
	if (combat) {
		ComponentConfig config;
		config.SetFloat("fireRate", 1.0f);
		config.SetFloat("attackDamage", 1.0f);
		combat->Initialize(config, enemy);
	}

	// 5. GroupComponent
	auto group = enemy->AddComponent<GroupComponent>();
	if (group) {
		ComponentConfig config;
		group->Initialize(config, enemy);
	}

	// 6. VisualComponent
	auto visual = enemy->AddComponent<VisualComponent>();
	if (visual) {
		ComponentConfig config;
		config.SetString("modelPath", def->modelPath);
		visual->Initialize(config, enemy);
		// Set Object3dSetup reference if available
		// visual->SetObject3dSetup(object3dSetup);
	}

	// 7. AIComponent - デフォルトはApproachBehavior
	auto ai = enemy->AddComponent<AIComponent>();
	if (ai) {
		auto behavior = std::make_unique<ApproachBehavior>();
		ComponentConfig config;
		ai->Initialize(config, enemy);
		// AIコンポーネントに戦略を設定
		// (実装は AIComponent の SetBehavior メソッドを使用)
	}

	return enemy;
}

const EnemyDefinition* EnemyFactory::GetDefinition(const std::string& typeId) const {
	auto it = definitions_.find(typeId);
	if (it != definitions_.end()) {
		return &it->second;
	}
	return nullptr;
}

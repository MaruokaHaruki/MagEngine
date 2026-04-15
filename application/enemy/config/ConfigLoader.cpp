/*********************************************************************
 * \file   ConfigLoader.cpp
 * \brief  JSON設定ファイルの読み込みと解析の実装
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#include "ConfigLoader.h"
#include "../../../externals/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// 静的メンバの初期化
bool ConfigLoader::initialized_ = false;

bool ConfigLoader::LoadAllConfigs(const std::string &configDataPath) {
	bool success = true;

	// 各ファイルを読み込む
	success &= LoadFormationConfigs(configDataPath + "/formations.json");
	success &= LoadEnemyConfigs(configDataPath + "/enemies.json");
	success &= LoadWaveConfigs(configDataPath + "/waves.json");

	if (success) {
		initialized_ = true;
	}

	return success;
}

bool ConfigLoader::LoadEnemyConfigs(const std::string &configPath) {
	try {
		std::ifstream file(configPath);
		if (!file.is_open()) {
			std::cerr << "Failed to open: " << configPath << std::endl;
			return false;
		}

		json j;
		file >> j;
		file.close();

		std::map<std::string, EnemyParamConfig> enemyConfigs;

		// enemies配列を走査
		for (const auto &enemyJson : j["enemies"]) {
			EnemyParamConfig config;
			config.id = enemyJson["id"].get<std::string>();
			config.type = enemyJson["type"].get<std::string>();
			config.name = enemyJson["name"].get<std::string>();
			config.model = enemyJson["model"].get<std::string>();

			// stats
			config.stats.hp = enemyJson["stats"]["hp"].get<int>();
			config.stats.speed = enemyJson["stats"]["speed"].get<float>();
			config.stats.radius = enemyJson["stats"]["radius"].get<float>();

			// behavior（共通）
			if (enemyJson.contains("behavior")) {
				auto &behaviorJson = enemyJson["behavior"];
				config.behavior.approach_speed = behaviorJson.value("approach_speed", 20.0f);
				config.behavior.combat_speed = behaviorJson.value("combat_speed", 18.0f);
				config.behavior.combat_radius = behaviorJson.value("combat_radius", 40.0f);
				config.behavior.combat_depth = behaviorJson.value("combat_depth", -45.0f);
				config.behavior.combat_duration = behaviorJson.value("combat_duration", 20.0f);
				config.behavior.move_interval = behaviorJson.value("move_interval", 2.5f);
				config.behavior.retreat_speed = behaviorJson.value("retreat_speed", 25.0f);
				config.behavior.player_tracking_speed = behaviorJson.value("player_tracking_speed", 0.05f);
				config.behavior.movement_smoothing = behaviorJson.value("movement_smoothing", 0.15f);
			}

			// gunner_behavior（遠距離敵用）
			if (enemyJson.contains("gunner_behavior")) {
				auto &gunnerJson = enemyJson["gunner_behavior"];
				config.gunner_behavior.approach_speed = gunnerJson.value("approach_speed", 18.0f);
				config.gunner_behavior.combat_speed = gunnerJson.value("combat_speed", 18.0f);
				config.gunner_behavior.shooting_distance = gunnerJson.value("shooting_distance", 35.0f);
				config.gunner_behavior.shoot_interval = gunnerJson.value("shoot_interval", 1.5f);
				config.gunner_behavior.combat_duration = gunnerJson.value("combat_duration", 15.0f);
				config.gunner_behavior.combat_depth = gunnerJson.value("combat_depth", 45.0f);
				config.gunner_behavior.combat_radius = gunnerJson.value("combat_radius", 40.0f);
				config.gunner_behavior.retreat_speed = gunnerJson.value("retreat_speed", 20.0f);
			}

			enemyConfigs[config.id] = config;
		}

		EnemyParamManager::Initialize(enemyConfigs);
		std::cout << "Loaded " << enemyConfigs.size() << " enemy configs" << std::endl;
		return true;

	} catch (const std::exception &e) {
		std::cerr << "Error loading enemy configs: " << e.what() << std::endl;
		return false;
	}
}

bool ConfigLoader::LoadWaveConfigs(const std::string &configPath) {
	try {
		std::ifstream file(configPath);
		if (!file.is_open()) {
			std::cerr << "Failed to open: " << configPath << std::endl;
			return false;
		}

		json j;
		file >> j;
		file.close();

		std::vector<WaveParamConfig> waveConfigs;

		// waves配列を走査
		for (const auto &waveJson : j["waves"]) {
			WaveParamConfig config;
			config.wave_id = waveJson["wave_id"].get<int>();
			config.enemy_count = waveJson["enemy_count"].get<int>();
			config.gunner_count = waveJson["gunner_count"].get<int>();
			config.spawn_interval = waveJson["spawn_interval"].get<float>();
			config.formation_ratio = waveJson["formation_ratio"].get<float>();
			config.max_group_size = waveJson["max_group_size"].get<int>();
			config.formation_pattern = waveJson["formation_pattern"].get<std::string>();

			waveConfigs.push_back(config);
		}

		WaveParamManager::Initialize(waveConfigs);
		std::cout << "Loaded " << waveConfigs.size() << " wave configs" << std::endl;
		return true;

	} catch (const std::exception &e) {
		std::cerr << "Error loading wave configs: " << e.what() << std::endl;
		return false;
	}
}

bool ConfigLoader::LoadFormationConfigs(const std::string &configPath) {
	try {
		std::ifstream file(configPath);
		if (!file.is_open()) {
			std::cerr << "Failed to open: " << configPath << std::endl;
			return false;
		}

		json j;
		file >> j;
		file.close();

		std::map<std::string, FormationPatternConfig> formations;

		// formations オブジェクトを走査
		for (auto it = j["formations"].begin(); it != j["formations"].end(); ++it) {
			const std::string &formationId = it.key();
			const auto &formationJson = it.value();

			FormationPatternConfig config;
			config.id = formationId;
			config.type = formationJson.value("type", 0);
			config.name = formationJson.value("name", "Unknown");
			config.max_members = formationJson.value("max_members", 5);
			config.spacing = formationJson.value("spacing", 30.0f);
			config.cohesion_strength = formationJson.value("cohesion_strength", 0.7f);
			config.separation_strength = formationJson.value("separation_strength", 0.5f);
			config.alignment_strength = formationJson.value("alignment_strength", 0.3f);

			// offsets配列を読み込む
			if (formationJson.contains("offsets")) {
				for (const auto &offset : formationJson["offsets"]) {
					std::array<float, 3> offsetArray;
					offsetArray[0] = offset[0].get<float>();
					offsetArray[1] = offset[1].get<float>();
					offsetArray[2] = offset[2].get<float>();
					config.offsets.push_back(offsetArray);
				}
			}

			formations[config.id] = config;
		}

		FormationConfigManager::Initialize(formations);
		std::cout << "Loaded " << formations.size() << " formation configs" << std::endl;
		return true;

	} catch (const std::exception &e) {
		std::cerr << "Error loading formation configs: " << e.what() << std::endl;
		return false;
	}
}

bool ConfigLoader::IsInitialized() {
	return initialized_;
}

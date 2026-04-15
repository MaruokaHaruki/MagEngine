/*********************************************************************
 * \file   EnemyParamConfig.cpp
 * \brief  敵パラメータ管理の実装
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#include "EnemyParamConfig.h"

// 静的メンバの初期化
std::map<std::string, EnemyParamConfig> EnemyParamManager::enemyConfigs_;
bool EnemyParamManager::initialized_ = false;

const EnemyParamConfig *EnemyParamManager::GetEnemyConfig(const std::string &enemyId) {
	auto it = enemyConfigs_.find(enemyId);
	if (it != enemyConfigs_.end()) {
		return &it->second;
	}
	return nullptr;
}

const std::map<std::string, EnemyParamConfig> &EnemyParamManager::GetAllConfigs() {
	return enemyConfigs_;
}

void EnemyParamManager::Initialize(const std::map<std::string, EnemyParamConfig> &configs) {
	enemyConfigs_ = configs;
	initialized_ = true;
}

bool EnemyParamManager::IsInitialized() {
	return initialized_;
}

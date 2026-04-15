/*********************************************************************
 * \file   FormationConfig.cpp
 * \brief  陣形設定管理の実装
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#include "FormationConfig.h"

// 静的メンバの初期化
std::map<std::string, FormationPatternConfig> FormationConfigManager::formations_;
bool FormationConfigManager::initialized_ = false;

void FormationConfigManager::Initialize(const std::map<std::string, FormationPatternConfig> &formations) {
	formations_ = formations;
	initialized_ = true;
}

const FormationPatternConfig *FormationConfigManager::GetFormationConfig(const std::string &formationId) {
	auto it = formations_.find(formationId);
	if (it != formations_.end()) {
		return &it->second;
	}
	return nullptr;
}

const std::map<std::string, FormationPatternConfig> &FormationConfigManager::GetAllFormations() {
	return formations_;
}

bool FormationConfigManager::HasFormation(const std::string &formationId) {
	return formations_.find(formationId) != formations_.end();
}

bool FormationConfigManager::IsInitialized() {
	return initialized_;
}

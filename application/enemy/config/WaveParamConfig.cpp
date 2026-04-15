/*********************************************************************
 * \file   WaveParamConfig.cpp
 * \brief  ウェーブ設定管理の実装
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#include "WaveParamConfig.h"
#include <algorithm>

// 静的メンバの初期化
std::vector<WaveParamConfig> WaveParamManager::waveConfigs_;
bool WaveParamManager::initialized_ = false;

void WaveParamManager::Initialize(const std::vector<WaveParamConfig> &waveConfigs) {
	waveConfigs_ = waveConfigs;
	// wave_idでソート
	std::sort(waveConfigs_.begin(), waveConfigs_.end(),
		[](const WaveParamConfig &a, const WaveParamConfig &b) {
			return a.wave_id < b.wave_id;
		});
	initialized_ = true;
}

const WaveParamConfig *WaveParamManager::GetWaveConfig(int waveId) {
	auto it = std::find_if(waveConfigs_.begin(), waveConfigs_.end(),
		[waveId](const WaveParamConfig &config) {
			return config.wave_id == waveId;
		});
	if (it != waveConfigs_.end()) {
		return &(*it);
	}
	return nullptr;
}

const std::vector<WaveParamConfig> &WaveParamManager::GetAllWaveConfigs() {
	return waveConfigs_;
}

int WaveParamManager::GetWaveCount() {
	return static_cast<int>(waveConfigs_.size());
}

bool WaveParamManager::IsInitialized() {
	return initialized_;
}

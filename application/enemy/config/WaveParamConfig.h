/*********************************************************************
 * \file   WaveParamConfig.h
 * \brief  ウェーブ設定構造体（JSONから読み込み）
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#pragma once
#include <string>
#include <map>
#include <vector>

/**
 * @brief 1波分のウェーブ設定
 */
struct WaveParamConfig {
	int wave_id = 1;					// ウェーブID（1-based）
	int enemy_count = 1;				// 敵（近接）の出現数
	int gunner_count = 1;				// ガンナー（遠距離）の出現数
	float spawn_interval = 1.0f;		// スポーン間隔（秒）
	float formation_ratio = 0.0f;		// 陣形使用比率（0.0-1.0）
	int max_group_size = 5;				// 陣形グループの最大人数
	std::string formation_pattern = "v_formation";  // 陣形パターン名（formations.jsonを参照）
};

/**
 * @brief ウェーブ設定の管理クラス
 */
class WaveParamManager {
public:
	/// \brief 全ウェーブ設定を初期化
	static void Initialize(const std::vector<WaveParamConfig> &waveConfigs);

	/// \brief 指定ウェーブIDの設定を取得
	static const WaveParamConfig *GetWaveConfig(int waveId);

	/// \brief すべてのウェーブ設定を取得
	static const std::vector<WaveParamConfig> &GetAllWaveConfigs();

	/// \brief ウェーブ総数を取得
	static int GetWaveCount();

	/// \brief 初期化済みかどうかを確認
	static bool IsInitialized();

private:
	static std::vector<WaveParamConfig> waveConfigs_;
	static bool initialized_;
};

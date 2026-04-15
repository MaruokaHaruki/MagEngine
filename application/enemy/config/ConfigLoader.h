/*********************************************************************
 * \file   ConfigLoader.h
 * \brief  JSON設定ファイルの読み込みと解析
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#pragma once
#include <string>
#include <map>
#include <vector>
#include "EnemyParamConfig.h"
#include "WaveParamConfig.h"
#include "FormationConfig.h"

/**
 * @brief JSON設定ファイルの読み込みと管理
 *
 * enemy設定、wave設定、formation設定をJSONから読み込み、
 * 各マネージャーに登録する責務を持つ
 */
class ConfigLoader {
public:
	/**
	 * @brief すべての設定ファイルを読み込む
	 * 
	 * @param configDataPath JSONファイルがあるディレクトリパス
	 *        （デフォルト："application/enemy/config/data"）
	 * @return 読み込み成功時true、失敗時false
	 */
	static bool LoadAllConfigs(const std::string &configDataPath = "application/enemy/config/data");

	/**
	 * @brief 敵パラメータをJSONから読み込む
	 * 
	 * @param configPath JSONファイルへのフルパス
	 * @return 読み込み成功時true
	 */
	static bool LoadEnemyConfigs(const std::string &configPath);

	/**
	 * @brief ウェーブ設定をJSONから読み込む
	 * 
	 * @param configPath JSONファイルへのフルパス
	 * @return 読み込み成功時true
	 */
	static bool LoadWaveConfigs(const std::string &configPath);

	/**
	 * @brief 陣形設定をJSONから読み込む
	 * 
	 * @param configPath JSONファイルへのフルパス
	 * @return 読み込み成功時true
	 */
	static bool LoadFormationConfigs(const std::string &configPath);

	/**
	 * @brief 初期化済みかどうかを確認
	 * 
	 * @return すべての設定が読み込まれている場合true
	 */
	static bool IsInitialized();

private:
	static bool initialized_;
};

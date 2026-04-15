/*********************************************************************
 * \file   EnemyParamConfig.h
 * \brief  敵パラメータ設定構造体（JSONから読み込み）
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#pragma once
#include <string>
#include <map>

/**
 * @brief 敵の基本パラメータ構造体
 *
 * JSONから読み込んだパラメータをC++オブジェクトに変換
 */
struct EnemyStats {
	int hp = 3;					// ヒットポイント
	float speed = 4.0f;			// 基本速度
	float radius = 1.0f;		// 衝突判定半径
};

/**
 * @brief 敵の行動パラメータ（近接敵Enemy用）
 */
struct EnemyBehavior {
	float approach_speed = 20.0f;			// 接近速度
	float combat_speed = 18.0f;				// 戦闘中の速度
	float combat_radius = 40.0f;			// 周回半径
	float combat_depth = -45.0f;			// Z軸の戦闘位置
	float combat_duration = 20.0f;			// 戦闘継続時間
	float move_interval = 2.5f;				// 移動更新間隔
	float retreat_speed = 25.0f;			// 退却速度
	float player_tracking_speed = 0.05f;	// プレイヤー追従速度
	float movement_smoothing = 0.15f;		// 移動スムージング
};

/**
 * @brief 敵の行動パラメータ（遠距離敵EnemyGunner用）
 */
struct GunnerBehavior {
	float approach_speed = 18.0f;		// 接近速度
	float combat_speed = 18.0f;			// 戦闘中の速度
	float shooting_distance = 35.0f;	// 射撃距離
	float shoot_interval = 1.5f;		// 射撃間隔
	float combat_duration = 15.0f;		// 戦闘継続時間
	float combat_depth = 45.0f;			// Z軸の戦闘位置（プレイヤーから離れた位置）
	float combat_radius = 40.0f;		// 移動半径
	float retreat_speed = 20.0f;		// 退却速度
};

/**
 * @brief 敵の基本設定（1体分）
 */
struct EnemyParamConfig {
	std::string id;					// 敵ID（unique identifier）
	std::string type;				// 敵タイプ（"close_range" or "ranged"）
	std::string name;				// 敵名
	std::string model;				// モデルパス

	EnemyStats stats;				// ステータス
	EnemyBehavior behavior;			// 行動パラメータ（近接用）
	GunnerBehavior gunner_behavior;	// 行動パラメータ（遠距離用）
};

/**
 * @brief 敵パラメータ全体の管理クラス
 */
class EnemyParamManager {
public:
	/// \brief パラメータを取得（敵IDで指定）
	static const EnemyParamConfig *GetEnemyConfig(const std::string &enemyId);

	/// \brief すべての敵パラメータを取得
	static const std::map<std::string, EnemyParamConfig> &GetAllConfigs();

	/// \brief パラメータマップを初期化
	static void Initialize(const std::map<std::string, EnemyParamConfig> &configs);

	/// \brief 初期化済みかどうかを確認
	static bool IsInitialized();

private:
	static std::map<std::string, EnemyParamConfig> enemyConfigs_;
	static bool initialized_;
};

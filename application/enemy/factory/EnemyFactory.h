/*********************************************************************
 * \file   EnemyFactory.h
 * \brief  JSON設定から敵を生成するファクトリクラス
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include <string>
#include <map>
#include <memory>
#include "MagMath.h"

using namespace MagMath;

// Forward declarations
class Enemy;

namespace MagEngine {
	class Object3dSetup;
}

/**
 * @brief 敵定義構造体
 */
struct EnemyDefinition {
	std::string id;
	std::string displayName;
	std::string modelPath;
	int maxHP = 3;
	float baseSpeed = 18.0f;
	float scale = 1.0f;
};

/**
 * @brief JSON設定から敵を生成するファクトリ
 * 
 * 責務：
 * - JSON 設定ファイルの読み込み
 * - 敵定義の管理
 * - 敵インスタンスの生成（コンポーネント組み立て）
 */
class EnemyFactory {
public:
	EnemyFactory() = default;
	~EnemyFactory() = default;

	/**
	 * @brief 敵定義をJSON から読み込み
	 * @param jsonPath JSON ファイルパス
	 * @return 読み込み成功したかどうか
	 */
	bool LoadDefinitions(const std::string& jsonPath);

	/**
	 * @brief 敵を生成
	 * @param typeId 敵タイプID
	 * @param position スポーン位置
	 * @param object3dSetup 3D描画セットアップ
	 * @return 生成された敵（失敗時nullptr）
	 */
	Enemy* CreateEnemy(const std::string& typeId, const Vector3& position, MagEngine::Object3dSetup* object3dSetup);

	/**
	 * @brief 敵定義を取得
	 */
	const EnemyDefinition* GetDefinition(const std::string& typeId) const;

private:
	std::map<std::string, EnemyDefinition> definitions_;
};

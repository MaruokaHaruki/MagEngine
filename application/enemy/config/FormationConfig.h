/*********************************************************************
 * \file   FormationConfig.h
 * \brief  陣形設定構造体（JSONから読み込み）
 *
 * \author Harukichimaru
 * \date   April 2025
 *********************************************************************/
#pragma once
#include <string>
#include <map>
#include <vector>
#include <array>

/**
 * @brief 1つの陣形パターン設定
 * 
 * 陣形の形状、オフセット、フロッキング強度などを定義
 */
struct FormationPatternConfig {
	std::string id;						// 陣形ID（例："v_formation"）
	int type = 0;						// 陣形タイプ（列挙値）
	std::string name = "Unknown";		// 陣形名

	int max_members = 5;				// グループの最大メンバー数

	float spacing = 30.0f;				// メンバー間の距離

	// メンバーの相対位置（最大8体分）
	std::vector<std::array<float, 3>> offsets;  // [x, y, z] offsets

	// フロッキング動作パラメータ
	float cohesion_strength = 0.7f;		// 結集力（目標位置への吸引）
	float separation_strength = 0.5f;	// 分離力（他のメンバーからの距離確保）
	float alignment_strength = 0.3f;	// 整列力（リーダーの方向に合わせる）
};

/**
 * @brief 陣形設定の管理クラス
 */
class FormationConfigManager {
public:
	/// \brief すべての陣形設定を初期化
	static void Initialize(const std::map<std::string, FormationPatternConfig> &formations);

	/// \brief 指定IDの陣形設定を取得
	static const FormationPatternConfig *GetFormationConfig(const std::string &formationId);

	/// \brief すべての陣形設定を取得
	static const std::map<std::string, FormationPatternConfig> &GetAllFormations();

	/// \brief 陣形が存在するかを確認
	static bool HasFormation(const std::string &formationId);

	/// \brief 初期化済みかどうかを確認
	static bool IsInitialized();

private:
	static std::map<std::string, FormationPatternConfig> formations_;
	static bool initialized_;
};

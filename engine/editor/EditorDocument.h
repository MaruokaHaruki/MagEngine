/*********************************************************************
 * \file EditorDocument.h
 * \brief GPU実体を含まない編集文書と安定ID
 *********************************************************************/
#pragma once
#include "engine/math/structure/common/Transform.h"
#include "externals/json.hpp"
#include <cstdint>
#include <string>
#include <vector>

///=============================================================================
/// 編集データ
namespace MagEngine {
	using EditorObjectId = uint64_t;
	struct EditorObjectRecord {
		EditorObjectId id = 0;
		std::string name = "Object";
		std::string typeId = "object3d";
		std::string modelPath;
		MagMath::Transform transform{{1, 1, 1}, {}, {}}; // 回転はラジアン、座標はエンジン座標系
		bool visible = true;
		nlohmann::json settings = nlohmann::json::object();
	};
	struct EditorDocument {
		std::vector<EditorObjectRecord> objects;
		EditorObjectId nextId = 1;
		MagMath::Transform camera{{1, 1, 1}, {}, {0, 5, -20}};
	};
	///--------------------------------------------------------------
	/// 文書操作
	/// @brief 新しいIDで追加。ID枯渇時は変更せず0を返す
	EditorObjectId AddObject(EditorDocument &document, EditorObjectRecord record);
	/// @brief 文書が所有する対象。削除・vector更新まで有効。不在時nullptr
	EditorObjectRecord *FindObject(EditorDocument &document, EditorObjectId id);
	const EditorObjectRecord *FindObject(const EditorDocument &document, EditorObjectId id);
	/// @brief IDと数値・設定構造を検証。失敗理由をerrorへ返す
	bool ValidateDocument(const EditorDocument &document, std::string &error);
	/// @brief dirty判定と履歴用の値比較。ID発行の高水位は比較しない
	bool EqualDocument(const EditorDocument &left, const EditorDocument &right);
}

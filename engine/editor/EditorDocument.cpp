/*********************************************************************
 * \file EditorDocument.cpp
 * \brief 編集文書の整合性と値比較
 *********************************************************************/
#include "EditorDocument.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace MagEngine {
	namespace {
		bool Finite(const MagMath::Vector3 &value) { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }
		bool ValidTransform(const MagMath::Transform &value) { return Finite(value.translate) && Finite(value.rotate) && Finite(value.scale) && value.scale.x > 0 && value.scale.y > 0 && value.scale.z > 0; }
		bool EqualVector(const MagMath::Vector3 &a, const MagMath::Vector3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
		bool EqualTransform(const MagMath::Transform &a, const MagMath::Transform &b) { return EqualVector(a.scale, b.scale) && EqualVector(a.rotate, b.rotate) && EqualVector(a.translate, b.translate); }
	}
	///=============================================================================
	/// 文書操作
	EditorObjectId AddObject(EditorDocument &document, EditorObjectRecord record) {
		if (document.nextId == 0 || document.nextId == std::numeric_limits<EditorObjectId>::max()) { return 0; }
		record.id = document.nextId;
		document.objects.push_back(std::move(record));
		return document.nextId++;
	}
	EditorObjectRecord *FindObject(EditorDocument &document, EditorObjectId id) {
		auto found = std::find_if(document.objects.begin(), document.objects.end(), [id](const auto &object) { return object.id == id; });
		return found == document.objects.end() ? nullptr : &*found;
	}
	const EditorObjectRecord *FindObject(const EditorDocument &document, EditorObjectId id) {
		auto found = std::find_if(document.objects.begin(), document.objects.end(), [id](const auto &object) { return object.id == id; });
		return found == document.objects.end() ? nullptr : &*found;
	}
	bool ValidateDocument(const EditorDocument &document, std::string &error) {
		error.clear();
		if (!document.nextId || !ValidTransform(document.camera)) { error = "文書IDまたはカメラが不正です"; return false; }
		std::unordered_set<EditorObjectId> ids;
		for (const auto &object : document.objects) {
			if (!object.id || object.id >= document.nextId || !ids.insert(object.id).second || !ValidTransform(object.transform) || !object.settings.is_object() || object.typeId.empty()) {
				error = object.name + ": ID、Transformまたは設定構造が不正です";
				return false;
			}
		}
		return true;
	}
	bool EqualDocument(const EditorDocument &left, const EditorDocument &right) {
		if (!EqualTransform(left.camera, right.camera) || left.objects.size() != right.objects.size()) { return false; }
		for (size_t i = 0; i < left.objects.size(); ++i) {
			const auto &a = left.objects[i]; const auto &b = right.objects[i];
			if (a.id != b.id || a.name != b.name || a.typeId != b.typeId || a.modelPath != b.modelPath || a.visible != b.visible || a.settings != b.settings || !EqualTransform(a.transform, b.transform)) { return false; }
		}
		return true;
	}
}

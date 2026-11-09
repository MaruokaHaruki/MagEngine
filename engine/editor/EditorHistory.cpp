/*********************************************************************
 * \file EditorHistory.cpp
 * \brief 値の復元でもID発行の高水位を維持する
 *********************************************************************/
#include "EditorHistory.h"
#include <algorithm>
namespace MagEngine {
	///=============================================================================
	/// 履歴の操作
	bool EditorHistory::Execute(EditorDocument &document, const EditorDocument &replacement) {
		std::string error;
		if (EqualDocument(document, replacement) || !ValidateDocument(replacement, error)) { return false; }
		auto candidate = replacement;
		candidate.nextId = (std::max)(candidate.nextId, document.nextId);
		undo_.push_back(document);
		if (undo_.size() > 100) { undo_.pop_front(); }
		redo_.clear();
		document = std::move(candidate);
		return true;
	}
	bool EditorHistory::Undo(EditorDocument &document) {
		if (undo_.empty()) { return false; }
		redo_.push_back(document);
		const auto nextId = document.nextId;
		document = std::move(undo_.back()); undo_.pop_back();
		document.nextId = (std::max)(document.nextId, nextId);
		return true;
	}
	bool EditorHistory::Redo(EditorDocument &document) {
		if (redo_.empty()) { return false; }
		undo_.push_back(document);
		const auto nextId = document.nextId;
		document = std::move(redo_.back()); redo_.pop_back();
		document.nextId = (std::max)(document.nextId, nextId);
		return true;
	}
	void EditorHistory::Reset() { undo_.clear(); redo_.clear(); }
}

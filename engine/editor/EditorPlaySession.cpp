/*********************************************************************
 * \file EditorPlaySession.cpp
 * \brief 明示編集の値だけを保持しシミュレーション値を反映しない
 *********************************************************************/
#include "EditorPlaySession.h"
#include <stdexcept>
namespace MagEngine {
	///=============================================================================
	/// 保存可能プロパティ
	nlohmann::json ReadEditorProperty(const EditorObjectRecord &o, std::string_view key) {
		if (key == "name") { return o.name; }
		if (key == "model") { return o.modelPath; }
		if (key == "visible") { return o.visible; }
		if (key == "settings") { return o.settings; }
		const auto &v = key == "position" ? o.transform.translate : key == "rotation" ? o.transform.rotate : o.transform.scale;
		if (key != "position" && key != "rotation" && key != "scale") { throw std::invalid_argument("未知の設定項目"); }
		return nlohmann::json::array({v.x, v.y, v.z});
	}
	bool WriteEditorProperty(EditorObjectRecord &o, std::string_view key, const nlohmann::json &value, std::string &error) {
		try {
			auto candidate = o;
			if (key == "name") { candidate.name = value.get<std::string>(); }
			else if (key == "model") { candidate.modelPath = value.get<std::string>(); }
			else if (key == "visible") { candidate.visible = value.get<bool>(); }
			else if (key == "settings") { candidate.settings = value; }
			else {
				if (!value.is_array() || value.size() != 3) { throw std::invalid_argument("3要素の数値が必要です"); }
				MagMath::Vector3 v{value[0].get<float>(), value[1].get<float>(), value[2].get<float>()};
				if (key == "position") { candidate.transform.translate = v; }
				else if (key == "rotation") { candidate.transform.rotate = v; }
				else if (key == "scale") { candidate.transform.scale = v; }
				else { throw std::invalid_argument("未知の設定項目"); }
			}
			EditorDocument check; AddObject(check, candidate);
			if (!ValidateDocument(check, error)) { return false; }
			o = std::move(candidate); error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
	///=============================================================================
	/// 再生状態
	bool EditorPlaySession::Start(const EditorDocument &document, std::string &error) {
		if (running_ || !pending_.empty()) { error = "先に再生を停止し、反映候補を確定してください"; return false; }
		if (!ValidateDocument(document, error)) { return false; }
		auto copy = document; baseline_ = document; runtime_ = std::move(copy); undo_.clear(); redo_.clear(); running_ = true; paused_ = false; return true;
	}
	bool EditorPlaySession::RecordUserEdit(EditorObjectId id, std::string propertyKey, const nlohmann::json &value, std::string &error) {
		if (!running_) { error = "再生中ではありません"; return false; }
		auto *object = FindObject(runtime_, id); const auto *original = FindObject(baseline_, id);
		if (!object || !original) { error = "対象が存在しません"; return false; }
		auto checked = *object; if (!WriteEditorProperty(checked, propertyKey, value, error)) { return false; }
		const EditorPropertyKey key{id, propertyKey};
		UserEdit edit; edit.key = key; edit.before = ReadEditorProperty(*object, propertyKey); edit.after = value;
		if (const auto found = pending_.find(key); found != pending_.end()) { edit.pendingBefore = found->second; }
		if (!WriteEditorProperty(*object, propertyKey, value, error)) { return false; }
		const auto before = ReadEditorProperty(*original, propertyKey);
		if (value == before) { pending_.erase(key); } else { pending_[key] = {before, value}; }
		if (const auto found = pending_.find(key); found != pending_.end()) { edit.pendingAfter = found->second; }
		undo_.push_back(std::move(edit)); if (undo_.size() > 100) { undo_.erase(undo_.begin()); } redo_.clear();
		return true;
	}
	void EditorPlaySession::Stop() { running_ = false; paused_ = false; }
	bool EditorPlaySession::UndoUserEdit(std::string &error) {
		if (!running_ || undo_.empty()) { return false; }
		const auto edit = undo_.back(); auto *object = FindObject(runtime_, edit.key.objectId);
		if (!object || !WriteEditorProperty(*object, edit.key.property, edit.before, error)) { return false; }
		if (edit.pendingBefore) { pending_[edit.key] = *edit.pendingBefore; } else { pending_.erase(edit.key); }
		redo_.push_back(edit); undo_.pop_back(); return true;
	}
	bool EditorPlaySession::RedoUserEdit(std::string &error) {
		if (!running_ || redo_.empty()) { return false; }
		const auto edit = redo_.back(); auto *object = FindObject(runtime_, edit.key.objectId);
		if (!object || !WriteEditorProperty(*object, edit.key.property, edit.after, error)) { return false; }
		if (edit.pendingAfter) { pending_[edit.key] = *edit.pendingAfter; } else { pending_.erase(edit.key); }
		undo_.push_back(edit); redo_.pop_back(); return true;
	}
	bool EditorPlaySession::ApplySelected(EditorDocument &document, std::span<const EditorPropertyKey> keys, EditorHistory &history, std::string &error) {
		if (running_) { error = "停止してから反映してください"; return false; }
		auto candidate = document;
		for (const auto &key : keys) {
			const auto found = pending_.find(key); auto *object = FindObject(candidate, key.objectId);
			if (found == pending_.end() || !object) { error = "反映対象が見つかりません"; return false; }
			if (!WriteEditorProperty(*object, key.property, found->second.after, error)) { return false; }
		}
		if (!ValidateDocument(candidate, error)) { return false; }
		history.Execute(document, candidate);
		for (const auto &key : keys) { pending_.erase(key); }
		error.clear(); return true;
	}
	void EditorPlaySession::DiscardPending() { if (!running_) { pending_.clear(); } }
	bool CommitEditorProperty(EditorDocument &document, EditorHistory &history, EditorPlaySession *session, const EditorPropertyKey &key, const nlohmann::json &before, const nlohmann::json &after, bool cancel, std::string &error) {
		auto *object = FindObject(document, key.objectId);
		if (!object) { error = "編集中の対象が存在しません"; return false; }
		if (!WriteEditorProperty(*object, key.property, before, error)) { return false; }
		if (cancel) { return true; }
		if (session && session->IsRunning()) { return session->RecordUserEdit(key.objectId, key.property, after, error); }
		auto candidate = document;
		if (!WriteEditorProperty(*FindObject(candidate, key.objectId), key.property, after, error)) { return false; }
		history.Execute(document, candidate); return true;
	}
}

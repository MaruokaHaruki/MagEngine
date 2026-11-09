/*********************************************************************
 * \file EditorPlaySession.h
 * \brief 編集文書から独立した再生値と選択反映
 *********************************************************************/
#pragma once
#include "EditorHistory.h"
#include <map>
#include <span>
#include <optional>
namespace MagEngine {
	struct EditorPropertyKey {
		EditorObjectId objectId = 0;
		std::string property;
		bool operator<(const EditorPropertyKey &other) const { return objectId != other.objectId ? objectId < other.objectId : property < other.property; }
	};
	struct EditorPropertyChange { nlohmann::json before; nlohmann::json after; };
	/// @brief 保存可能プロパティの読み書き。失敗時は対象を変更しない
	bool WriteEditorProperty(EditorObjectRecord &object, std::string_view key, const nlohmann::json &value, std::string &error);
	nlohmann::json ReadEditorProperty(const EditorObjectRecord &object, std::string_view key);
	class EditorPlaySession {
	public:
		bool Start(const EditorDocument &document, std::string &error);
		void Pause(bool value) { paused_ = value; }
		bool IsRunning() const { return running_; }
		bool IsPaused() const { return paused_; }
		/// @brief 再生用の値。GPU実体はGame側Hostが所有する
		EditorDocument &Runtime() { return runtime_; }
		const std::map<EditorPropertyKey, EditorPropertyChange> &Pending() const { return pending_; }
		bool RecordUserEdit(EditorObjectId id, std::string propertyKey, const nlohmann::json &value, std::string &error);
		void Stop();
		bool UndoUserEdit(std::string &error);
		bool RedoUserEdit(std::string &error);
		bool ApplySelected(EditorDocument &document, std::span<const EditorPropertyKey> keys, EditorHistory &history, std::string &error);
		void DiscardPending();
	private:
		EditorDocument baseline_;
		EditorDocument runtime_;
		std::map<EditorPropertyKey, EditorPropertyChange> pending_;
		bool running_ = false;
		bool paused_ = false;
		struct UserEdit {
			EditorPropertyKey key;
			nlohmann::json before, after;
			std::optional<EditorPropertyChange> pendingBefore, pendingAfter;
		};
		std::vector<UserEdit> undo_, redo_;
	};
	/// @brief 1つの入力操作だけを確定／取消する。シミュレーションによる他項目の変化は保持
	bool CommitEditorProperty(EditorDocument &document, EditorHistory &history, EditorPlaySession *session, const EditorPropertyKey &key, const nlohmann::json &before, const nlohmann::json &after, bool cancel, std::string &error);
}

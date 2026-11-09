/*********************************************************************
 * \file EditorPanels.h
 * \brief 選択・設定・資産・保存・再生をつなぐ制作パネル
 *********************************************************************/
#pragma once
#include "EditorPlaySession.h"
#include "EditorTypeRegistry.h"
#include <functional>
#include <filesystem>
namespace MagEngine {
	class EditorUiSystem;
	struct EditorPanelActions {
		std::function<void()> save, load, importBlender, play, stop;
		std::function<uint64_t()> sceneTexture;
		std::function<EditorBounds(const EditorObjectRecord &)> bounds;
	};
	///=============================================================================
	/// 制作画面。参照先とコールバックの所有者はGame側Hostで、解除まで存続する
	class EditorPanels {
	public:
		EditorPanels(EditorDocument &document, EditorDocument &saved, EditorHistory &history, EditorPlaySession &session, EditorTypeRegistry &registry, std::filesystem::path root);
		void Register(EditorUiSystem &system);
		void Unregister(EditorUiSystem &system);
		EditorPanelActions actions;
		std::string error;
		std::string sceneName = "scene.json";
		EditorObjectId selected = 0;
		bool viewportFocused = false;
		/// @brief 前フレームのフォーカスに加え、現在の入力位置が画像内か確認する
		bool ContainsViewportPoint(MagMath::Vector2 point) const { return viewportFocused && point.x >= imageRect_.x && point.y >= imageRect_.y && point.x < imageRect_.x + imageRect_.width && point.y < imageRect_.y + imageRect_.height; }
		/// @brief 編集確定前の候補も含む現在の文書
		EditorDocument &ActiveDocument();
		/// @brief 終了やモード切替より先に、現在の入力操作を確定する
		void FinishEditing() { FlushEdit(); }
	private:
		void DrawToolbar();
		void DrawHierarchy();
		void DrawInspector();
		void DrawAssets();
		void DrawViewport();
		void Commit(EditorDocument candidate);
		void TrackEdit(bool changed, const EditorDocument &before, std::string_view property);
		void FlushEdit(bool cancel = false);
		EditorDocument &document_, &saved_;
		EditorHistory &history_;
		EditorPlaySession &session_;
		EditorTypeRegistry &registry_;
		std::filesystem::path root_;
		std::vector<std::string> assets_;
		std::string search_;
		EditorDocument editStart_;
		EditorPropertyKey editKey_;
		nlohmann::json editValue_;
		bool editing_ = false;
		int mode_ = 0;
		int axis_ = -1;
		float dragStart_ = 0;
		MagMath::Vector3 dragValue_{};
		std::vector<EditorPropertyKey> selectedChanges_;
		ViewportRect imageRect_;
	};
}

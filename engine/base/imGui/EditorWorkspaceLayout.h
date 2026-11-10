/*********************************************************************
 * \file EditorWorkspaceLayout.h
 * \brief 既定のドック配置と分類色。内部ImGui依存は実装に限定する
 *********************************************************************/
#pragma once
#include "EditorPanelRegistry.h"
#include "imgui.h"
#include <array>
namespace MagEngine {
	enum class EditorLayoutPreset { Standard, SceneFocus, Debug };
	enum class EditorWorkspaceMode { Edit, Game };
	const char *EditorCategoryName(EditorUiCategory category);
	ImVec4 EditorCategoryColor(EditorUiCategory category);
	const char *EditorPresetName(EditorLayoutPreset preset);
	bool EditorPresetOpens(EditorLayoutPreset preset, const EditorUiPanelDesc &panel);
	class EditorWorkspaceLayout {
	public:
		/// @brief UIフレーム開始直後、パネルのBeginより先にだけ呼ぶ
		void Build(EditorLayoutPreset, const EditorPanelRegistry &, ImGuiID dockspace, ImVec2 size);
		void DockNewPanel(const EditorUiPanelDesc &, ImGuiID dockspace, const EditorPanelRegistry *registry = nullptr);
	private:
		std::array<ImGuiID, 4> regions_{};
	};
}

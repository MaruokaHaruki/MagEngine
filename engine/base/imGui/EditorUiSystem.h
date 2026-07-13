/*********************************************************************
 * \file   EditorUiSystem.h
 * \brief  Editor / Debug UI の一元管理
 *
 * \author Harukichimaru
 * \date   July 2026
 * \note   NOTE: Debug UI の追加経路を登録制に統一し、Framework と Scene から個別 ImGui 知識を分離する
 *********************************************************************/
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace MagEngine {

	enum class EditorUiCategory {
		Engine,
		Scene,
		Rendering,
		Tools,
		Application
	};

	struct EditorState {
		bool isEditorEnabled = true;
		bool isDockSpaceEnabled = true;
		bool isMenuBarEnabled = true;
	};

	struct EditorUiPanelDesc {
		std::string name;
		EditorUiCategory category = EditorUiCategory::Tools;
		bool isOpen = false;
		std::function<void()> drawFunc;
	};

	class EditorUiSystem {
	public:
		void Initialize();
		void Finalize();

		void RegisterPanel(const std::string &name, EditorUiCategory category, bool defaultOpen, std::function<void()> drawFunc);
		void UnregisterPanel(const std::string &name);
		void ClearScenePanels();

		void SetPanelOpen(const std::string &name, bool isOpen);
		bool IsPanelOpen(const std::string &name) const;

		void Draw();

	private:
		void DrawDockSpace();
		void DrawMainMenuBar();
		void DrawPanels();
		void DrawCategoryMenu(EditorUiCategory category);
		EditorUiPanelDesc *FindPanel(const std::string &name);
		const EditorUiPanelDesc *FindPanel(const std::string &name) const;

		EditorState editorState_;
		std::vector<EditorUiPanelDesc> panels_;
	};
}

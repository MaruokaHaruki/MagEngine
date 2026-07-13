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
	struct EngineContext;

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
		bool defaultOpen = false;
		bool isOpen = false;
		// Panel本体のBegin/EndはEditorUiSystemが所有し、登録側は内容だけを描画する。
		std::function<void()> drawFunc;
	};

	class EditorUiSystem {
	public:
		void Initialize();
		void Finalize();
		/// @brief Engine常駐Panelを登録
		/// @param engineContext Engineサービスの非所有参照
		/// @param renderingPanel Framework固有のRendering UI本体
		/// @param performancePanel Framework固有の性能UI本体
		/// @note Managerの寿命はFrameworkが所有し、Panel登録はその後に行う
		void RegisterEnginePanels(const EngineContext &engineContext,
			std::function<void()> renderingPanel,
			std::function<void()> performancePanel);

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

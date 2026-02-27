/*********************************************************************
 * \file   HierarchyPanel.cpp
 * \brief  シーンオブジェクト階層パネル実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "HierarchyPanel.h"
#include "EditorState.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	void HierarchyPanel::Initialize(EditorState *editorState, DirectXCore *dxCore) {
		BasePanel::Initialize(editorState, dxCore);
		ClearObjects();
	}

	void HierarchyPanel::Draw() {
		if (!isVisible_)
			return;

		BeginPanel(200, 300);
		if (BeginPanelWindow()) {
			DrawSectionHeader("Panel Not Active");
			ImGui::TextDisabled("Hierarchy は現在使用されていません。");
		}
		EndPanel();
	}

	void HierarchyPanel::Update() {
	}

	void HierarchyPanel::AddObject(const std::string &name, void *objectPtr, int depth) {
	}

	void HierarchyPanel::ClearObjects() {
		sceneObjects_.clear();
	}

	void HierarchyPanel::RefreshHierarchy() {
	}

}

/*********************************************************************
 * \file   InspectorPanel.cpp
 * \brief  オブジェクトプロパティ検査パネル実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "InspectorPanel.h"
#include "EditorState.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	void InspectorPanel::Initialize(EditorState *editorState, DirectXCore *dxCore) {
		BasePanel::Initialize(editorState, dxCore);
	}

	void InspectorPanel::Draw() {
		if (!isVisible_)
			return;

		BeginPanel(250, 300);
		if (BeginPanelWindow()) {
			DrawSectionHeader("Panel Not Active");
			ImGui::TextDisabled("Inspector は現在使用されていません。");
		}
		EndPanel();
	}

	void InspectorPanel::Update() {
	}

}

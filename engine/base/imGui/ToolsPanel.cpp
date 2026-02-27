/*********************************************************************
 * \file   ToolsPanel.cpp
 * \brief  ツール・パラメータ調整パネル実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "ToolsPanel.h"
#include "EditorState.h"
#include "PostEffectManager.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	void ToolsPanel::Initialize(EditorState *editorState, DirectXCore *dxCore) {
		BasePanel::Initialize(editorState, dxCore);
	}

	void ToolsPanel::Draw() {
		if (!isVisible_)
			return;

		BeginPanel(250, 300);
		if (BeginPanelWindow()) {

			DrawSectionHeader("Viewport Settings");

			ImGui::Checkbox("Show Grid##grid", &editorState_->showGridInViewport);
			ImGui::Checkbox("Show Debug Info##info", &editorState_->showDebugInfoInViewport);

			ImGui::Spacing();
			DrawSectionHeader("Game Settings");

			static float timeScale = 1.0f;
			ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 2.0f);

			ImGui::Spacing();
			DrawSectionHeader("Editor");

			ImGui::Checkbox("Enable DockSpace", &editorState_->dockspaceEnabled);
			ImGui::Checkbox("Show Menu Bar", &editorState_->menuBarVisible);
		}
		EndPanel();
	}

	void ToolsPanel::Update() {
	}

	void ToolsPanel::SetPostEffectManager(PostEffectManager *postEffectManager) {
		postEffectManager_ = postEffectManager;
	}

}

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

		ImGui::SetNextWindowSizeConstraints(ImVec2(250, 300), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::Begin(panelName_.c_str(), &isVisible_)) {

			if (ImGui::CollapsingHeader("Post Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawPostEffectControls();
			}

			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Game Settings")) {
				DrawGameSettings();
			}

			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Debug Settings")) {
				DrawDebugSettings();
			}
		}
		ImGui::End();
	}

	void ToolsPanel::Update() {
		// ツール関連の更新処理
	}

	void ToolsPanel::DrawPostEffectControls() {
		ImGui::Indent();

		static float vignetteStrength = 0.5f;
		static float bloomThreshold = 1.0f;

		if (ImGui::CollapsingHeader("Vignetting")) {
			ImGui::SliderFloat("Strength##vignette", &vignetteStrength, 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Bloom")) {
			ImGui::SliderFloat("Threshold##bloom", &bloomThreshold, 0.0f, 2.0f);
		}

		if (ImGui::CollapsingHeader("Grayscale")) {
			static float grayscaleAmount = 0.0f;
			ImGui::SliderFloat("Amount##grayscale", &grayscaleAmount, 0.0f, 1.0f);
		}

		ImGui::Unindent();
	}

	void ToolsPanel::DrawGameSettings() {
		ImGui::Indent();

		static float timeScale = 1.0f;
		static bool showColliders = false;
		static bool showPhysics = false;

		ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 2.0f);
		ImGui::Checkbox("Show Colliders##collider", &showColliders);
		ImGui::Checkbox("Show Physics##physics", &showPhysics);

		ImGui::Unindent();
	}

	void ToolsPanel::DrawDebugSettings() {
		ImGui::Indent();

		ImGui::Checkbox("Show Grid##grid", &editorState_->showGridInViewport);
		ImGui::Checkbox("Show Debug Info##info", &editorState_->showDebugInfoInViewport);

		static bool showPerformance = false;
		ImGui::Checkbox("Show Performance", &showPerformance);

		ImGui::Unindent();
	}

}

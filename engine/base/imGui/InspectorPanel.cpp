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

		ImGui::SetNextWindowSizeConstraints(ImVec2(250, 300), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::Begin(panelName_.c_str(), &isVisible_)) {

			if (!editorState_->IsObjectSelected()) {
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No object selected");
			} else {
				DrawObjectInfo();
				ImGui::Separator();
				DrawTransformProperties();
				ImGui::Separator();
				DrawMaterialProperties();
				ImGui::Separator();
				DrawComponentProperties();
			}
		}
		ImGui::End();
	}

	void InspectorPanel::Update() {
		// インスペクター関連の更新処理
	}

	void InspectorPanel::DrawObjectInfo() {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Object");
		ImGui::Indent();
		ImGui::Text("Name: %s", editorState_->selectedObjectName.c_str());
		ImGui::Text("Address: 0x%p", editorState_->selectedObject);
		ImGui::Unindent();
	}

	void InspectorPanel::DrawTransformProperties() {
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent();

			static float position[3] = {0.0f, 0.0f, 0.0f};
			static float rotation[3] = {0.0f, 0.0f, 0.0f};
			static float scale[3] = {1.0f, 1.0f, 1.0f};

			ImGui::DragFloat3("Position##pos", position, 0.1f);
			ImGui::DragFloat3("Rotation##rot", rotation, 0.1f);
			ImGui::DragFloat3("Scale##scl", scale, 0.1f);

			ImGui::Unindent();
		}
	}

	void InspectorPanel::DrawMaterialProperties() {
		if (ImGui::CollapsingHeader("Material")) {
			ImGui::Indent();

			static ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
			static float metallic = 0.0f;
			static float roughness = 0.5f;

			ImGui::ColorEdit4("Color", (float *)&color);
			ImGui::SliderFloat("Metallic##met", &metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness##rough", &roughness, 0.0f, 1.0f);

			ImGui::Unindent();
		}
	}

	void InspectorPanel::DrawComponentProperties() {
		if (ImGui::CollapsingHeader("Components")) {
			ImGui::Indent();
			ImGui::Text("No components attached");
			ImGui::Unindent();
		}
	}

}

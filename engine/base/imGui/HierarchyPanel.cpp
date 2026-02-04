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

		ImGui::SetNextWindowSizeConstraints(ImVec2(200, 300), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::Begin(panelName_.c_str(), &isVisible_)) {

			// ツールバー
			if (ImGui::Button("Refresh")) {
				RefreshHierarchy();
			}
			ImGui::SameLine();
			ImGui::Checkbox("Auto Refresh", &autoRefresh_);
			ImGui::Checkbox("Show Only Active", &showOnlyActive_);

			ImGui::Separator();

			// オブジェクトツリー表示
			for (const auto &obj : sceneObjects_) {
				DrawObjectNode(obj);
			}

			// 検索ボックス
			ImGui::Separator();
			static char searchBuffer[128] = "";
			ImGui::InputTextWithHint("##search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
		}
		ImGui::End();
	}

	void HierarchyPanel::Update() {
		if (autoRefresh_) {
			// 定期的に階層をリフレッシュ（パフォーマンス考慮）
		}
	}

	void HierarchyPanel::AddObject(const std::string &name, void *objectPtr, int depth) {
		SceneObjectInfo info{};
		info.name = name;
		info.objectPtr = objectPtr;
		info.isExpanded = false;
		info.depth = depth;
		sceneObjects_.push_back(info);
	}

	void HierarchyPanel::ClearObjects() {
		sceneObjects_.clear();
	}

	void HierarchyPanel::RefreshHierarchy() {
		// TODO: シーンマネージャからオブジェクト情報を取得して更新
	}

	void HierarchyPanel::DrawObjectNode(const SceneObjectInfo &obj) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
		if (editorState_->selectedObject == obj.objectPtr) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool nodeOpen = ImGui::TreeNodeEx(obj.name.c_str(), flags);

		if (ImGui::IsItemClicked()) {
			editorState_->SetSelectedObject(obj.objectPtr, obj.name);
		}

		if (nodeOpen) {
			// 子ノードがあれば描画
			ImGui::TreePop();
		}
	}

}

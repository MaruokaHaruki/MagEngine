/*********************************************************************
 * \file EditorWorkspaceLayout.cpp
 * \brief 同梱ImGuiのDockBuilderをこの実装だけで使う
 *********************************************************************/
#include "EditorWorkspaceLayout.h"
#include "EditorTheme.h"
#include "imgui_internal.h"
#include <algorithm>
namespace MagEngine {
	const char *EditorCategoryName(EditorUiCategory category) {
		switch (category) {
		case EditorUiCategory::Authoring: return "制作";
		case EditorUiCategory::Engine: return "エンジン";
		case EditorUiCategory::GameDebug: return "ゲームデバッグ";
		default: return "ツール";
		}
	}
	ImVec4 EditorCategoryColor(EditorUiCategory category) { return EditorThemeCategoryColor(category); }
	const char *EditorPresetName(EditorLayoutPreset preset) { return preset == EditorLayoutPreset::Standard ? "標準" : preset == EditorLayoutPreset::SceneFocus ? "画面重視" : "デバッグ"; }
	bool EditorPresetOpens(EditorLayoutPreset preset, const EditorUiPanelDesc &panel) {
		if (panel.id == "workspace.viewport") { return true; }
		if (preset == EditorLayoutPreset::SceneFocus) { return false; }
		if (preset == EditorLayoutPreset::Debug) { return panel.id == "engine.performance" || panel.id == "workspace.messages"; }
		return panel.category == EditorUiCategory::Authoring && panel.defaultOpen;
	}
	///=============================================================================
	/// ドックツリーを明示適用時だけ作る
	void EditorWorkspaceLayout::Build(EditorLayoutPreset preset, const EditorPanelRegistry &registry, ImGuiID dockspace, ImVec2 size) {
		ImGui::DockBuilderRemoveNode(dockspace);
		ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace, {(std::max)(size.x, 1.0f), (std::max)(size.y, 1.0f)});
		ImGuiID center = dockspace; regions_.fill(dockspace);
		if (preset != EditorLayoutPreset::SceneFocus) {
			if (registry.Find("authoring.objects") && preset == EditorLayoutPreset::Standard) { regions_[0] = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.20f, nullptr, &center); }
			const float rightRatio = registry.Find("authoring.objects") && preset == EditorLayoutPreset::Standard ? 0.25f / 0.80f : 0.25f;
			regions_[2] = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, rightRatio, nullptr, &center);
			regions_[3] = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, nullptr, &center);
		}
		regions_[1] = center;
		for (const auto &panel : registry.Panels()) {
			const auto name = panel.name + "###" + panel.id;
			ImGui::DockBuilderDockWindow(name.c_str(), regions_[static_cast<size_t>(panel.defaultRegion)]);
		}
		ImGui::DockBuilderFinish(dockspace);
	}
	void EditorWorkspaceLayout::DockNewPanel(const EditorUiPanelDesc &panel, ImGuiID dockspace, const EditorPanelRegistry *registry) {
		const auto name = panel.name + "###" + panel.id;
		// 保存済みの配置があれば、翻訳やScene再生成で標準位置へ戻さない。
		const auto *settings = ImGui::FindWindowSettingsByID(ImHashStr(name.c_str()));
		if (settings) { return; }
		// 復元したツリーでは以前のプリセットのノードIDを使わない。
		// 現在の葉領域から標準の方向に最も近いものを選ぶ。
		ImGuiID target = dockspace;
		float best = -1e30f;
		auto visit = [&](auto &&self, ImGuiDockNode *node) -> void {
			if (!node) { return; }
			if (node->ChildNodes[0] || node->ChildNodes[1]) { self(self, node->ChildNodes[0]); self(self, node->ChildNodes[1]); return; }
			float score = 0;
			switch (panel.defaultRegion) {
			case EditorDockRegion::Left: score = -node->Pos.x; break;
			case EditorDockRegion::Right: score = node->Pos.x + node->Size.x * 0.5f; break;
			case EditorDockRegion::Bottom: score = node->Pos.y + node->Size.y * 0.5f; break;
			case EditorDockRegion::Center: score = node->IsCentralNode() ? 1.0f : -1.0f; break;
			}
			if (score > best) { best = score; target = node->ID; }
		};
		visit(visit, ImGui::DockBuilderGetNode(dockspace));
		if (registry) {
			for (const auto &sibling : registry->Panels()) {
				if (sibling.id == panel.id || sibling.defaultRegion != panel.defaultRegion) { continue; }
				const auto siblingName = sibling.name + "###" + sibling.id;
				const auto *saved = ImGui::FindWindowSettingsByID(ImHashStr(siblingName.c_str()));
				if (saved && saved->DockId && ImGui::DockBuilderGetNode(saved->DockId)) { target = saved->DockId; break; }
			}
		}
		ImGui::DockBuilderDockWindow(name.c_str(), target);
	}
}

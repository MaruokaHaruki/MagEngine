/*********************************************************************
 * \file   EditorUiSystem.cpp
 * \brief  Editor / Debug UI の一元管理実装
 *
 * \author Harukichimaru
 * \date   July 2026
 *********************************************************************/
#include "EditorUiSystem.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cassert>

namespace MagEngine {
	namespace {
		const char *ToCategoryName(EditorUiCategory category) {
			switch (category) {
			case EditorUiCategory::Engine:
				return "Engine";
			case EditorUiCategory::Scene:
				return "Scene";
			case EditorUiCategory::Rendering:
				return "Rendering";
			case EditorUiCategory::Tools:
				return "Tools";
			case EditorUiCategory::Application:
				return "Application";
			default:
				return "Unknown";
			}
		}
	}

	void EditorUiSystem::Initialize() {
		// NOTE: Docking は EditorUiSystem が唯一の入口として有効化し、旧 Layout 側へ責務を残さない。
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	void EditorUiSystem::Finalize() {
		// NOTE: std::function のキャプチャが破棄済み Scene を参照しないよう、終了時に登録を全て破棄する。
		panels_.clear();
	}

	void EditorUiSystem::RegisterPanel(const std::string &name, EditorUiCategory category, bool defaultOpen, std::function<void()> drawFunc) {
		if (name.empty() || !drawFunc) {
			assert(false && "EditorUiPanel requires a unique name and draw function.");
			return;
		}

		if (EditorUiPanelDesc *panel = FindPanel(name)) {
			// NOTE: 同名 Panel は一意キーとして扱い、Scene 再生成時の重複登録を上書きで防ぐ。
			panel->category = category;
			panel->isOpen = defaultOpen;
			panel->drawFunc = std::move(drawFunc);
			return;
		}

		EditorUiPanelDesc panel{};
		panel.name = name;
		panel.category = category;
		panel.isOpen = defaultOpen;
		panel.drawFunc = std::move(drawFunc);
		panels_.push_back(std::move(panel));
	}

	void EditorUiSystem::UnregisterPanel(const std::string &name) {
		panels_.erase(
			std::remove_if(panels_.begin(), panels_.end(), [&name](const EditorUiPanelDesc &panel) {
				return panel.name == name;
			}),
			panels_.end());
	}

	void EditorUiSystem::ClearScenePanels() {
		// NOTE: Scene 切替時にキャプチャ済み this が古い Scene を参照しないよう、Scene カテゴリを一括破棄する。
		panels_.erase(
			std::remove_if(panels_.begin(), panels_.end(), [](const EditorUiPanelDesc &panel) {
				return panel.category == EditorUiCategory::Scene;
			}),
			panels_.end());
	}

	void EditorUiSystem::SetPanelOpen(const std::string &name, bool isOpen) {
		if (EditorUiPanelDesc *panel = FindPanel(name)) {
			panel->isOpen = isOpen;
		}
	}

	bool EditorUiSystem::IsPanelOpen(const std::string &name) const {
		if (const EditorUiPanelDesc *panel = FindPanel(name)) {
			return panel->isOpen;
		}
		return false;
	}

	void EditorUiSystem::Draw() {
		if (!editorState_.isEditorEnabled) {
			return;
		}

		if (editorState_.isDockSpaceEnabled) {
			DrawDockSpace();
		}
		if (editorState_.isMenuBarEnabled) {
			DrawMainMenuBar();
		}
		DrawPanels();
	}

	void EditorUiSystem::DrawDockSpace() {
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
									   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
									   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if (ImGui::Begin("Editor DockSpace", nullptr, windowFlags)) {
			ImGuiID dockspaceId = ImGui::GetID("EditorMainDockSpace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		}
		ImGui::End();
		ImGui::PopStyleVar(3);
	}

	void EditorUiSystem::DrawMainMenuBar() {
		if (!ImGui::BeginMainMenuBar()) {
			return;
		}

		if (ImGui::BeginMenu("Window")) {
			DrawCategoryMenu(EditorUiCategory::Engine);
			DrawCategoryMenu(EditorUiCategory::Scene);
			DrawCategoryMenu(EditorUiCategory::Rendering);
			DrawCategoryMenu(EditorUiCategory::Tools);
			DrawCategoryMenu(EditorUiCategory::Application);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Editor")) {
			ImGui::MenuItem("Enable DockSpace", nullptr, &editorState_.isDockSpaceEnabled);
			ImGui::MenuItem("Show Menu Bar", nullptr, &editorState_.isMenuBarEnabled);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	void EditorUiSystem::DrawPanels() {
		for (EditorUiPanelDesc &panel : panels_) {
			if (!panel.isOpen || !panel.drawFunc) {
				continue;
			}

			bool isOpen = panel.isOpen;
			if (ImGui::Begin(panel.name.c_str(), &isOpen)) {
				panel.drawFunc();
			}
			ImGui::End();
			panel.isOpen = isOpen;
		}
	}

	void EditorUiSystem::DrawCategoryMenu(EditorUiCategory category) {
		if (!ImGui::BeginMenu(ToCategoryName(category))) {
			return;
		}

		for (EditorUiPanelDesc &panel : panels_) {
			if (panel.category != category) {
				continue;
			}
			ImGui::MenuItem(panel.name.c_str(), nullptr, &panel.isOpen);
		}

		ImGui::EndMenu();
	}

	EditorUiPanelDesc *EditorUiSystem::FindPanel(const std::string &name) {
		auto it = std::find_if(panels_.begin(), panels_.end(), [&name](const EditorUiPanelDesc &panel) {
			return panel.name == name;
		});
		return it != panels_.end() ? &(*it) : nullptr;
	}

	const EditorUiPanelDesc *EditorUiSystem::FindPanel(const std::string &name) const {
		auto it = std::find_if(panels_.begin(), panels_.end(), [&name](const EditorUiPanelDesc &panel) {
			return panel.name == name;
		});
		return it != panels_.end() ? &(*it) : nullptr;
	}
}

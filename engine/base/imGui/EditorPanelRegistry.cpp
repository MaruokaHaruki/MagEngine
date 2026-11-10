/*********************************************************************
 * \file EditorPanelRegistry.cpp
 * \brief 安定IDによる登録とScene所有参照の解除
 *********************************************************************/
#include "EditorPanelRegistry.h"
#include <algorithm>
namespace MagEngine {
	bool EditorPanelRegistry::Register(EditorUiPanelDesc panel) {
		if (panel.id.empty() || panel.name.empty() || !panel.drawFunc || Find(panel.id)) { return false; }
		panel.isOpen = panel.defaultOpen; panels_.push_back(std::move(panel)); return true;
	}
	void EditorPanelRegistry::Unregister(std::string_view id) { std::erase_if(panels_, [id](const auto &panel) { return panel.id == id; }); }
	void EditorPanelRegistry::ClearScenePanels() { std::erase_if(panels_, [](const auto &panel) { return panel.scope == EditorPanelScope::Scene; }); }
	EditorUiPanelDesc *EditorPanelRegistry::Find(std::string_view id) {
		auto found = std::find_if(panels_.begin(), panels_.end(), [id](const auto &panel) { return panel.id == id; }); return found == panels_.end() ? nullptr : &*found;
	}
	const EditorUiPanelDesc *EditorPanelRegistry::Find(std::string_view id) const {
		auto found = std::find_if(panels_.begin(), panels_.end(), [id](const auto &panel) { return panel.id == id; }); return found == panels_.end() ? nullptr : &*found;
	}
}

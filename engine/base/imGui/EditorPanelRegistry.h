/*********************************************************************
 * \file EditorPanelRegistry.h
 * \brief パネルの表示分類と所有者の寿命を分けて保持する
 *********************************************************************/
#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>
namespace MagEngine {
	///=============================================================================
	/// パネルの契約
	enum class EditorUiCategory { Authoring, Engine, GameDebug, Tools };
	enum class EditorPanelScope { Engine, Scene };
	enum class EditorDockRegion { Left, Center, Right, Bottom };
	struct EditorUiPanelDesc {
		std::string id; // 翻訳や表示名変更で変えない保存用ID
		std::string name;
		EditorUiCategory category = EditorUiCategory::Tools;
		EditorPanelScope scope = EditorPanelScope::Engine;
		EditorDockRegion defaultRegion = EditorDockRegion::Right;
		bool defaultOpen = false;
		bool isOpen = false;
		std::function<void()> drawFunc; // 所有元が存続中のみ有効。解除は所有元の破棄より先
	};
	class EditorPanelRegistry {
	public:
		/// @brief 空ID・重複・無効コールバックは拒否。別の表示名は同名でも登録できる
		bool Register(EditorUiPanelDesc panel);
		void Unregister(std::string_view id);
		/// @brief Scene所有だけを解除。表示分類には依存しない
		void ClearScenePanels();
		EditorUiPanelDesc *Find(std::string_view id);
		const EditorUiPanelDesc *Find(std::string_view id) const;
		std::vector<EditorUiPanelDesc> &Panels() { return panels_; }
		const std::vector<EditorUiPanelDesc> &Panels() const { return panels_; }
		void Clear() { panels_.clear(); }
	private:
		std::vector<EditorUiPanelDesc> panels_;
	};
}

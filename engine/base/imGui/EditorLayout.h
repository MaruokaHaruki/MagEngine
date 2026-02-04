/*********************************************************************
 * \file   EditorLayout.h
 * \brief  エディターレイアウト管理
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   全パネルを統括し、DockSpace構成を管理
 *********************************************************************/
#pragma once
#include "EditorState.h"
#include <memory>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	// Forward declarations
	class BasePanel;
	class GameViewportPanel;
	class HierarchyPanel;
	class InspectorPanel;
	class ConsolePanel;
	class ToolsPanel;
	class DirectXCore;
	class PostEffectManager;

	///=============================================================================
	///						エディターレイアウト
	class EditorLayout {
	public:
		/// \brief コンストラクタ
		EditorLayout();

		/// \brief デストラクタ
		~EditorLayout();

		/// \brief 初期化
		void Initialize(DirectXCore *dxCore, PostEffectManager *postEffectManager);

		/// \brief 更新
		void Update();

		/// \brief 描画
		void Draw();

		/// \brief 終了処理
		void Finalize();

		//========================================
		// パネル管理
		void ShowAllPanels();
		void HideAllPanels();
		void ResetLayout();

		//========================================
		// エディター状態へのアクセス
		EditorState *GetEditorState() {
			return &editorState_;
		}
		const EditorState *GetEditorState() const {
			return &editorState_;
		}

		//========================================
		// Getter
		GameViewportPanel *GetViewportPanel() {
			return viewportPanel_.get();
		}
		HierarchyPanel *GetHierarchyPanel() {
			return hierarchyPanel_.get();
		}
		InspectorPanel *GetInspectorPanel() {
			return inspectorPanel_.get();
		}
		ConsolePanel *GetConsolePanel() {
			return consolePanel_.get();
		}
		ToolsPanel *GetToolsPanel() {
			return toolsPanel_.get();
		}

	private:
		//========================================
		// エディター状態
		EditorState editorState_;

		//========================================
		// パネルインスタンス
		std::unique_ptr<GameViewportPanel> viewportPanel_;
		std::unique_ptr<HierarchyPanel> hierarchyPanel_;
		std::unique_ptr<InspectorPanel> inspectorPanel_;
		std::unique_ptr<ConsolePanel> consolePanel_;
		std::unique_ptr<ToolsPanel> toolsPanel_;

		//========================================
		// ポインタ
		DirectXCore *dxCore_ = nullptr;
		PostEffectManager *postEffectManager_ = nullptr;

		//========================================
		// ヘルパー関数
		void SetupDockspace();
		void DrawMenuBar();
	};

}

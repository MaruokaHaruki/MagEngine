/*********************************************************************
 * \file   HierarchyPanel.h
 * \brief  シーンオブジェクト階層パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   シーン内のオブジェクトをツリービューで表示
 *********************************************************************/
#pragma once
#include "BasePanel.h"
#include <string>
#include <vector>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						シーンオブジェクト情報
	struct SceneObjectInfo {
		std::string name;
		void *objectPtr;
		bool isExpanded;
		int depth; // ネストレベル
	};

	///=============================================================================
	///						ヒエラルキーパネル
	class HierarchyPanel : public BasePanel {
	public:
		HierarchyPanel() : BasePanel("Hierarchy") {
		}

		void Initialize(EditorState *editorState, DirectXCore *dxCore) override;
		void Draw() override;
		void Update() override;

		//========================================
		// オブジェクト管理
		void AddObject(const std::string &name, void *objectPtr, int depth = 0);
		void ClearObjects();
		void RefreshHierarchy();

	private:
		std::vector<SceneObjectInfo> sceneObjects_;
		bool autoRefresh_ = true;
		bool showOnlyActive_ = false;

		// ヘルパー関数
		void DrawObjectNode(const SceneObjectInfo &obj);
	};

}

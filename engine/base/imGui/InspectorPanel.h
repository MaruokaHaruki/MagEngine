/*********************************************************************
 * \file   InspectorPanel.h
 * \brief  オブジェクトプロパティ検査パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   選択中のオブジェクトのプロパティを表示・編集
 *********************************************************************/
#pragma once
#include "BasePanel.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						インスペクターパネル
	class InspectorPanel : public BasePanel {
	public:
		InspectorPanel() : BasePanel("Inspector") {
		}

		void Initialize(EditorState *editorState, DirectXCore *dxCore) override;
		void Draw() override;
		void Update() override;

	private:
		bool showTransform_ = true;
		bool showMaterial_ = true;
		bool showComponent_ = true;

		// ヘルパー関数
		void DrawObjectInfo();
		void DrawTransformProperties();
		void DrawMaterialProperties();
		void DrawComponentProperties();
	};

}

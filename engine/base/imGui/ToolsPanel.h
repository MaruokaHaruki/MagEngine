/*********************************************************************
 * \file   ToolsPanel.h
 * \brief  ツール・パラメータ調整パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   ゲーム設定とエディター設定を調整
 *********************************************************************/
#pragma once
#include "BasePanel.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	// Forward declarations
	class PostEffectManager;

	///=============================================================================
	///						ツールパネル
	class ToolsPanel : public BasePanel {
	public:
		ToolsPanel() : BasePanel("Tools") {
		}

		void Initialize(EditorState *editorState, DirectXCore *dxCore) override;
		void Draw() override;
		void Update() override;

		void SetPostEffectManager(PostEffectManager *postEffectManager);

	private:
		PostEffectManager *postEffectManager_ = nullptr;
	};

}

/*********************************************************************
 * \file   ToolsPanel.h
 * \brief  ツール・パラメータ調整パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   ポストエフェクトやゲーム設定を調整
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

		void SetPostEffectManager(PostEffectManager *postEffectManager) {
			postEffectManager_ = postEffectManager;
		}

	private:
		PostEffectManager *postEffectManager_ = nullptr;
		bool showPostEffects_ = true;
		bool showGameSettings_ = true;
		bool showDebugSettings_ = true;

		// ヘルパー関数
		void DrawPostEffectControls();
		void DrawGameSettings();
		void DrawDebugSettings();
	};

}

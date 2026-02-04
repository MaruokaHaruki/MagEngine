/*********************************************************************
 * \file   GameViewportPanel.h
 * \brief  ゲーム画面表示パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   レンダーテクスチャを縮小して表示
 *********************************************************************/
#pragma once
#include "BasePanel.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						ゲームビューポートパネル
	class GameViewportPanel : public BasePanel {
	public:
		GameViewportPanel() : BasePanel("Game Viewport") {
		}

		void Initialize(EditorState *editorState, DirectXCore *dxCore) override;
		void Draw() override;
		void Update() override;

		//========================================
		// Getter/Setter
		ImVec2 GetViewportSize() const {
			return viewportSize_;
		}
		ImVec2 GetViewportPosition() const {
			return viewportPosition_;
		}
		bool IsViewportFocused() const {
			return isViewportFocused_;
		}
		bool IsViewportHovered() const {
			return isViewportHovered_;
		}

		void SetRenderTextureHandle(void *textureHandle) {
			textureHandle_ = textureHandle;
		}

	private:
		ImVec2 viewportSize_ = {640, 360}; // ビューポートサイズ
		ImVec2 viewportPosition_ = {0, 0}; // ビューポート位置
		bool isViewportFocused_ = false;   // フォーカス状態
		bool isViewportHovered_ = false;   // ホバー状態
		uint32_t textureIndex_ = 0;		   // テクスチャSRVインデックス
		void *textureHandle_ = nullptr;	   // レンダーテクスチャハンドル
	};

}

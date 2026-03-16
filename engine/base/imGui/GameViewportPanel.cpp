/*********************************************************************
 * \file   GameViewportPanel.cpp
 * \brief  ゲーム画面表示パネル実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "GameViewportPanel.h"
#include "DirectXCore.h"
#include "EditorState.h"
#include "SrvSetup.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	void GameViewportPanel::Initialize(EditorState *editorState, DirectXCore *dxCore) {
		BasePanel::Initialize(editorState, dxCore);
	}

	void GameViewportPanel::Draw() {
		if (!isVisible_)
			return;

		// 初回のみ左下に配置
		static bool first_time = true;
		if (first_time) {
			first_time = false;
			ImGuiViewport *main_viewport = ImGui::GetMainViewport();
			ImVec2 work_pos = main_viewport->WorkPos;
			ImVec2 work_size = main_viewport->WorkSize;

			// 左下配置：x=work_pos.x, y=work_pos.y+work_size.y*0.6
			ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + work_size.y * 0.6f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(work_size.x * 0.3f, work_size.y * 0.4f), ImGuiCond_FirstUseEver);
		}

		BeginPanel(320, 180);
		if (BeginPanelWindow(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

			// ビューポートサイズを取得
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
			if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
				viewportSize_ = viewportPanelSize;
				viewportPosition_ = ImGui::GetCursorScreenPos();

				// 背景を暗く描画
				ImDrawList *drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(
					{viewportPosition_.x, viewportPosition_.y},
					{viewportPosition_.x + viewportSize_.x, viewportPosition_.y + viewportSize_.y},
					ImGui::GetColorU32(ImVec4(0.05f, 0.05f, 0.1f, 1.0f)));

				// テクスチャを描画（有効なハンドルがある場合）
				if (textureHandle_ != nullptr) {
					ImGui::SetCursorScreenPos(viewportPosition_);
					ImGui::Image((ImTextureID)textureHandle_, viewportSize_, ImVec2(0, 0), ImVec2(1, 1));
				}

				// ホバーとフォーカス状態を更新
				isViewportHovered_ = ImGui::IsItemHovered();
				isViewportFocused_ = ImGui::IsItemActive();
			}
		}
		EndPanel();
	}

	void GameViewportPanel::Update() {
	}

}

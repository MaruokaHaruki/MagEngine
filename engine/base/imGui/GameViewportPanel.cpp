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
		// 必要に応じて初期化処理を行う
	}

	void GameViewportPanel::Draw() {
		if (!isVisible_)
			return;

		ImGui::SetNextWindowSizeConstraints(ImVec2(320, 180), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::Begin(panelName_.c_str(), &isVisible_,
						 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

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
				} else {
					// ハンドルがない場合はテキストを表示
					ImGui::SetCursorScreenPos({viewportPosition_.x + viewportSize_.x / 2 - 100,
											   viewportPosition_.y + viewportSize_.y / 2 - 30});
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "Render Texture Not Ready");
					ImGui::SetCursorScreenPos({viewportPosition_.x + viewportSize_.x / 2 - 120,
											   viewportPosition_.y + viewportSize_.y / 2 + 10});
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Handle: 0x%p", textureHandle_);
				}

				// ホバーとフォーカス状態を更新
				isViewportHovered_ = ImGui::IsItemHovered();
				isViewportFocused_ = ImGui::IsItemActive();
			}

			// グリッド表示オプション
			if (ImGui::CollapsingHeader("Viewport Options")) {
				ImGui::Checkbox("Show Grid", &editorState_->showGridInViewport);
				ImGui::Checkbox("Show Debug Info", &editorState_->showDebugInfoInViewport);
				ImGui::SliderFloat("Scale", &editorState_->viewportScale, 0.1f, 2.0f);
			}

			// デバッグ情報
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 1, 0.5f, 1), "Debug Info");
			ImGui::Text("Texture Handle: 0x%p", textureHandle_);
			ImGui::Text("Viewport Size: %.0f x %.0f", viewportSize_.x, viewportSize_.y);
		}
		ImGui::End();
	}

	void GameViewportPanel::Update() {
		// ビューポート関連の更新処理
	}

}

/*********************************************************************
 * \file EditorViewport.cpp
 * \brief ゲーム全体を切り抜かず表示し、クリックで入力対象を選ぶ
 *********************************************************************/
#include "EditorViewport.h"
#include "imgui.h"
namespace MagEngine {
	void EditorViewport::BeginFrame() { input_.allowInput = false; input_.imageRect = {}; }
	void EditorViewport::Draw(uint64_t texture, MagMath::Vector2 renderSize) {
		const auto position = ImGui::GetCursorScreenPos(), available = ImGui::GetContentRegionAvail();
		const auto rect = FitViewport({position.x, position.y, available.x, available.y}, renderSize.y > 0 ? renderSize.x / renderSize.y : 0);
		input_.imageRect = rect; input_.renderSize = renderSize;
		if (!texture || rect.width <= 0 || rect.height <= 0) { input_.allowInput = false; return; }
		ImGui::SetCursorScreenPos({rect.x, rect.y}); ImGui::Image(static_cast<ImTextureID>(texture), {rect.width, rect.height});
		const bool hovered = ImGui::IsItemHovered();
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { input_.focused = hovered; }
		const bool modal = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
		input_.allowInput = input_.focused && hovered && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput && !modal;
		const auto color = input_.allowInput ? IM_COL32(110, 168, 254, 255) : IM_COL32(65, 70, 80, 255);
		auto *draw = ImGui::GetWindowDrawList(); draw->AddRect({rect.x, rect.y}, {rect.x + rect.width, rect.y + rect.height}, color, 0, 0, 2);
		draw->PushClipRect({rect.x, rect.y}, {rect.x + rect.width, rect.y + rect.height}, true);
		if (rect.width > 240 * ImGui::GetFontSize() / 16.0f) draw->AddText({rect.x + 8, rect.y + 8}, IM_COL32(210, 215, 225, 255), input_.allowInput ? "ゲーム操作中" : "クリックしてゲームを操作");
		draw->PopClipRect();
		if (overlay_) { overlay_(rect); }
	}
}

/*********************************************************************
 * \file EditorTheme.cpp
 * \brief ダークな作業領域とライム・シアンの限定アクセント
 *********************************************************************/
#include "EditorTheme.h"
#include <cmath>
namespace MagEngine {
	///=============================================================================
	/// sRGBの描画先で設計色を表示するためのUI限定変換
	ImVec4 EditorDisplayColor(unsigned rgb) {
		auto linear = [](unsigned value) { const float c = value / 255.0f; return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); };
		return {linear((rgb >> 16) & 255), linear((rgb >> 8) & 255), linear(rgb & 255), 1};
	}
	ImVec4 EditorThemeCategoryColor(EditorUiCategory category) {
		// 既存の4分類の表示色を保持する。
		switch (category) {
		case EditorUiCategory::Authoring: return {110.0f / 255, 168.0f / 255, 254.0f / 255, 1};
		case EditorUiCategory::Engine: return {81.0f / 255, 199.0f / 255, 180.0f / 255, 1};
		case EditorUiCategory::GameDebug: return {229.0f / 255, 181.0f / 255, 103.0f / 255, 1};
		default: return {185.0f / 255, 154.0f / 255, 232.0f / 255, 1};
		}
	}
	int EditorToolbarRows(float availableWidth, float scale) { return availableWidth < 800.0f * scale ? 3 : 1; }
	///=============================================================================
	/// 倍率を累積させず、毎回基準値からテーマを構築する
	void ApplyEditorTheme(ImGuiStyle &style, float scale) {
		if (!std::isfinite(scale) || scale < 0.5f || scale > 4) return;
		style = ImGuiStyle{}; ImGui::StyleColorsDark(&style);
		style.WindowPadding = {10, 8}; style.FramePadding = {8, 5}; style.ItemSpacing = {8, 6}; style.ItemInnerSpacing = {6, 4}; style.CellPadding = {6, 4};
		style.WindowRounding = 0; style.ChildRounding = 0; style.FrameRounding = 2; style.PopupRounding = 2; style.TabRounding = 0;
		style.WindowBorderSize = 1; style.PopupBorderSize = 1; style.TabBorderSize = 0; style.ScrollbarSize = 14; style.GrabMinSize = 10;
		style.WindowMenuButtonPosition = ImGuiDir_Right;
		auto *c = style.Colors;
		c[ImGuiCol_Text] = EditorDisplayColor(0xDCE2E8); c[ImGuiCol_TextDisabled] = EditorDisplayColor(0x8B98A7);
		c[ImGuiCol_WindowBg] = EditorDisplayColor(0x16191F); c[ImGuiCol_ChildBg] = EditorDisplayColor(0x1D222A); c[ImGuiCol_PopupBg] = EditorDisplayColor(0x222832);
		c[ImGuiCol_Border] = c[ImGuiCol_Separator] = EditorDisplayColor(0x3C4653);
		c[ImGuiCol_TitleBg] = c[ImGuiCol_MenuBarBg] = EditorDisplayColor(0x1D222A); c[ImGuiCol_TitleBgActive] = EditorDisplayColor(0x303B48);
		c[ImGuiCol_FrameBg] = c[ImGuiCol_Button] = c[ImGuiCol_Header] = EditorDisplayColor(0x29323D);
		c[ImGuiCol_FrameBgHovered] = c[ImGuiCol_ButtonHovered] = c[ImGuiCol_HeaderHovered] = EditorDisplayColor(0x3A4B58);
		c[ImGuiCol_FrameBgActive] = c[ImGuiCol_ButtonActive] = c[ImGuiCol_HeaderActive] = EditorDisplayColor(0x405F6B);
		c[ImGuiCol_Tab] = EditorDisplayColor(0x222832); c[ImGuiCol_TabHovered] = EditorDisplayColor(0x405F6B); c[ImGuiCol_TabSelected] = EditorDisplayColor(0x364651);
		c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = c[ImGuiCol_TabSelectedOverline] = EditorDisplayColor(0xD7F542);
		c[ImGuiCol_NavHighlight] = c[ImGuiCol_SeparatorActive] = c[ImGuiCol_ResizeGripActive] = EditorDisplayColor(0x4ED9E8);
		c[ImGuiCol_ScrollbarBg] = EditorDisplayColor(0x16191F); c[ImGuiCol_ScrollbarGrab] = EditorDisplayColor(0x52606D);
		c[ImGuiCol_TextSelectedBg] = EditorDisplayColor(0x365967); c[ImGuiCol_DockingEmptyBg] = EditorDisplayColor(0x101318);
		style.ScaleAllSizes(scale);
	}
}

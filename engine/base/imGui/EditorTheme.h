/*********************************************************************
 * \file EditorTheme.h
 * \brief Vectorheartの共通色と100%基準の寸法
 *********************************************************************/
#pragma once
#include "imgui.h"
#include "EditorPanelRegistry.h"
///=============================================================================
/// 制作画面の共通テーマ
namespace MagEngine {
	/// @brief 100%基準から再設定する。有限の0.5〜4倍以外は変更しない
	void ApplyEditorTheme(ImGuiStyle &style, float scale);
	/// @brief 分類色。既存の分類と色の対応を維持する
	ImVec4 EditorThemeCategoryColor(EditorUiCategory category);
	/// @brief 幅は画面座標単位。800px@100%未満で操作群を縦に並べる
	int EditorToolbarRows(float availableWidth, float scale);
	/// @brief sRGB表示色をUI用の線形色へ変換する（ゲーム画像には使用しない）
	ImVec4 EditorDisplayColor(unsigned rgb);
}

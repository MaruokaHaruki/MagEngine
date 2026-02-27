/*********************************************************************
 * \file   EditorState.h
 * \brief  エディター状態管理
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   グローバルなエディター状態を一元管理
 *********************************************************************/
#pragma once
#include <cstdint>
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						パネル表示状態
	struct PanelVisibility {
		bool viewport = true; // Viewportパネル
		bool console = true;  // Consoleパネル
		bool tools = true;	  // Toolsパネル
	};

	///=============================================================================
	///						エディター状態構造体
	struct EditorState {
		//========================================
		// エディター制御
		bool isEditorMode = true; // エディターモード有効フラグ
		bool isPlayMode = false;  // 再生中フラグ

		//========================================
		// パネル表示状態
		PanelVisibility panelVisibility;

		//========================================
		// Viewport設定
		bool showGridInViewport = false;	  // Viewport内にグリッド表示
		bool showDebugInfoInViewport = false; // デバッグ情報表示

		//========================================
		// レイアウト設定
		bool dockspaceEnabled = true; // DockSpace有効フラグ
		bool menuBarVisible = true;	  // メニューバー表示
	};

}

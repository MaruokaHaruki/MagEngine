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
		bool hierarchy = true; // Hierarchyパネル
		bool inspector = true; // Inspectorパネル
		bool viewport = true;  // Viewportパネル
		bool console = true;   // Consoleパネル
		bool tools = true;	   // Toolsパネル
	};

	///=============================================================================
	///						エディター状態構造体
	struct EditorState {
		//========================================
		// エディターモード
		bool isEditorMode = true; // エディターモード有効フラグ
		bool isPlayMode = false;  // 再生中フラグ
		bool isPaused = false;	  // ポーズ中フラグ

		//========================================
		// 選択情報
		void *selectedObject = nullptr; // 選択中のオブジェクト
		std::string selectedObjectName; // 選択中のオブジェクト名

		//========================================
		// パネル表示状態
		PanelVisibility panelVisibility;

		//========================================
		// Viewport設定
		float viewportScale = 1.0f;			  // Viewport表示倍率（小さいほどズーム）
		bool showGridInViewport = true;		  // Viewport内にグリッド表示
		bool showDebugInfoInViewport = false; // デバッグ情報表示

		//========================================
		// レイアウト設定
		bool dockspaceEnabled = true; // DockSpace有効フラグ
		bool menuBarVisible = true;	  // メニューバー表示

		//========================================
		// ユーティリティ関数
		void ResetSelectedObject() {
			selectedObject = nullptr;
			selectedObjectName.clear();
		}

		void SetSelectedObject(void *obj, const std::string &name) {
			selectedObject = obj;
			selectedObjectName = name;
		}

		bool IsObjectSelected() const {
			return selectedObject != nullptr;
		}
	};

}

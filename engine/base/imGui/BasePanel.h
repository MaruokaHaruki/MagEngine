/*********************************************************************
 * \file   BasePanel.h
 * \brief  ImGuiパネル基底クラス
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   全てのパネルはこのクラスを継承
 *********************************************************************/
#pragma once
#include "imgui.h"
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	// Forward declarations
	struct EditorState;
	class DirectXCore;

	///=============================================================================
	///						パネル基底クラス
	class BasePanel {
	public:
		/// \brief 仮想デストラクタ
		virtual ~BasePanel() = default;

		/// \brief 初期化
		virtual void Initialize(EditorState *editorState, DirectXCore *dxCore) {
			editorState_ = editorState;
			dxCore_ = dxCore;
		}

		/// \brief パネル描画
		virtual void Draw() = 0;

		/// \brief パネル更新
		virtual void Update() {
		}

		/// \brief 終了処理
		virtual void Finalize() {
		}

		//========================================
		// Getter/Setter
		bool IsVisible() const {
			return isVisible_;
		}
		void SetVisible(bool visible) {
			isVisible_ = visible;
		}
		void ToggleVisibility() {
			isVisible_ = !isVisible_;
		}

		const std::string &GetPanelName() const {
			return panelName_;
		}

	protected:
		/// \brief 保護されたコンストラクタ
		BasePanel(const std::string &name) : panelName_(name) {
		}

		//========================================
		// メンバ変数
		std::string panelName_;				 // パネル名
		bool isVisible_ = true;				 // 表示フラグ
		EditorState *editorState_ = nullptr; // エディター状態へのポインタ
		DirectXCore *dxCore_ = nullptr;		 // DirectXCoreへのポインタ

		//========================================
		// ImGui ヘルパー関数
		void BeginPanel() {
			if (isVisible_) {
				ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
			}
		}

		void EndPanel() {
			ImGui::End();
		}
	};

}

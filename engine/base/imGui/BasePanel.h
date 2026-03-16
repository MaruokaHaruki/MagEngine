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
#include <functional>
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	// Forward declarations
	struct EditorState;
	class DirectXCore;

	// パネルスタイル設定
	struct PanelStyle {
		ImVec4 headerColor = ImVec4(0.2f, 0.6f, 0.9f, 1.0f);
		ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		float textScaling = 1.0f;
		ImVec2 minSize = ImVec2(200, 150);
	};

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

		void SetStyle(const PanelStyle &style) {
			style_ = style;
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
		PanelStyle style_;					 // スタイル設定

		//========================================
		// ImGui ヘルパー関数
		void BeginPanel(float minWidth = 200.0f, float minHeight = 150.0f) {
			if (isVisible_) {
				ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, minHeight), ImVec2(FLT_MAX, FLT_MAX));
			}
		}

		bool BeginPanelWindow(ImGuiWindowFlags flags = 0) {
			return ImGui::Begin(panelName_.c_str(), &isVisible_, flags);
		}

		void EndPanel() {
			ImGui::End();
		}

		// セクションヘッダー表示
		void DrawSectionHeader(const char *label) {
			ImGui::PushStyleColor(ImGuiCol_Text, style_.headerColor);
			ImGui::Separator();
			ImGui::Text(label);
			ImGui::PopStyleColor();
			ImGui::Separator();
		}

		// テキスト表示（自動色付け）
		void DrawText(const char *text, const ImVec4 &color = ImVec4(1, 1, 1, 1)) {
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("%s", text);
			ImGui::PopStyleColor();
		}

		// ハイライト設定
		void PushHeaderStyle() {
			ImGui::PushStyleColor(ImGuiCol_Text, style_.headerColor);
		}

		void PopHeaderStyle() {
			ImGui::PopStyleColor();
		}
	};

}

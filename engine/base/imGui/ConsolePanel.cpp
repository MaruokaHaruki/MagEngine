/*********************************************************************
 * \file   ConsolePanel.cpp
 * \brief  ログ・デバッグ出力パネル実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "ConsolePanel.h"
#include "EditorState.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	void ConsolePanel::Initialize(EditorState *editorState, DirectXCore *dxCore) {
		BasePanel::Initialize(editorState, dxCore);
		ClearLogs();
	}

	void ConsolePanel::Draw() {
		if (!isVisible_)
			return;

		BeginPanel(400, 200);
		if (BeginPanelWindow()) {

			// ツールバー
			if (ImGui::Button("Clear")) {
				ClearLogs();
			}
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &autoScroll_);
			ImGui::SameLine();
			ImGui::Checkbox("Info", &showInfo_);
			ImGui::SameLine();
			ImGui::Checkbox("Warning", &showWarning_);
			ImGui::SameLine();
			ImGui::Checkbox("Error", &showError_);

			ImGui::Separator();

			// ログ表示エリア
			if (ImGui::BeginChild("##console_logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
				for (const auto &log : logs_) {
					// フィルター適用
					if ((log.severity == 0 && !showInfo_) ||
						(log.severity == 1 && !showWarning_) ||
						(log.severity == 2 && !showError_)) {
						continue;
					}

					// 重要度に応じた色分け
					ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					switch (log.severity) {
					case 1: // Warning
						color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
						break;
					case 2: // Error
						color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
						break;
					}
					ImGui::TextColored(color, "%s", log.text.c_str());
				}

				// オートスクロール
				if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::EndChild();
		}
		EndPanel();
	}

	void ConsolePanel::Update() {
	}

	void ConsolePanel::AddLog(const std::string &message, int severity) {
		if (logs_.size() >= kMaxLogs) {
			logs_.erase(logs_.begin());
		}
		LogMessage log{};
		log.text = message;
		log.severity = severity;
		log.timestamp = 0.0f;
		logs_.push_back(log);
	}

	void ConsolePanel::ClearLogs() {
		logs_.clear();
	}

}

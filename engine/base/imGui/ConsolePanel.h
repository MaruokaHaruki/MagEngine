/*********************************************************************
 * \file   ConsolePanel.h
 * \brief  ログ・デバッグ出力パネル
 *
 * \author Harukichimaru
 * \date   February 2025
 * \note   ゲーム実行中のログ出力を表示
 *********************************************************************/
#pragma once
#include "BasePanel.h"
#include <string>
#include <vector>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						ログメッセージ
	struct LogMessage {
		std::string text;
		int severity; // 0=Info, 1=Warning, 2=Error
		float timestamp;
	};

	///=============================================================================
	///						コンソールパネル
	class ConsolePanel : public BasePanel {
	public:
		ConsolePanel() : BasePanel("Console") {
		}

		void Initialize(EditorState *editorState, DirectXCore *dxCore) override;
		void Draw() override;
		void Update() override;

		//========================================
		// ログ管理
		void AddLog(const std::string &message, int severity = 0);
		void ClearLogs();

	private:
		std::vector<LogMessage> logs_;
		bool showInfo_ = true;
		bool showWarning_ = true;
		bool showError_ = true;
		bool autoScroll_ = true;
		static const int kMaxLogs = 1000;
	};

}

/*********************************************************************
 * \file EditorLaunchOptions.h
 * \brief 通常起動と編集用起動の引数解析
 *********************************************************************/
#pragma once
#include <filesystem>
#include <span>
#include <string>
namespace MagEngine {
	struct EditorLaunchOptions {
		bool editor = false;
		std::filesystem::path projectRoot;
		std::filesystem::path scene;
	};
	/// @brief 不正な引数や制作ルート外のシーンを拒否し、出力を保持する
	bool ParseEditorLaunchOptions(std::span<const std::wstring> args, EditorLaunchOptions &options, std::string &error);
	/// @brief 解析後の既定値を解決。runtimeRootは絶対資産ルート、editorAvailableはDebugでtrue
	/// @note 明示シーンは実行モードを維持する。失敗時はoptionsを変更しない
	bool ResolveEngineLaunchOptions(EditorLaunchOptions &options, const std::filesystem::path &runtimeRoot, bool editorAvailable, std::string &error);
}

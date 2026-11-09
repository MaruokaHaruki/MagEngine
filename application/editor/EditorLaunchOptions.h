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
}

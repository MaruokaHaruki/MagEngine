/*********************************************************************
 * \file   main.cpp
 * \brief  メインファイル
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "application/EngineApp.h"
#include "MagFramework.h"
#include <memory>
#include <filesystem>
#include <stdexcept>
#include "Logger.h"
#include <shellapi.h>
#include <vector>
#pragma comment(lib, "shell32.lib")

///=============================================================================
///						Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { // 各引数の中身があるか確認する
	if (hInstance == nullptr || lpCmdLine == nullptr || nCmdShow < 0 || hPrevInstance != nullptr) {
		return -1; // 引数が不正な場合はエラー終了
	}
	try {
		// Unicode起動引数は作業フォルダを変える前に解析する。
		int argumentCount = 0;
		auto rawArguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
		if (!rawArguments) { throw std::runtime_error("起動引数を解析できません"); }
		std::vector<std::wstring> arguments;
		for (int i = 1; i < argumentCount; ++i) { arguments.emplace_back(rawArguments[i]); }
		LocalFree(rawArguments);
		MagEngine::EditorLaunchOptions launch;
		std::string launchError;
		if (!MagEngine::ParseEditorLaunchOptions(arguments, launch, launchError)) { throw std::runtime_error(launchError); }
#ifndef _DEBUG
		if (launch.editor) { throw std::runtime_error("編集画面はDebug構成で起動してください"); }
#endif
		// NOTE: 配布先の資産を参照し、起動時の作業フォルダに依存させない。
		wchar_t executablePath[MAX_PATH]{};
		const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		if (pathLength == 0 || pathLength == MAX_PATH) {
			throw std::runtime_error("Cannot resolve executable path");
		}
		std::filesystem::current_path(std::filesystem::path(executablePath).parent_path());
		// 既存のShader／モデル読込はresources相対。編集時は明示した制作ルートを使う。
		if (!launch.projectRoot.empty()) { std::filesystem::current_path(launch.projectRoot); }
		//========================================
		// フレームワークのインスタンスを生成
		std::unique_ptr<MagEngine::MagFramework> framework = std::make_unique<MagEngine::EngineApp>(launch);
		//========================================
		// ゲームの実行
		framework->Run();
	} catch (const std::exception &error) {
		Logger::Log(error.what(), Logger::LogLevel::Error);
		return 1;
	}
	//========================================
	return 0;
}

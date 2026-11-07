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

///=============================================================================
///						Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) { // 各引数の中身があるか確認する
	if (hInstance == nullptr || lpCmdLine == nullptr || nCmdShow < 0 || hPrevInstance != nullptr) {
		return -1; // 引数が不正な場合はエラー終了
	}
	try {
		// NOTE: 配布先の資産を参照し、起動時の作業フォルダに依存させない。
		wchar_t executablePath[MAX_PATH]{};
		const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
		if (pathLength == 0 || pathLength == MAX_PATH) {
			throw std::runtime_error("Cannot resolve executable path");
		}
		std::filesystem::current_path(std::filesystem::path(executablePath).parent_path());
		//========================================
		// フレームワークのインスタンスを生成
		std::unique_ptr<MagEngine::MagFramework> framework = std::make_unique<MagEngine::EngineApp>();
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
/*********************************************************************
* \file   main.cpp
* \brief  メインファイル
*
* \author Harukichimaru
* \date   November 2024
* \note
*********************************************************************/
#include "EngineApp.h"
#include "MagFramework.h"
///=============================================================================
///						Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {\
    // 各引数の中身があるか確認する
	if(hInstance == nullptr || lpCmdLine == nullptr || nCmdShow < 0 || hPrevInstance != nullptr) {
		return -1; // 引数が不正な場合はエラー終了
	}
    //========================================
    // フレームワークのインスタンスを生成
    MagFramework *framework = new EngineApp();
    //========================================
    // ゲームの実行
    framework->Run();
    //========================================
    // 後処理
    delete framework;
    //========================================
    return 0;
}

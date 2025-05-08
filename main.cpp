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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

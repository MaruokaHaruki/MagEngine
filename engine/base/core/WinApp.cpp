/*********************************************************************
 * \file   WinApp.cpp
 * \brief  ウィンドウズアプリケーション管理クラスの実装
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   ウィンドウ作成・破棄・イベント処理を実装
 *********************************************************************/
#include "WinApp.h"
#include "Input.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///                        ウィンドウプロシージャ
	LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		if (msg == WM_NCCREATE) {
			const auto *create = reinterpret_cast<CREATESTRUCTW *>(lparam);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
		}
		auto *window = reinterpret_cast<WinApp *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		// サイズ通知ではGPUを操作せず、次の安全なフレーム境界へ要求だけ渡す。
		if (msg == WM_SIZE && window) {
			window->surfaceState_.Request({LOWORD(lparam), HIWORD(lparam)});
		}
		if (msg == WM_DPICHANGED && window) {
			window->dpiScale_ = HIWORD(wparam) / 96.0f;
			const auto *rect = reinterpret_cast<const RECT *>(lparam);
			SetWindowPos(hwnd, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOACTIVATE | SWP_NOZORDER);
			RECT client{}; if (GetClientRect(hwnd, &client)) window->surfaceState_.Request({static_cast<uint32_t>(client.right), static_cast<uint32_t>(client.bottom)});
			return 0;
		}
		if (msg == WM_CLOSE) {
			auto *app = reinterpret_cast<WinApp *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
			if (app && app->closeRequestHandler_ && !app->closeRequestHandler_()) { return 0; }
		}
		//========================================
		// ImGuiに伝える
		if(ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
			return true;
		}
		//========================================
		// メッセージに応じてゲーム固有の処理を行う
		switch(msg) {
			// ウィンドウが破棄された
		case WM_DESTROY:
			// OSに対して、アプリの終了を伝える
			PostQuitMessage(0);
			return 0;
		case WM_MOUSEWHEEL:
			short delta = GET_WHEEL_DELTA_WPARAM(wparam);
			Input::GetInstance()->OnMouseWheel(delta);
			return 0;
		}
		//========================================
		// 標準のメッセージ処理を行う
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	///=============================================================================
	///                        ウィンドウの生成
	void WinApp::CreateGameWindow(const wchar_t *title, int32_t clientWidth, int32_t clientHeight) {
		// manifest指定はないため、最初のHWND生成前に一度だけ設定する。
		// ホスト側が指定済みの場合のACCESS_DENIEDは既存の設定を維持する。
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		surfaceState_.Request({static_cast<uint32_t>(clientWidth), static_cast<uint32_t>(clientHeight)});
		//========================================
		// COM 初期化
		CoInitializeEx(0, COINIT_MULTITHREADED);
		//========================================
		// ウィンドウクラス生成
		// ウィンドウプロシーシャ
		wc_.lpfnWndProc = WindowProc;
		// ウィンドウクラス
		wc_.lpszClassName = L"CG2WindowClass";
		// インスタンスハンドル
		wc_.hInstance = GetModuleHandle(nullptr);
		// カーソル
		wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
		// ウィンドウクラスの登録
		RegisterClass(&wc_);
		//========================================
		// ウィンドウ生成
		// ウィンドウサイズを表す構造体にクライアント領域を入れる
		wrc_ = { 0, 0, clientWidth, clientHeight };
		// クライアント領域を元に実際のサイズにwrcを変更してもらう
		AdjustWindowRect(&wrc_, WS_OVERLAPPEDWINDOW, false);
		// ウィンドウ作成
		hwnd_ = CreateWindow(wc_.lpszClassName,
			title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			wrc_.right - wrc_.left,
			wrc_.bottom - wrc_.top,
			nullptr,
			nullptr,
			wc_.hInstance,
			this);
		//========================================
		// ウィンドウ表示
		ShowWindow(hwnd_, SW_SHOW);
		dpiScale_ = GetDpiForWindow(hwnd_) / 96.0f;
	}

	///=============================================================================
	///                        ウィンドウの終了
	void WinApp::CloseWindow() {
		//========================================
		// COM 終了
		CoUninitialize();
	}
}

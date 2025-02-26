/*********************************************************************
 * \file   WinApp.h
 * \brief　ウィンドウズアプリケーションクラス
 * 
 * \author Harukichimaru
 * \date   December 2024
 * \note   
 *********************************************************************/
//NOTE:ウィンドウズAPIを管理する

#include <cstdint>
#include <string>
#include <windows.h>

#pragma once
class WinApp {
public:
	/// ===クライアントの領域サイズ=== ///
	static constexpr  int32_t kWindowWidth_ = 1280;//横幅
	static constexpr  int32_t kWindowHeight_ = 720;//縦幅

public:

	/**----------------------------------------------------------------------------
	 * \brief  WindowProc	ウィンドウプロシージャ
	 * \param  hwnd			ウィンドウハンドル
	 * \param  msg			メッセージ
	 * \param  wparam		メッセージ固有の追加情報
	 * \param  lparam		メッセージ固有の追加情報
	 * \return 
	 */
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/**----------------------------------------------------------------------------
	 * \brief  CreateGameWindow ゲームウィンドウの生成
	 * \param  title			ウィンドウタイトル
	 * \param  clientWidth		クライアントの横幅
	 * \param  clientHeight		クライアントの縦幅
	 */
	void CreateGameWindow(const wchar_t* title = L"DirectXGame", int32_t clientWidth = kWindowWidth_, int32_t clientHeight = kWindowHeight_);

	/// <summary>
	/// ウィンドウの終了
	/// </summary>
	void CloseWindow();

	// ゲッター
	static constexpr int32_t GetWindowWidth() { return kWindowWidth_; }
	static constexpr int32_t GetWindowHeight() { return kWindowHeight_; }
	WNDCLASS GetWindowClass() const { return wc_; }
	RECT GetWindowRect() const { return wrc_; }
	HWND GetWindowHandle() const { return hwnd_; }

	// セッター
	void SetWindowClass(const WNDCLASS& wc) { wc_ = wc; }
	void SetWindowRect(const RECT& wrc) { wrc_ = wrc; }
	void SetWindowHandle(HWND hwnd) { hwnd_ = hwnd; }

private:
	/// ===ウィンドウクラス=== ///
	WNDCLASS wc_{};

	/// ===WRC=== ///
	RECT wrc_{};

	/// ===ウィンドウハンドル=== ///
	HWND hwnd_{};

};


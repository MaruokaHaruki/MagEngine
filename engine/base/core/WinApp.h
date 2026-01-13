/*********************************************************************
 * \file   WinApp.h
 * \brief　ウィンドウズアプリケーションクラス
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   ウィンドウズAPIを管理する
 *********************************************************************/
#pragma once
#include <cstdint>
#include <string>
#include <windows.h>

 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///                        ウィンドウクラス
	class WinApp {
		///--------------------------------------------------------------
		///                        クライアント領域サイズ
	public:
		static constexpr int32_t kWindowWidth_ = 1280; // 横幅
		static constexpr int32_t kWindowHeight_ = 720; // 縦幅

		///--------------------------------------------------------------
		///                        メンバ関数
	public:
		/// @brief WindowProc 	ウィンドウプロシージャ
		/// @param hwnd 		ウィンドウハンドル
		/// @param msg 			メッセージ
		/// @param wparam 		メッセージ固有の追加情報
		/// @param lparam 		メッセージ固有の追加情報
		/// @return
		static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// @brief CreateGameWindow ゲームウィンドウの生成
		/// @param title 			ウィンドウタイトル
		/// @param clientWidth 		クライアントの横幅
		/// @param clientHeight		クライアントの縦幅
		void CreateGameWindow(const wchar_t *title = L"DirectXGame", int32_t clientWidth = kWindowWidth_, int32_t clientHeight = kWindowHeight_);

		/// @brief CloseWindow ウィンドウの破棄
		void CloseWindow();

		///--------------------------------------------------------------
		///                        Getter

		/// @brief GetWindowWidth 	ウィンドウ横幅取得
		/// @return 				ウィンドウ横幅
		static constexpr int32_t GetWindowWidth() {
			return kWindowWidth_;
		}

		/// @brief GetWindowHeight 	ウィンドウ縦幅取得
		/// @return 				ウィンドウ縦幅
		static constexpr int32_t GetWindowHeight() {
			return kWindowHeight_;
		}

		/// @brief GetWindowClass 	ウィンドウクラス取得
		/// @return 				ウィンドウクラス
		WNDCLASS GetWindowClass() const {
			return wc_;
		}

		/// @brief GetWindowRect 	ウィンドウ領域取得
		/// @return 				ウィンドウ領域
		RECT GetWindowRect() const {
			return wrc_;
		}

		/// @brief GetWindowHandle 	ウィンドウハンドル取得
		/// @return 				ウィンドウハンドル
		HWND GetWindowHandle() const {
			return hwnd_;
		}

		///--------------------------------------------------------------
		///                        Setter

		/// @brief SetWindowClass 	ウィンドウクラス設定
		/// @param wc 				ウィンドウクラス
		void SetWindowClass(const WNDCLASS &wc) {
			wc_ = wc;
		}

		/// @brief SetWindowRect 	ウィンドウ領域設定
		/// @param wrc 				ウィンドウ領域
		void SetWindowRect(const RECT &wrc) {
			wrc_ = wrc;
		}

		/// @brief SetWindowHandle 	ウィンドウハンドル設定
		/// @param hwnd 			ウィンドウハンドル
		void SetWindowHandle(HWND hwnd) {
			hwnd_ = hwnd;
		}

		///--------------------------------------------------------------
		///                        メンバ変数
	private:
		//========================================
		// ウィンドウクラス
		WNDCLASS wc_{};
		//========================================
		// ウィンドウ領域
		RECT wrc_{};
		//========================================
		// ウィンドウハンドル
		HWND hwnd_{};
	};
}
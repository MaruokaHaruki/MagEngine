///=====================================================///
///
/// 文字列変換ユーティリティ
///
///=====================================================///

#pragma once
#include <Windows.h>
#include <string>

namespace WstringUtility {
	/**----------------------------------------------------------------------------
	 * @brief std::string を std::wstring に変換
	 * @param str 変換元の文字列
	 * @return std::wstring 変換後のワイド文字列
	 */
	std::wstring ConvertString(const std::string &str);

	/**----------------------------------------------------------------------------
	 * @brief std::wstring を std::string に変換
	 * @param str 変換元のワイド文字列
	 * @return std::string 変換後の文字列
	 */
	std::string ConvertString(const std::wstring &str);
}
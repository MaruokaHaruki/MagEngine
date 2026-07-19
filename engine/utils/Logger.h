///=============================================================================
/// ログ出力
///=============================================================================
#pragma once
#include <string>
#include <Windows.h>

namespace Logger {
	/// @brief 出力先で扱うログの重要度
	/// @note 表示色などの表現はログ出力側で決定し、呼び出し側は重要度のみを指定する。
	enum class LogLevel {
		Info,
		Success,
		Warning,
		Error
	};

	/// @brief デバッグ出力へメッセージを記録
	/// @note Windowsのデバッグ出力に依存するため、Release環境の永続ログ用途には使用しない。
	void Log(const std::string &message, LogLevel level = LogLevel::Info);
}

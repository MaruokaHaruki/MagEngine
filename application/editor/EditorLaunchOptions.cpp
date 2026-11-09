/*********************************************************************
 * \file EditorLaunchOptions.cpp
 * \brief Unicode起動引数と制作ルートの入力境界
 *********************************************************************/
#include "EditorLaunchOptions.h"
namespace MagEngine {
	bool ParseEditorLaunchOptions(std::span<const std::wstring> args, EditorLaunchOptions &options, std::string &error) {
		EditorLaunchOptions candidate;
		for (size_t i = 0; i < args.size(); ++i) {
			if (args[i] == L"--editor") { candidate.editor = true; }
			else if ((args[i] == L"--project" || args[i] == L"--scene") && i + 1 < args.size()) {
				const bool root = args[i] == L"--project"; ++i;
				if (root) { candidate.projectRoot = args[i]; } else { candidate.scene = args[i]; }
			} else { error = "起動引数は--editor --project <絶対パス> --scene <相対パス>を指定してください"; return false; }
		}
		if ((candidate.editor || !candidate.scene.empty()) && !candidate.projectRoot.is_absolute()) { error = "制作ルートの絶対パスが必要です"; return false; }
		if (!candidate.projectRoot.empty() && !candidate.projectRoot.is_absolute()) { error = "制作ルートは絶対パスで指定してください"; return false; }
		if (!candidate.scene.empty()) {
			const auto normalized = candidate.scene.lexically_normal();
			if (normalized.is_absolute() || normalized.has_root_name() || normalized.empty() || *normalized.begin() == "..") { error = "シーンは制作ルート内の相対パスで指定してください"; return false; }
			candidate.scene = normalized;
		}
		options = std::move(candidate); error.clear(); return true;
	}
}

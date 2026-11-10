/*********************************************************************
 * \file EditorLayoutStore.h
 * \brief シーンデータと分離したユーザー別レイアウト保存
 *********************************************************************/
#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace MagEngine {
	struct EditorLayoutSnapshot {
		int version = 1;
		std::string name;
		std::string imguiVersion = "1.91.9b";
		std::string ini;
		std::map<std::string, bool> panelOpen;
		std::string selectedLayout = "標準";
	};
	/// @brief 保存文書と同梱版のINIを検証する。ImGuiを初期化せず利用可能
	bool ValidateLayout(const EditorLayoutSnapshot &snapshot, std::string &error);
	std::string EditorLayoutKey(std::string_view utf8);
	class EditorLayoutStore {
	public:
		/// @brief rootはUI状態専用の保存先。テストでは生成領域を注入する
		explicit EditorLayoutStore(std::filesystem::path root) : root_(std::move(root)) {}
		bool SaveNamed(const EditorLayoutSnapshot &, bool allowOverwrite, std::string &error);
		bool LoadNamed(std::string_view name, EditorLayoutSnapshot &, std::string &error);
		std::vector<std::string> ListNames(std::string &error) const;
		bool SaveSession(std::string_view mode, const EditorLayoutSnapshot &, std::string &error);
		bool LoadSession(std::string_view mode, EditorLayoutSnapshot &, std::string &error);
	private:
		bool Save(const std::filesystem::path &, const EditorLayoutSnapshot &, bool overwrite, std::string &error);
		bool Load(const std::filesystem::path &, EditorLayoutSnapshot &, std::string &error) const;
		std::filesystem::path root_;
	};
}

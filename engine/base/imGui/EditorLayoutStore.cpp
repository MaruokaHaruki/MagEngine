/*********************************************************************
 * \file EditorLayoutStore.cpp
 * \brief 不正な配置を拒否し、置換失敗時は既存の保存を維持する
 *********************************************************************/
#include "EditorLayoutStore.h"
#include "externals/json.hpp"
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <set>
#include <stdexcept>
namespace MagEngine {
	namespace {
		bool ValidName(std::string_view name) {
			if (name.empty() || name == "." || name == ".." || name.front() == ' ' || name.back() == ' ') { return false; }
			size_t count = 0;
			for (unsigned char c : name) { if (c < 32 || c == '/' || c == '\\' || c == 127) { return false; } if ((c & 0xc0) != 0x80) { ++count; } }
			return count <= 64;
		}
		std::string Read(const std::filesystem::path &path) {
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file || file.tellg() < 0 || file.tellg() > 2 * 1024 * 1024) { throw std::runtime_error("配置ファイルが読めないか、大きすぎます"); }
			file.seekg(0); return std::string((std::istreambuf_iterator<char>(file)), {});
		}
		using Json = nlohmann::json;
	}
	std::string EditorLayoutKey(std::string_view value) {
		uint64_t hash = 14695981039346656037ull; for (unsigned char c : value) { hash ^= c; hash *= 1099511628211ull; }
		std::ostringstream text; text << std::hex << std::setw(16) << std::setfill('0') << hash; return text.str();
	}
	///=============================================================================
	/// 同梱ImGuiの配置だけを受け入れる
	bool ValidateLayout(const EditorLayoutSnapshot &snapshot, std::string &error) {
		try {
			if (snapshot.version != 1 || snapshot.imguiVersion != "1.91.9b" || !ValidName(snapshot.name) || snapshot.ini.size() > 1024 * 1024 || snapshot.panelOpen.size() > 128) { throw std::runtime_error("配置名・形式・バージョンまたは容量が不正です"); }
			std::istringstream stream(snapshot.ini); std::string line; size_t windows = 0; bool section = false;
			std::map<unsigned long, unsigned long> parents; std::set<unsigned long> windowDocks;
			const std::regex idPattern("ID=0x([0-9A-Fa-f]+)"), parentPattern("Parent=0x([0-9A-Fa-f]+)"), dockPattern("DockId=0x([0-9A-Fa-f]+)");
			const std::regex pairPattern("(Pos|Size|SizeRef)=([^ ,]+),([^ ,]+)");
			while (std::getline(stream, line)) {
				if (!line.empty() && line.back() == '\r') { line.pop_back(); }
				if (line.empty()) { continue; }
				if (line.find('\0') != std::string::npos || line.size() > 4096) { throw std::runtime_error("配置INIに不正な行があります"); }
				if (line.front() == '[') {
					section = line.starts_with("[Window][") || line == "[Docking][Data]";
					if (!section || line.back() != ']') { throw std::runtime_error("配置INIのセクションが未対応です"); }
					if (line.starts_with("[Window][") && ++windows > 128) { throw std::runtime_error("ウィンドウ数が上限を超えています"); }
					continue;
				}
				if (!section) { throw std::runtime_error("配置INIの見出しがありません"); }
				for (auto it = std::sregex_iterator(line.begin(), line.end(), pairPattern); it != std::sregex_iterator(); ++it) {
					const auto kind = (*it)[1].str(); const float x = std::stof((*it)[2].str()), y = std::stof((*it)[3].str());
					if (!std::isfinite(x) || !std::isfinite(y) || std::abs(x) > 100000 || std::abs(y) > 100000 || (kind != "Pos" && (x <= 0 || y <= 0))) { throw std::runtime_error("配置INIの座標や大きさが不正です"); }
				}
				std::smatch match;
				if (line.find("DockNode") != std::string::npos || line.find("DockSpace") != std::string::npos) {
					if (!std::regex_search(line, match, idPattern)) { throw std::runtime_error("ドックIDがありません"); }
					const auto id = std::stoul(match[1].str(), nullptr, 16); unsigned long parent = 0;
					if (std::regex_search(line, match, parentPattern)) { parent = std::stoul(match[1].str(), nullptr, 16); }
					if (!id || parents.size() >= 256 || !parents.emplace(id, parent).second) { throw std::runtime_error("ドックIDが重複するか容量を超えています"); }
				}
				if (std::regex_search(line, match, dockPattern)) { windowDocks.insert(std::stoul(match[1].str(), nullptr, 16)); }
			}
			for (const auto &[id, parent] : parents) {
				(void)parent; std::set<unsigned long> visited; auto current = id;
				while (current) { if (!visited.insert(current).second || !parents.contains(current)) { throw std::runtime_error("ドックの親参照が不正か循環しています"); } current = parents.at(current); }
			}
			for (auto id : windowDocks) { if (id && !parents.contains(id)) { throw std::runtime_error("ウィンドウのドック先が存在しません"); } }
			error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
	///=============================================================================
	/// 文書の読書き
	bool EditorLayoutStore::Save(const std::filesystem::path &path, const EditorLayoutSnapshot &value, bool overwrite, std::string &error) {
		std::filesystem::path temporary;
		try {
			if (!ValidateLayout(value, error)) { return false; }
			if (std::filesystem::exists(path) && !overwrite) { throw std::runtime_error("同じ名前の配置があります。上書きを確認してください"); }
			std::filesystem::create_directories(root_);
			const auto bytes = Json{{"version", value.version}, {"name", value.name}, {"imguiVersion", value.imguiVersion}, {"ini", value.ini}, {"panelOpen", value.panelOpen}, {"selectedLayout", value.selectedLayout}}.dump(2);
			temporary = path; temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
			HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE) { temporary.clear(); throw std::runtime_error("配置保存の一時ファイルを作れません"); }
			DWORD written = 0; const bool success = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size() && FlushFileBuffers(file); const bool closed = CloseHandle(file) != 0;
			if (!success || !closed) { throw std::runtime_error("配置の書込に失敗しました"); }
			const bool exists = std::filesystem::exists(path);
			const bool replaced = exists && overwrite ? ReplaceFileW(path.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr) != 0 : MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
			if (!replaced) { throw std::runtime_error("配置ファイルを置換できません"); }
			error.clear(); return true;
		} catch (const std::exception &e) { if (!temporary.empty()) { std::error_code ignored; std::filesystem::remove(temporary, ignored); } error = e.what(); return false; }
	}
	bool EditorLayoutStore::Load(const std::filesystem::path &path, EditorLayoutSnapshot &value, std::string &error) const {
		try {
			const auto json = Json::parse(Read(path)); EditorLayoutSnapshot candidate;
			candidate.version = json.at("version").get<int>(); candidate.name = json.at("name").get<std::string>(); candidate.imguiVersion = json.at("imguiVersion").get<std::string>(); candidate.ini = json.at("ini").get<std::string>(); candidate.panelOpen = json.at("panelOpen").get<std::map<std::string, bool>>(); candidate.selectedLayout = json.at("selectedLayout").get<std::string>();
			if (!ValidateLayout(candidate, error)) { return false; } value = std::move(candidate); error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
	bool EditorLayoutStore::SaveNamed(const EditorLayoutSnapshot &value, bool overwrite, std::string &error) {
		const auto path = root_ / (EditorLayoutKey(value.name) + ".json");
		if (std::filesystem::exists(path)) { EditorLayoutSnapshot existing; if (!Load(path, existing, error) || existing.name != value.name) { error = "保存名の衝突か既存配置の破損を検出しました"; return false; } }
		return Save(path, value, overwrite, error);
	}
	bool EditorLayoutStore::LoadNamed(std::string_view name, EditorLayoutSnapshot &value, std::string &error) {
		if (!ValidName(name)) { error = "配置名が不正です"; return false; }
		EditorLayoutSnapshot candidate; if (!Load(root_ / (EditorLayoutKey(name) + ".json"), candidate, error)) { return false; }
		if (candidate.name != name) { error = "保存名が一致しません"; return false; } value = std::move(candidate); return true;
	}
	std::vector<std::string> EditorLayoutStore::ListNames(std::string &error) const {
		std::vector<std::string> result; std::error_code ec;
		if (!std::filesystem::exists(root_, ec)) { return result; }
		for (std::filesystem::directory_iterator it(root_, ec), end; !ec && it != end; it.increment(ec)) {
			if (it->path().extension() != ".json" || it->path().filename().string().starts_with("session-")) { continue; }
			EditorLayoutSnapshot value; std::string localError; if (Load(it->path(), value, localError)) { result.push_back(value.name); }
		}
		if (ec) { error = "配置一覧を読めません"; } std::sort(result.begin(), result.end()); return result;
	}
	bool EditorLayoutStore::SaveSession(std::string_view mode, const EditorLayoutSnapshot &value, std::string &error) { if (mode != "edit" && mode != "game") { error = "起動モードが不正です"; return false; } return Save(root_ / ("session-" + std::string(mode) + ".json"), value, true, error); }
	bool EditorLayoutStore::LoadSession(std::string_view mode, EditorLayoutSnapshot &value, std::string &error) { if (mode != "edit" && mode != "game") { error = "起動モードが不正です"; return false; } return Load(root_ / ("session-" + std::string(mode) + ".json"), value, error); }
}

/*********************************************************************
 * \file EditorDocumentStore.cpp
 * \brief 検証後の読込と元ファイルを維持する置換保存
 *********************************************************************/
#include "EditorDocumentStore.h"
#include "EditorTypeRegistry.h"
#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <numbers>
#include <stdexcept>

namespace MagEngine {
	namespace {
		using Json = nlohmann::json;
		Json Vector(const MagMath::Vector3 &v) { return Json::array({v.x, v.y, v.z}); }
		MagMath::Vector3 Vector(const Json &v) {
			if (!v.is_array() || v.size() != 3) { throw std::runtime_error("Vector3は3要素が必要です"); }
			return {v.at(0).get<float>(), v.at(1).get<float>(), v.at(2).get<float>()};
		}
		Json Transform(const MagMath::Transform &v) { return {{"position", Vector(v.translate)}, {"rotation", Vector(v.rotate)}, {"scale", Vector(v.scale)}}; }
		MagMath::Transform Transform(const Json &v) { return {Vector(v.at("scale")), Vector(v.at("rotation")), Vector(v.at("position"))}; }
		std::string Read(const std::filesystem::path &path) {
			std::ifstream stream(path, std::ios::binary);
			if (!stream) {
				const auto utf8Path = path.u8string();
				throw std::runtime_error("ファイルを開けません: " + std::string(utf8Path.begin(), utf8Path.end()));
			}
			std::string bytes((std::istreambuf_iterator<char>(stream)), {});
			if (stream.bad()) { throw std::runtime_error("ファイル読込失敗"); }
			return bytes;
		}
		bool Within(const std::filesystem::path &root, const std::filesystem::path &path) {
			auto rel = std::filesystem::weakly_canonical(path).lexically_relative(std::filesystem::weakly_canonical(root));
			return !rel.empty() && !rel.is_absolute() && *rel.begin() != "..";
		}
	}
	///=============================================================================
	/// 保存境界
	EditorDocumentStore::EditorDocumentStore(std::filesystem::path root, const EditorTypeRegistry *registry) : root_(std::move(root)), registry_(registry) {
		if (!root_.is_absolute()) { throw std::invalid_argument("制作ルートは絶対パスで指定してください"); }
	}
	bool EditorDocumentStore::ValidateAssets(const EditorDocument &document, std::string &error) const {
		if (!ValidateDocument(document, error)) { return false; }
		for (const auto &object : document.objects) {
			const auto *type = registry_ ? registry_->Find(object.typeId) : nullptr;
			if ((registry_ && !type) || (!registry_ && object.typeId != "object3d")) { error = object.name + ": 未対応の型です"; return false; }
			if (type && type->validate && !type->validate(object, error)) { return false; }
			if (object.modelPath.empty()) { continue; } // EMPTYはモデルを持たない
			const auto relative = std::filesystem::path(std::u8string(object.modelPath.begin(), object.modelPath.end()));
			const auto asset = root_ / "resources" / relative;
			if (relative.is_absolute() || !Within(root_ / "resources", asset) || !std::filesystem::is_regular_file(asset)) { error = object.name + ": モデルが存在しないか制作ルート外です: " + object.modelPath; return false; }
		}
		return true;
	}
	bool EditorDocumentStore::Load(const std::filesystem::path &path, EditorDocument &document, std::string &error) {
		try {
			if (!Within(root_, path)) { throw std::runtime_error("読込先は制作ルート内に指定してください"); }
			const auto bytes = Read(path); const auto json = Json::parse(bytes);
			if (json.at("version").get<int>() != 1) { throw std::runtime_error("未対応の文書バージョンです"); }
			EditorDocument candidate; candidate.nextId = json.at("nextId").get<EditorObjectId>(); candidate.camera = Transform(json.at("camera"));
			for (const auto &item : json.at("objects")) {
				EditorObjectRecord object;
				object.id = item.at("id").get<EditorObjectId>(); object.name = item.at("name").get<std::string>(); object.typeId = item.at("type").get<std::string>();
				object.modelPath = item.at("model").get<std::string>(); object.visible = item.at("visible").get<bool>(); object.transform = Transform(item.at("transform")); object.settings = item.at("settings");
				candidate.objects.push_back(std::move(object));
			}
			if (!ValidateAssets(candidate, error)) { return false; }
			knownFiles_[std::filesystem::weakly_canonical(path)] = bytes;
			document = std::move(candidate); error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
	bool EditorDocumentStore::Save(const std::filesystem::path &path, const EditorDocument &document, std::string &error) {
		std::filesystem::path temporary;
		try {
			if (!Within(root_ / "resources/config/scenes", path)) { throw std::runtime_error("保存先は制作ルートのresources/config/scenes内に指定してください"); }
			if (!ValidateAssets(document, error)) { return false; }
			const auto key = std::filesystem::weakly_canonical(path);
			const bool exists = std::filesystem::exists(path);
			const auto known = knownFiles_.find(key);
			if ((exists && (known == knownFiles_.end() || Read(path) != known->second)) || (!exists && known != knownFiles_.end())) { throw std::runtime_error("保存先が外部で変更されています。再読込または別名保存してください"); }
			Json json{{"version", 1}, {"nextId", document.nextId}, {"camera", Transform(document.camera)}, {"objects", Json::array()}};
			for (const auto &o : document.objects) { json["objects"].push_back({{"id", o.id}, {"name", o.name}, {"type", o.typeId}, {"model", o.modelPath}, {"visible", o.visible}, {"transform", Transform(o.transform)}, {"settings", o.settings}}); }
			const auto bytes = json.dump(2);
			std::filesystem::create_directories(path.parent_path());
			// 同名の既存一時ファイルを上書きしない。CREATE_NEWで排他的に作る。
			temporary = path; temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
			HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE) { temporary.clear(); throw std::runtime_error("保存用一時ファイルを作成できません"); }
			DWORD written = 0;
			const bool complete = bytes.size() <= MAXDWORD && WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size() && FlushFileBuffers(file);
			const bool closed = CloseHandle(file) != 0;
			if (!complete || !closed) { throw std::runtime_error("文書の書込に失敗しました"); }
			// 置換前にも外部更新を確認する。排他編集を保証するファイルロックではない。
			if (exists && Read(path) != known->second) { throw std::runtime_error("保存中に外部更新されました"); }
			const bool replaced = exists ? ReplaceFileW(path.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr) != 0 : MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
			if (!replaced) { throw std::runtime_error("保存先の置換に失敗しました: Windows error " + std::to_string(GetLastError())); }
			knownFiles_[key] = bytes; error.clear(); return true;
		} catch (const std::exception &e) {
			if (!temporary.empty()) { std::error_code ignored; std::filesystem::remove(temporary, ignored); }
			error = e.what(); return false;
		}
	}
	bool EditorDocumentStore::ImportBlender(const std::filesystem::path &path, EditorDocument &document, std::string &error) {
		try {
			EditorDocument candidate;
			const auto json = Json::parse(Read(path));
			for (const auto &item : json.at("objects")) {
				if ((item.contains("children") && !item.at("children").empty()) || (item.at("type") != "MESH" && item.at("type") != "EMPTY") || item.contains("collider")) { throw std::runtime_error("初期版は階層・ColliderのないMESH／EMPTYに対応します"); }
				EditorObjectRecord object; object.name = item.value("name", "Object"); object.modelPath = item.value("file_name", "");
				if (!object.modelPath.empty()) { object.modelPath = "model/" + object.modelPath; }
				if (item.contains("transform")) {
					const auto &t = item.at("transform"); object.transform.translate = Vector(t.at("translation")); object.transform.translate.z *= -1;
					object.transform.scale = Vector(t.at("scale")); object.transform.rotate = Vector(t.at("rotation"));
					// 入力契約は度。新文書はMakeAffineMatrixが使うラジアンで保持。
					const float radians = std::numbers::pi_v<float> / 180.0f;
					object.transform.rotate.x *= radians; object.transform.rotate.y *= -radians; object.transform.rotate.z *= -radians;
				}
				if (!AddObject(candidate, std::move(object))) { throw std::runtime_error("ID上限です"); }
			}
			if (!ValidateAssets(candidate, error)) { return false; }
			document = std::move(candidate); error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
}

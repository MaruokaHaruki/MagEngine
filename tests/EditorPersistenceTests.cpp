/*********************************************************************
 * \file EditorPersistenceTests.cpp
 * \brief 保存往復・破損・外部更新・失敗時の既存文書保護
 *********************************************************************/
#include "engine/editor/EditorDocumentStore.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace { void Check(bool v, const char *m) { if (!v) { throw std::runtime_error(m); } } }
int main(int argc, char **argv) {
	try {
		using namespace MagEngine;
		if (argc != 2) { throw std::runtime_error("fixture root required"); }
		const auto root = std::filesystem::path(argv[1]) / std::filesystem::path(L"編集 保存テスト");
		std::filesystem::create_directories(root / "resources/config/scenes");
		std::ofstream(root / "resources/test.obj") << "fixture";
		const auto file = root / "resources/config/scenes/test.json";
		std::error_code ignored; std::filesystem::remove(file, ignored);
		EditorDocument document; EditorObjectRecord record; record.modelPath = "test.obj";
		AddObject(document, record);
		EditorDocumentStore store(root); std::string error;
		if (!store.Save(file, document, error)) { throw std::runtime_error("initial save: " + error); }
		EditorDocument loaded;
		Check(store.Load(file, loaded, error) && EqualDocument(document, loaded), "RoundTripPreservesIdsAndValues");
		std::ofstream(file, std::ios::app) << " ";
		Check(!store.Save(file, document, error), "ExternalEditIsConflict");
		std::ofstream(file) << R"({"version":999})";
		Check(!store.Load(file, loaded, error) && EqualDocument(document, loaded), "LoadFailureLeavesDocumentUntouched");
		Check(!store.Save(file, document, error), "SaveFailurePreservesOriginal");
		std::ifstream original(file); std::string bytes((std::istreambuf_iterator<char>(original)), {});
		Check(bytes == R"({"version":999})", "original preserved");
		record.modelPath = "../outside.obj"; EditorDocument invalid; AddObject(invalid, record);
		Check(!store.Save(root / "resources/config/scenes/invalid.json", invalid, error), "RejectRootEscape");
		record.modelPath = "missing.obj"; invalid = {}; AddObject(invalid, record);
		Check(!store.Save(root / "resources/config/scenes/missing.json", invalid, error), "RejectMissingAsset");
		std::ofstream(root / "hierarchy.json") << R"({"objects":[{"type":"EMPTY","children":[{}]}]})";
		Check(!store.ImportBlender(root / "hierarchy.json", loaded, error) && EqualDocument(document, loaded), "RejectHierarchyImport");
		std::filesystem::create_directories(root / "resources/model"); std::ofstream(root / "resources/model/test.obj") << "fixture";
		std::ofstream(root / "flat.json") << R"({"objects":[{"name":"mesh","type":"MESH","file_name":"test.obj","transform":{"translation":[1,2,3],"rotation":[90,0,0],"scale":[1,1,1]}}]})";
		Check(store.ImportBlender(root / "flat.json", loaded, error) && loaded.objects[0].modelPath == "model/test.obj", "ImportUsesModelDirectory");
		std::cout << "EditorPersistence PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

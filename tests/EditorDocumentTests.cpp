/*********************************************************************
 * \file EditorDocumentTests.cpp
 * \brief 編集文書のIDと入力境界を直接検証する
 *********************************************************************/
#include "engine/editor/EditorDocument.h"
#include "engine/editor/EditorTypeRegistry.h"
#include "application/editor/EditorLaunchOptions.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
	void Check(bool value, const char *message) {
		if (!value) { throw std::runtime_error(message); }
	}
}

///=============================================================================
/// 同名対象の区別、削除後のID、外部入力の拒否
int main() {
	try {
		using namespace MagEngine;
		EditorTypeRegistry registry;
		EditorTypeDescriptor descriptor; descriptor.typeId = "object3d";
		Check(registry.Register(descriptor) && !registry.Register(descriptor) && !registry.Find("unknown"), "RegistryRejectsDuplicateAndUnknownType");
		EditorLaunchOptions launch;
		std::string launchError;
		std::vector<std::wstring> arguments{L"--editor", L"--project", L"C:\\制作 フォルダ", L"--scene", L"resources/config/scenes/test.json"};
		Check(ParseEditorLaunchOptions(arguments, launch, launchError) && launch.editor && launch.projectRoot.is_absolute(), "UnicodeProjectRoot");
		arguments.back() = L"../outside.json";
		Check(!ParseEditorLaunchOptions(arguments, launch, launchError), "RejectSceneEscape");
		arguments = {L"--project", L"relative"};
		Check(!ParseEditorLaunchOptions(arguments, launch, launchError), "RejectRelativeProjectRoot");
		EditorDocument document;
		EditorObjectRecord record;
		record.name = "同じ名前";
		const auto first = AddObject(document, record);
		const auto second = AddObject(document, record);
		Check(first != 0 && second != first, "DuplicateNamesDoNotAlias");
		document.objects.erase(document.objects.begin());
		const auto third = AddObject(document, record);
		Check(third > second && FindObject(document, first) == nullptr, "StableIdsSurviveRemoval");
		std::string error;
		Check(ValidateDocument(document, error), "normal document");
		document.objects[1].id = document.objects[0].id;
		Check(!ValidateDocument(document, error) && !error.empty(), "RejectDuplicateIds");
		document.objects[1].id = third;
		document.objects[0].transform.translate.x = std::numeric_limits<float>::infinity();
		Check(!ValidateDocument(document, error), "RejectNonFiniteTransform");
		document.objects[0].transform.translate.x = 0.0f;
		document.nextId = std::numeric_limits<EditorObjectId>::max();
		Check(AddObject(document, record) == 0, "RejectIdExhaustion");
		std::cout << "EditorDocument PASS\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}

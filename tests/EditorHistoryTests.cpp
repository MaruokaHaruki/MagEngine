/*********************************************************************
 * \file EditorHistoryTests.cpp
 * \brief 削除復元と履歴分岐によるIDの取り違えを検証
 *********************************************************************/
#include "engine/editor/EditorHistory.h"
#include <iostream>
#include <stdexcept>
namespace { void Check(bool value, const char *message) { if (!value) { throw std::runtime_error(message); } } }
int main() {
	try {
		using namespace MagEngine;
		EditorDocument document;
		const auto id = AddObject(document, {});
		EditorHistory history;
		auto changed = document; changed.objects.clear();
		Check(history.Execute(document, changed), "delete");
		Check(history.Undo(document) && FindObject(document, id), "UndoDeleteRestoresSameId");
		changed = document; const auto duplicateId = AddObject(changed, changed.objects[0]);
		Check(history.Execute(document, changed) && duplicateId != id, "DuplicateUsesFreshId");
		Check(history.Undo(document), "undo duplicate");
		changed = document; const auto fresh = AddObject(changed, {});
		Check(fresh > duplicateId, "NeverReuseDiscardedRedoId");
		Check(history.Execute(document, changed) && !history.Redo(document), "NewEditClearsRedo");
		Check(!history.Execute(document, document), "NoOpDoesNotAddHistory");
		Check(history.Undo(document) && document.objects.size() == 1, "single undo");
		std::cout << "EditorHistory PASS\n";
		return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

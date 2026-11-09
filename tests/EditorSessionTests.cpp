/*********************************************************************
 * \file EditorSessionTests.cpp
 * \brief 実行値と明示編集値の分離を検証
 *********************************************************************/
#include "engine/editor/EditorPlaySession.h"
#include <iostream>
#include <stdexcept>
namespace { void Check(bool v, const char *m) { if (!v) { throw std::runtime_error(m); } } }
int main() {
	try {
		using namespace MagEngine;
		EditorDocument document; auto id = AddObject(document, {}); const auto original = document;
		EditorPlaySession session; std::string error;
		Check(session.Start(document, error), "start");
		FindObject(session.Runtime(), id)->transform.translate.x = 99;
		Check(session.Pending().empty(), "SimulationChangesNeverBecomeCandidates");
		Check(session.RecordUserEdit(id, "position", {1, 2, 3}, error), "edit");
		Check(session.UndoUserEdit(error) && session.Pending().empty(), "RuntimeUndoKeepsSeparateHistory");
		Check(session.RedoUserEdit(error) && session.Pending().size() == 1, "RuntimeRedoRestoresExplicitCandidate");
		FindObject(session.Runtime(), id)->transform.translate.x = 300;
		session.Stop();
		Check(EqualDocument(document, original), "StopKeepsEditDocumentUntouched");
		Check(!session.Start(document, error), "PendingChangesBlockNextAction");
		EditorHistory history; std::vector<EditorPropertyKey> keys{{id, "position"}};
		Check(session.ApplySelected(document, keys, history, error), "apply");
		Check(FindObject(document, id)->transform.translate.x == 1, "ExplicitValueSurvivesLaterSimulation");
		Check(history.Undo(document) && EqualDocument(document, original) && !history.Undo(document), "ApplySelectedIsOneUndo");
		session.DiscardPending();
		Check(session.Start(document, error), "start drag simulation");
		auto *runtimeObject = FindObject(session.Runtime(), id);
		runtimeObject->transform.scale = {4, 4, 4};
		Check(CommitEditorProperty(session.Runtime(), history, &session, {id, "position"}, {0, 0, 0}, {7, 8, 9}, false, error), "commit explicit drag");
		Check(session.Pending().size() == 1 && runtimeObject->transform.scale.x == 4, "DragDoesNotCaptureSimulationOtherProperties");
		Check(CommitEditorProperty(session.Runtime(), history, &session, {id, "position"}, {7, 8, 9}, {100, 100, 100}, true, error), "cancel drag");
		Check(runtimeObject->transform.translate.x == 7 && session.Pending().size() == 1, "CancelRestoresOnlyDraggedProperty");
		session.Stop(); session.DiscardPending();
		auto invalid = document; invalid.objects[0].transform.scale.x = 0;
		Check(!session.Start(invalid, error) && EqualDocument(document, original), "FailedStartKeepsEditing");
		std::cout << "EditorSession PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

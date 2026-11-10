/*********************************************************************
 * \file EditorWorkspaceTests.cpp
 * \brief 表示上の分類とコールバックの寿命を直接検証する
 *********************************************************************/
#include "engine/base/imGui/EditorPanelRegistry.h"
#include "engine/base/imGui/EditorLayoutStore.h"
#include "engine/base/imGui/EditorWorkspaceLayout.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/base/imGui/EditorUiSystem.h"
#include "engine/base/imGui/EditorTheme.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace { void Check(bool value, const char *message) { if (!value) { throw std::runtime_error(message); } } }
int main(int argc, char **argv) {
	try {
		using namespace MagEngine;
		ImGuiStyle theme;
		ApplyEditorTheme(theme, 1); const float padding = theme.WindowPadding.x;
		ApplyEditorTheme(theme, 1.5f);
		Check(std::abs(theme.WindowPadding.x - padding * 1.5f) < 0.001f, "DpiUsesBaseline");
		ApplyEditorTheme(theme, 2); ApplyEditorTheme(theme, 1);
		Check(theme.WindowPadding.x == padding, "DpiScalingDoesNotAccumulate");
		ApplyEditorTheme(theme, 0);
		Check(theme.WindowPadding.x == padding, "InvalidDpiPreservesStyle");
		Check(EditorToolbarRows(600, 1) > EditorToolbarRows(1200, 1) && EditorToolbarRows(1200, 2) > 1, "NarrowToolbarKeepsPrimaryActions");
		EditorPanelRegistry registry;
		Check(registry.Register({"game.switcher", "切替", EditorUiCategory::GameDebug, EditorPanelScope::Engine, EditorDockRegion::Right, false, false, [] {}}), "register permanent");
		Check(registry.Register({"game.scene", "同じ表示名", EditorUiCategory::GameDebug, EditorPanelScope::Scene, EditorDockRegion::Right, false, false, [] {}}), "register scene");
		Check(registry.Register({"authoring.details", "同じ表示名", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Right, true, true, [] {}}), "DuplicateLabelsKeepDistinctIds");
		Check(!registry.Register({"game.scene", "別の名前", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, false, [] {}}), "DuplicateIdsAreRejected");
		registry.Find("authoring.details")->name = "変更後";
		Check(registry.Find("authoring.details") != nullptr, "StableIdSurvivesRenamedLabel");
		registry.ClearScenePanels();
		Check(registry.Find("game.switcher") && registry.Find("authoring.details") && !registry.Find("game.scene"), "ClassificationDoesNotChangeLifetime");
		Check(argc == 2, "fixture root");
		const auto root = std::filesystem::path(argv[1]) / "workspace";
		EditorLayoutStore store(root); std::string error;
		EditorLayoutSnapshot layout; layout.name = "制作配置"; layout.ini = "[Window][設定###authoring.details]\nPos=10,20\nSize=300,400\nCollapsed=0\n\n"; layout.panelOpen["authoring.details"] = true;
		Check(store.SaveNamed(layout, true, error), "save named");
		EditorLayoutSnapshot loaded;
		Check(store.LoadNamed(layout.name, loaded, error) && loaded.ini == layout.ini && loaded.panelOpen == layout.panelOpen, "NamedLayoutRoundTrip");
		Check(!store.SaveNamed(layout, false, error), "ConfirmBeforeOverwrite");
		layout.ini += "\n";
		Check(store.SaveSession("edit", layout, error), "save session");
		Check(store.LoadNamed(layout.name, loaded, error) && loaded.ini != layout.ini, "SessionDoesNotOverwriteNamedLayout");
		auto invalid = layout; invalid.name = "../bad";
		Check(!store.SaveNamed(invalid, true, error), "RejectUnsafeName");
		invalid = layout; invalid.version = 999;
		Check(!ValidateLayout(invalid, error), "RejectFutureVersion");
		invalid = layout; invalid.ini = "[Docking][Data]\nDockNode ID=0x1 Parent=0x2 SizeRef=100,100\nDockNode ID=0x2 Parent=0x1 SizeRef=100,100\n";
		Check(!ValidateLayout(invalid, error), "RejectCyclicIni");
		invalid.ini.assign(1024 * 1024 + 1, 'x');
		Check(!ValidateLayout(invalid, error), "RejectOversizedIni");
		const auto before = loaded.ini;
		Check(!store.LoadNamed("missing", loaded, error) && loaded.ini == before, "FailedLoadPreservesSnapshot");
		EditorLayoutStore isolated(root / "other-project");
		Check(!isolated.LoadNamed("制作配置", loaded, error), "ProjectIsolation");
		ImGui::CreateContext(); auto &io = ImGui::GetIO(); io.IniFilename = nullptr; io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; io.DisplaySize = {1280, 720}; io.DeltaTime = 1.0f / 60;
		io.Fonts->AddFontDefault(); io.Fonts->Build();
		EditorPanelRegistry docked;
		docked.Register({"workspace.viewport", "シーン", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Center, true, true, [] {}});
		docked.Register({"authoring.details", "設定", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Right, true, true, [] {}});
		docked.Register({"game.check", "デバッグ", EditorUiCategory::GameDebug, EditorPanelScope::Scene, EditorDockRegion::Right, false, false, [] {}});
		ImGui::NewFrame(); const ImGuiID dockspace = ImGui::GetID("test.workspace");
		EditorWorkspaceLayout layoutBuilder; layoutBuilder.Build(EditorLayoutPreset::Standard, docked, dockspace, {1280, 680});
		ImGui::SetNextWindowPos({0, 0}); ImGui::SetNextWindowSize({1280, 680}); ImGui::Begin("Host"); ImGui::DockSpace(dockspace);
		ImGui::End(); ImGui::Begin("設定###authoring.details"); Check(ImGui::GetWindowDockID() != 0, "PresetCreatesDockedWindows"); ImGui::End();
		Check(!docked.Find("game.check")->isOpen, "DefaultPanelsExcludeDebugging");
		ImGui::EndFrame();
		EditorUiSystem ui; ui.Initialize(); ui.SetSceneTexture([] { return uint64_t{1}; });
		ui.RegisterPanel({"authoring.objects", "一覧", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Left, true, true, [] {}});
		ui.RegisterPanel({"engine.closed", "閉じた機能", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, false, [] {}});
		ui.ConfigureWorkspace(root, EditorWorkspaceMode::Edit, root / "ui-state");
		ui.QueuePreset(EditorLayoutPreset::Standard); // 前回のセッション記録に依存しない検証の出発点
		ImGui::NewFrame(); ui.Draw(); ImGui::Render();
		auto captured = ui.CaptureLayout("標準保存");
		Check(ValidateLayout(captured, error), error.c_str());
		Check(ui.SaveLayoutAs("標準保存", true, error), "CaptureAndSaveRealDockIni");
		ui.QueuePreset(EditorLayoutPreset::SceneFocus);
		ImGui::NewFrame(); ui.Draw(); ImGui::Render();
		Check(!ui.IsPanelOpen("authoring.objects"), "SceneFocusHidesAuxiliaryPanels");
		Check(ui.SaveWorkspaceSession(error), "SaveRealSession");
		ui.QueueNamedLayout("標準保存"); ImGui::NewFrame(); ui.Draw(); ImGui::Render();
		Check(ui.IsPanelOpen("authoring.objects"), "NamedLayoutRestoresPanelOpenState");
		ui.RegisterPanel({"engine.late", "追加機能", EditorUiCategory::Engine, EditorPanelScope::Scene, EditorDockRegion::Right, true, true, [] {}});
		for (int frame = 0; frame < 2; ++frame) { ImGui::NewFrame(); ui.Draw(); ImGui::Render(); }
		const auto *rightSettings = ImGui::FindWindowSettingsByID(ImHashStr("閉じた機能###engine.closed"));
		const auto *lateWindow = ImGui::FindWindowByName("追加機能###engine.late");
		Check(rightSettings && lateWindow && rightSettings->DockId == lateWindow->DockId, "LatePanelUsesSavedOrDefaultRegion");
		ui.Finalize();
		const auto corruptRoot = root / "corrupt-state";
		std::filesystem::create_directories(corruptRoot);
		std::ofstream(corruptRoot / "session-edit.json") << "preserve-corrupt-session";
		EditorUiSystem recovery; recovery.Initialize(); recovery.ConfigureWorkspace(root, EditorWorkspaceMode::Edit, corruptRoot);
		Check(!recovery.SaveWorkspaceSession(error), "CorruptSessionNeverOverwrittenOnExit");
		std::ifstream preserved(corruptRoot / "session-edit.json"); std::string preservedBytes((std::istreambuf_iterator<char>(preserved)), {});
		Check(preservedBytes == "preserve-corrupt-session", "OriginalCorruptSessionPreserved");
		recovery.Finalize(); ImGui::DestroyContext();
		std::cout << "EditorWorkspace PASS\n"; return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

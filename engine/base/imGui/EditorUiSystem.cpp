/*********************************************************************
 * \file EditorUiSystem.cpp
 * \brief ゲーム画像を中心にした共通ドックとレイアウトの操作導線
 *********************************************************************/
#include "EditorUiSystem.h"
#include "EditorTheme.h"
#include "EngineContext.h"
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "Input.h"
#include "LightManager.h"
#include "LineManager.h"
#include "TrailEffectManager.h"
#include "Logger.h"
#include <shlobj.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
namespace MagEngine {
    ///=============================================================================
    /// 所有スコープと表示分類を明示して登録する
    void EditorUiSystem::Initialize() { ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; registry_.Clear(); }
    void EditorUiSystem::Finalize() {
        registry_.Clear(); viewport_.SetOverlay({}); toolbar_ = {}; beforeLayoutChange_ = {}; additionalMessages_ = {}; fileMenu_ = {}; sceneTexture_ = {}; store_.reset(); configured_ = false;
    }
    void EditorUiSystem::RegisterEnginePanels(const EngineContext &c, std::function<void()> rendering, std::function<void()> performance) {
        c.Validate(); const auto *context = &c;
        RegisterPanel("engine.input", "入力", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->input->ImGuiDraw(); });
        RegisterPanel("engine.camera", "カメラ", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->cameraManager->DrawImGui(); });
        RegisterPanel("engine.light", "ライト", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->lightManager->DrawImGui(); });
        RegisterPanel("engine.line", "線描画", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->lineManager->DrawImGui(); });
        RegisterPanel("engine.trail", "トレイル", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->trailEffectManager->DrawImGui(); });
        RegisterPanel("engine.text", "テキスト", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, [context] { context->debugTextManager->DrawImGui(); });
        RegisterPanel("engine.posteffect", "ポストエフェクト", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Right, false, std::move(rendering));
        RegisterPanel("engine.performance", "性能", EditorUiCategory::Engine, EditorPanelScope::Engine, EditorDockRegion::Bottom, false, std::move(performance));
    }
    bool EditorUiSystem::RegisterPanel(EditorUiPanelDesc panel) {
        if (const auto saved = savedPanelOpen_.find(panel.id); saved != savedPanelOpen_.end()) { panel.defaultOpen = saved->second; }
        return registry_.Register(std::move(panel));
    }
    bool EditorUiSystem::RegisterPanel(const std::string &id, const std::string &name, EditorUiCategory category, EditorPanelScope scope, EditorDockRegion region, bool open, std::function<void()> draw) {
        return RegisterPanel({id, name, category, scope, region, open, open, std::move(draw)});
    }
    void EditorUiSystem::UnregisterPanel(const std::string &id) { registry_.Unregister(id); dockedIds_.erase(id); }
    void EditorUiSystem::ClearScenePanels() {
        for (const auto &panel : registry_.Panels()) { if (panel.scope == EditorPanelScope::Scene) { savedPanelOpen_[panel.id] = panel.isOpen; dockedIds_.erase(panel.id); } }
        registry_.ClearScenePanels();
    }
    void EditorUiSystem::SetPanelOpen(const std::string &id, bool open) { if (auto *panel = registry_.Find(id)) { panel->isOpen = open; } }
    bool EditorUiSystem::IsPanelOpen(const std::string &id) const { const auto *panel = registry_.Find(id); return panel && panel->isOpen; }
    ///=============================================================================
    /// 配置の保存先はゲーム資産や作業ディレクトリと分離する
    void EditorUiSystem::ConfigureWorkspace(std::filesystem::path projectRoot, EditorWorkspaceMode mode, std::filesystem::path testStore) {
        mode_ = mode; configured_ = true; ImGui::GetIO().IniFilename = nullptr;
        if (!registry_.Find("workspace.viewport")) {
            RegisterPanel("workspace.viewport", mode == EditorWorkspaceMode::Edit ? "シーン" : "ゲーム", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Center, true, [this] { viewport_.Draw(sceneTexture_ ? sceneTexture_() : 0, {static_cast<float>(renderSize_.width), static_cast<float>(renderSize_.height)}); });
        }
        RegisterPanel("workspace.messages", "メッセージ", EditorUiCategory::Tools, EditorPanelScope::Engine, EditorDockRegion::Bottom, false, [this] { if (message_.empty()) { ImGui::TextUnformatted("メッセージはありません"); } else { ImGui::TextWrapped("%s", message_.c_str()); } if (additionalMessages_) { additionalMessages_(); } });
        pendingPreset_ = EditorLayoutPreset::Standard;
        try {
            auto identity = std::filesystem::weakly_canonical(projectRoot).wstring();
            CharLowerBuffW(identity.data(), static_cast<DWORD>(identity.size()));
            const auto utf8 = std::filesystem::path(identity).generic_u8string(); const std::string key(utf8.begin(), utf8.end());
            if (testStore.empty()) {
                PWSTR folder = nullptr; const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder);
                if (FAILED(result)) { throw std::runtime_error("ユーザー設定フォルダを取得できません"); }
                testStore = std::filesystem::path(folder) / "MagEngine/Editor" / EditorLayoutKey(key); CoTaskMemFree(folder);
            }
            std::filesystem::create_directories(testStore);
            const auto identityPath = testStore / "project.txt";
            if (std::filesystem::exists(identityPath)) {
                std::ifstream file(identityPath, std::ios::binary); const std::string existing((std::istreambuf_iterator<char>(file)), {});
                if (existing != key) { throw std::runtime_error("配置保存先のプロジェクト識別情報が一致しません"); }
            } else { std::ofstream file(identityPath, std::ios::binary); file << key; file.close(); if (!file) { throw std::runtime_error("プロジェクト識別情報を保存できません"); } }
            store_ = std::make_unique<EditorLayoutStore>(testStore);
            EditorLayoutSnapshot snapshot; std::string error;
            if (store_->LoadSession(mode == EditorWorkspaceMode::Edit ? "edit" : "game", snapshot, error)) { QueueRestore(std::move(snapshot)); }
            else if (std::filesystem::exists(testStore / (mode == EditorWorkspaceMode::Edit ? "session-edit.json" : "session-game.json"))) { sessionWritable_ = false; message_ = "保存配置を復元できないため標準へ戻します。元ファイルの自動上書きは停止します: " + error; }
        } catch (const std::exception &e) { message_ = e.what(); store_.reset(); }
    }
    EditorLayoutSnapshot EditorUiSystem::CaptureLayout(std::string name) {
        EditorLayoutSnapshot result; result.name = std::move(name); result.selectedLayout = selectedLayout_; result.panelOpen = savedPanelOpen_;
        for (const auto &panel : registry_.Panels()) { result.panelOpen[panel.id] = panel.isOpen; }
        const char *ini = ImGui::SaveIniSettingsToMemory(); std::istringstream stream(ini ? ini : ""); std::string line; bool include = false;
        while (std::getline(stream, line)) { if (!line.empty() && line.front() == '[') { include = line.starts_with("[Window][") || line == "[Docking][Data]"; } if (include && line != "Size=0,0") { result.ini += line + "\n"; } }
        return result;
    }
    bool EditorUiSystem::SaveLayoutAs(std::string_view name, bool overwrite, std::string &error) {
        if (!store_) { error = "配置の保存先が使用できません"; return false; }
        auto value = CaptureLayout(std::string(name)); value.selectedLayout = name;
        if (!store_->SaveNamed(value, overwrite, error)) { return false; } selectedLayout_ = name; return true;
    }
    bool EditorUiSystem::SaveWorkspaceSession(std::string &error) {
        if (!configured_ || !store_) { return true; }
        if (!sessionWritable_) { error = "復元できなかった配置ファイルを保全するため、自動保存を停止しています"; return false; }
        return store_->SaveSession(mode_ == EditorWorkspaceMode::Edit ? "edit" : "game", CaptureLayout("最後の配置"), error);
    }
    void EditorUiSystem::QueueNamedLayout(std::string name) {
        EditorLayoutSnapshot snapshot;
        if (!store_ || !store_->LoadNamed(name, snapshot, message_)) { return; }
        QueueRestore(std::move(snapshot));
    }
    ///=============================================================================
    /// 適用はパネル描画より先。毎フレーム配置を強制しない
    void EditorUiSystem::ApplyPending(ImVec2 size) {
        if (pendingRestore_ || pendingPreset_) {
            if (beforeLayoutChange_) { beforeLayoutChange_(); }
            dockedIds_.clear();
            if (pendingRestore_) {
                auto value = std::move(*pendingRestore_); pendingRestore_.reset();
                if (ValidateLayout(value, message_)) {
                    ImGui::LoadIniSettingsFromMemory(value.ini.data(), value.ini.size()); savedPanelOpen_ = value.panelOpen; selectedLayout_ = value.selectedLayout;
                    for (auto &panel : registry_.Panels()) { const auto found = savedPanelOpen_.find(panel.id); panel.isOpen = found != savedPanelOpen_.end() && found->second; }
                } else { pendingPreset_ = EditorLayoutPreset::Standard; }
            }
            if (pendingPreset_) {
                const auto preset = *pendingPreset_; pendingPreset_.reset(); selectedLayout_ = EditorPresetName(preset); savedPanelOpen_.clear();
                layout_.Build(preset, registry_, dockspace_, size);
                for (auto &panel : registry_.Panels()) { panel.isOpen = EditorPresetOpens(preset, panel); }
            }
        }
        for (auto &panel : registry_.Panels()) { if (dockedIds_.insert(panel.id).second) { layout_.DockNewPanel(panel, dockspace_, &registry_); } }
    }
    void EditorUiSystem::DrawMenus() {
        if (!ImGui::BeginMainMenuBar()) { return; }
        if (fileMenu_ && ImGui::BeginMenu("ファイル")) { fileMenu_(); ImGui::EndMenu(); }
        if (ImGui::BeginMenu("表示")) {
            for (auto category : {EditorUiCategory::Authoring, EditorUiCategory::Engine, EditorUiCategory::GameDebug, EditorUiCategory::Tools}) {
                ImGui::PushStyleColor(ImGuiCol_Text, EditorCategoryColor(category)); const bool opened = ImGui::BeginMenu(EditorCategoryName(category)); ImGui::PopStyleColor();
                if (opened) { for (auto &panel : registry_.Panels()) { if (panel.category == category) { ImGui::MenuItem(panel.name.c_str(), nullptr, &panel.isOpen); } } ImGui::EndMenu(); }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("レイアウト")) {
            for (auto preset : {EditorLayoutPreset::Standard, EditorLayoutPreset::SceneFocus, EditorLayoutPreset::Debug}) { if (ImGui::MenuItem(EditorPresetName(preset))) { QueuePreset(preset); } }
            ImGui::Separator(); if (ImGui::MenuItem("現在の配置を名前付き保存")) { savePopup_ = true; }
            if (store_) { std::string error; for (const auto &name : store_->ListNames(error)) { if (ImGui::MenuItem(name.c_str())) { QueueNamedLayout(name); } } }
            if (ImGui::MenuItem("標準へ戻す")) { QueuePreset(EditorLayoutPreset::Standard); } ImGui::EndMenu();
        }
        if (ImGui::GetContentRegionAvail().x > 280 * ImGui::GetFontSize() / 16.0f) { ImGui::SameLine(); ImGui::TextDisabled("配置: %.48s", selectedLayout_.c_str()); }
        if (!message_.empty()) { ImGui::SameLine(); if (ImGui::SmallButton("通知あり")) { SetPanelOpen("workspace.messages", true); } }
        ImGui::EndMainMenuBar();
        if (savePopup_) { ImGui::OpenPopup("配置を保存"); savePopup_ = false; }
        if (ImGui::BeginPopupModal("配置を保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            char buffer[512]{}; const auto count = (std::min)(saveName_.size(), sizeof(buffer) - 1); std::copy_n(saveName_.data(), count, buffer);
            if (ImGui::InputText("名前", buffer, sizeof(buffer))) { saveName_ = buffer; }
            if (ImGui::Button("保存")) { if (SaveLayoutAs(saveName_, false, message_)) { ImGui::CloseCurrentPopup(); } else { if (store_) { const auto names = store_->ListNames(message_); overwritePopup_ = std::find(names.begin(), names.end(), saveName_) != names.end(); } } }
            ImGui::SameLine(); if (ImGui::Button("取消")) { ImGui::CloseCurrentPopup(); }
            if (!message_.empty()) { ImGui::TextWrapped("%s", message_.c_str()); }
            if (overwritePopup_) { ImGui::TextUnformatted("同じ名前の配置を上書きしますか？"); if (ImGui::Button("上書き")) { if (SaveLayoutAs(saveName_, true, message_)) { overwritePopup_ = false; ImGui::CloseCurrentPopup(); } } ImGui::SameLine(); if (ImGui::Button("上書きしない")) { overwritePopup_ = false; } }
            ImGui::EndPopup();
        }
    }
    ///=============================================================================
    /// 共通画面の描画
    void EditorUiSystem::Draw() {
        viewport_.BeginFrame(); if (!configured_) { return; }
        if (!dockspace_) { dockspace_ = ImGui::GetID("MagEngine.EditorDockSpace"); }
        const auto initialSize = ImGui::GetMainViewport()->WorkSize;
        ApplyPending({initialSize.x, (std::max)(1.0f, initialSize.y - toolbarHeight_)});
        DrawMenus();
        const auto *viewport = ImGui::GetMainViewport(); ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize);
        const auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0); ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("MagEngine.Workspace", nullptr, flags);
        const float top = ImGui::GetCursorPosY();
        const float scale = ImGui::GetFontSize() / 16.0f;
        const auto lime = EditorDisplayColor(0xD7F542);
        if (ImGui::GetContentRegionAvail().x > 500 * scale) {
            auto *draw = ImGui::GetWindowDrawList(); const auto p = ImGui::GetCursorScreenPos();
            const auto ink = ImGui::ColorConvertFloat4ToU32(lime);
            draw->AddLine({p.x, p.y + 3 * scale}, {p.x, p.y + 17 * scale}, ink, 3 * scale);
            draw->AddCircle({p.x + 13 * scale, p.y + 10 * scale}, 5 * scale, ink, 12, scale);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 26 * scale);
            ImFont *heading = nullptr;
            // アトラス再構築でポインタが変わるため、毎フレーム現在のフォントを参照する。
            for (auto *font : ImGui::GetIO().Fonts->Fonts) if (font->FontSize > 17 * scale && font->FontSize < 20 * scale) { heading = font; break; }
            if (heading) ImGui::PushFont(heading);
            ImGui::TextColored(lime, "MAGENGINE");
            if (heading) ImGui::PopFont();
            ImGui::SameLine(); ImGui::TextDisabled("/ CREATIVE SYSTEM");
        }
        if (toolbar_) { toolbar_(); } else { ImGui::TextUnformatted("実行中"); }
        ImGui::Separator(); const auto size = ImGui::GetContentRegionAvail();
        toolbarHeight_ = ImGui::GetCursorPosY() - top + ImGui::GetStyle().WindowPadding.y * 2;
        ImGui::DockSpace(dockspace_, size, ImGuiDockNodeFlags_None); ImGui::End(); ImGui::PopStyleVar(2);
        DrawPanels();
    }
    void EditorUiSystem::DrawPanels() {
        for (auto &panel : registry_.Panels()) {
            if (panel.id == "workspace.viewport") { panel.isOpen = true; }
            if (!panel.isOpen || !panel.drawFunc) { continue; }
            const auto color = EditorCategoryColor(panel.category); const auto name = panel.name + "###" + panel.id;
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, {color.x * 0.28f, color.y * 0.28f, color.z * 0.28f, 1});
            ImGui::PushStyleColor(ImGuiCol_TabSelected, {color.x * 0.32f, color.y * 0.32f, color.z * 0.32f, 1});
            if (ImGui::Begin(name.c_str(), panel.id == "workspace.viewport" ? nullptr : &panel.isOpen, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_HorizontalScrollbar)) {
                if (!ImGui::IsWindowDocked()) {
                    const auto *v = ImGui::GetMainViewport(); const auto p = ImGui::GetWindowPos(); const auto s = ImGui::GetWindowSize();
                    const ImVec2 bounded{std::clamp(p.x, v->WorkPos.x, v->WorkPos.x + (std::max)(0.0f, v->WorkSize.x - s.x)), std::clamp(p.y, v->WorkPos.y, v->WorkPos.y + (std::max)(0.0f, v->WorkSize.y - s.y))};
                    ImGui::SetWindowPos(bounded);
                    if (v->WorkSize.x > 0 && v->WorkSize.y > 0) { ImGui::SetWindowSize({(std::min)(s.x, v->WorkSize.x), (std::min)(s.y, v->WorkSize.y)}); }
                }
                ImGui::TextColored(color, "%s", EditorCategoryName(panel.category));
                const auto p = ImGui::GetCursorScreenPos(); const float width = ImGui::GetContentRegionAvail().x;
                if (width > 120) {
                    auto *draw = ImGui::GetWindowDrawList(); const auto ink = ImGui::ColorConvertFloat4ToU32(color);
                    draw->AddLine(p, {p.x + 28, p.y}, ink);
                    draw->AddLine({p.x + 28, p.y}, {p.x + 32, p.y - 4}, ink);
                }
                ImGui::Separator(); panel.drawFunc();
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                    const auto p0 = ImGui::GetWindowPos(); const auto s = ImGui::GetWindowSize();
                    ImGui::GetWindowDrawList()->AddLine({p0.x + 1, p0.y + s.y - 1}, {p0.x + s.x - 1, p0.y + s.y - 1}, ImGui::ColorConvertFloat4ToU32(EditorDisplayColor(0x4ED9E8)));
                }
            }
            ImGui::End(); ImGui::PopStyleColor(2);
        }
    }
}

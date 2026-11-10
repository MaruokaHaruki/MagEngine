/*********************************************************************
 * \file EditorUiSystem.h
 * \brief 制作画面の共通シェル、分類、配置保存を管理する
 *********************************************************************/
#pragma once
#include "EditorPanelRegistry.h"
#include "EditorWorkspaceLayout.h"
#include "EditorLayoutStore.h"
#include "engine/base/core/WindowSurfaceState.h"
#include "engine/editor/EditorViewport.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
namespace MagEngine {
    struct EngineContext;
    class EditorUiSystem {
    public:
        void Initialize();
        void Finalize();
        void RegisterEnginePanels(const EngineContext &, std::function<void()> rendering, std::function<void()> performance);
        bool RegisterPanel(EditorUiPanelDesc panel);
        bool RegisterPanel(const std::string &id, const std::string &name, EditorUiCategory category, EditorPanelScope scope, EditorDockRegion region, bool open, std::function<void()> draw);
        void UnregisterPanel(const std::string &id);
        void ClearScenePanels();
        void SetPanelOpen(const std::string &id, bool open);
        bool IsPanelOpen(const std::string &id) const;
        /// @brief パネル登録後に呼ぶ。実際の配置適用は次のUIフレーム開始まで遅延する
        void ConfigureWorkspace(std::filesystem::path projectRoot, EditorWorkspaceMode mode, std::filesystem::path testStore = {});
        void Draw();
		/// @brief GPU更新成功後の画像寸法を表示と入力へ渡す
		void SetRenderSize(SurfaceSize size) { renderSize_ = size; }
        void UseCreationLayout() { QueuePreset(EditorLayoutPreset::Standard); }
        EditorViewport &GetViewport() { return viewport_; }
        void SetToolbar(std::function<void()> toolbar) { toolbar_ = std::move(toolbar); }
        void SetAdditionalMessages(std::function<void()> callback) { additionalMessages_ = std::move(callback); }
        void SetFileMenu(std::function<void()> callback) { fileMenu_ = std::move(callback); }
        void SetPanelTitle(std::string_view id, std::string title) { if (auto *panel = registry_.Find(id)) { panel->name = std::move(title); } }
        void SetBeforeLayoutChange(std::function<void()> callback) { beforeLayoutChange_ = std::move(callback); }
        void SetSceneTexture(std::function<uint64_t()> callback) { sceneTexture_ = std::move(callback); }
        void QueuePreset(EditorLayoutPreset preset) { pendingPreset_ = preset; pendingRestore_.reset(); }
        void QueueRestore(EditorLayoutSnapshot snapshot) { pendingRestore_ = std::move(snapshot); pendingPreset_.reset(); }
        void QueueNamedLayout(std::string name);
        EditorLayoutSnapshot CaptureLayout(std::string name);
        bool SaveLayoutAs(std::string_view name, bool overwrite, std::string &error);
        bool SaveWorkspaceSession(std::string &error);
        const std::string &GetMessage() const { return message_; }
    private:
        void DrawMenus();
        void DrawPanels();
        void ApplyPending(ImVec2 size);
        EditorPanelRegistry registry_;
        EditorWorkspaceLayout layout_;
        EditorViewport viewport_;
		SurfaceSize renderSize_{};
        std::unique_ptr<EditorLayoutStore> store_;
        std::optional<EditorLayoutPreset> pendingPreset_;
        std::optional<EditorLayoutSnapshot> pendingRestore_;
        std::map<std::string, bool> savedPanelOpen_;
        std::set<std::string> dockedIds_;
        std::function<void()> toolbar_, beforeLayoutChange_;
        std::function<void()> additionalMessages_;
        std::function<void()> fileMenu_;
        std::function<uint64_t()> sceneTexture_;
        EditorWorkspaceMode mode_ = EditorWorkspaceMode::Game;
        std::string selectedLayout_ = "標準", message_, saveName_;
        ImGuiID dockspace_ = 0;
		float toolbarHeight_ = 0; // 直前のフレームで実測したドック以外の高さ
        bool configured_ = false;
        bool sessionWritable_ = true;
        bool savePopup_ = false;
        bool overwritePopup_ = false;
    };
}

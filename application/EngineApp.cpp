/*********************************************************************
 * \file EngineApp.cpp
 * \brief 制作文書を共通エンジンへ接続する最小の起動アプリ
 *********************************************************************/
#include "EngineApp.h"
#include "Logger.h"
namespace MagEngine {
    ///=============================================================================
    /// 空文書または明示した文書の初期化
    void EngineApp::InitializeApplication() {
        editorHost_ = std::make_unique<EditorHost>();
        editorHost_->Initialize(engineContext_, launchOptions_);
#ifdef _DEBUG
        editorUiSystem_->ConfigureWorkspace(launchOptions_.projectRoot, launchOptions_.editor ? EditorWorkspaceMode::Edit : EditorWorkspaceMode::Game);
#endif // _DEBUG
        win_->SetCloseRequestHandler([this] { return editorHost_->CanClose(); });
    }
    ///=============================================================================
    /// 文書と再生セッションの更新
    void EngineApp::UpdateApplication(const FrameTime &frameTime) {
        if (editorHost_) editorHost_->Update(frameTime);
    }
    ///=============================================================================
    /// 編集対象の描画登録
    void EngineApp::RegisterApplicationRenderables(RenderWorld &renderWorld) {
        if (editorHost_) editorHost_->RegisterRenderables(renderWorld);
    }
    ///=============================================================================
    /// 部分初期化にも対応した終了処理
    void EngineApp::FinalizeApplication() {
#ifdef _DEBUG
        if (editorUiSystem_) { std::string error; if (!editorUiSystem_->SaveWorkspaceSession(error)) Logger::Log(error, Logger::LogLevel::Warning); }
#endif // _DEBUG
        if (editorHost_) {
            if (win_) win_->SetCloseRequestHandler({});
            editorHost_->Finalize(); editorHost_.reset();
        }
    }
}

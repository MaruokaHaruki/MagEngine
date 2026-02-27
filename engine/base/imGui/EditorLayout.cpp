/*********************************************************************
 * \file   EditorLayout.cpp
 * \brief  エディターレイアウト管理実装
 *
 * \author Harukichimaru
 * \date   February 2025
 *********************************************************************/
#include "EditorLayout.h"
#include "ConsolePanel.h"
#include "DirectXCore.h"
#include "GameViewportPanel.h"
#include "HierarchyPanel.h"
#include "ImguiSetup.h"
#include "InspectorPanel.h"
#include "PostEffectManager.h"
#include "ToolsPanel.h"
#include "imgui.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	EditorLayout::EditorLayout() {
		// パネルの生成
		viewportPanel_ = std::make_unique<GameViewportPanel>();
		consolePanel_ = std::make_unique<ConsolePanel>();
		toolsPanel_ = std::make_unique<ToolsPanel>();
	}

	EditorLayout::~EditorLayout() {
		Finalize();
	}

	void EditorLayout::Initialize(DirectXCore *dxCore, PostEffectManager *postEffectManager, class ImguiSetup *imguiSetup) {
		dxCore_ = dxCore;
		postEffectManager_ = postEffectManager;
		imguiSetup_ = imguiSetup;

		// 各パネルを初期化
		viewportPanel_->Initialize(&editorState_, dxCore_);
		consolePanel_->Initialize(&editorState_, dxCore_);
		toolsPanel_->Initialize(&editorState_, dxCore_);

		// ToolsPanelにPostEffectManagerを設定
		toolsPanel_->SetPostEffectManager(postEffectManager_);

		// ImGuiのDockSpace設定
		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	void EditorLayout::Update() {
		viewportPanel_->Update();
		consolePanel_->Update();
		toolsPanel_->Update();
	}

	void EditorLayout::Draw() {
		// DockSpace設定
		SetupDockspace();

		// メニューバー描画
		DrawMenuBar();

		// 各パネルの描画
		if (editorState_.panelVisibility.viewport) {
			viewportPanel_->Draw();
		}
		if (editorState_.panelVisibility.console) {
			consolePanel_->Draw();
		}
		if (editorState_.panelVisibility.tools) {
			toolsPanel_->Draw();
		}

		// パフォーマンスモニターの描画
#ifdef _DEBUG
		if (editorState_.panelVisibility.performanceMonitor && imguiSetup_) {
			imguiSetup_->ShowPerformanceMonitor();
		}
#endif
	}

	void EditorLayout::Finalize() {
		if (viewportPanel_)
			viewportPanel_->Finalize();
		if (consolePanel_)
			consolePanel_->Finalize();
		if (toolsPanel_)
			toolsPanel_->Finalize();
	}

	void EditorLayout::ShowAllPanels() {
		editorState_.panelVisibility.viewport = true;
		editorState_.panelVisibility.console = true;
		editorState_.panelVisibility.tools = true;
		editorState_.panelVisibility.performanceMonitor = true;
	}

	void EditorLayout::HideAllPanels() {
		editorState_.panelVisibility.viewport = false;
		editorState_.panelVisibility.console = false;
		editorState_.panelVisibility.tools = false;
		editorState_.panelVisibility.performanceMonitor = false;
	}

	void EditorLayout::ResetLayout() {
		editorState_ = EditorState();
		ShowAllPanels();
	}

	void EditorLayout::SetupDockspace() {
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Window", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		ImGui::End();
	}

	void EditorLayout::DrawMenuBar() {
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Viewport", nullptr, &editorState_.panelVisibility.viewport);
				ImGui::MenuItem("Console", nullptr, &editorState_.panelVisibility.console);
				ImGui::MenuItem("Tools", nullptr, &editorState_.panelVisibility.tools);
#ifdef _DEBUG
				ImGui::MenuItem("Performance Monitor", nullptr, &editorState_.panelVisibility.performanceMonitor);
#endif
				ImGui::Separator();
				if (ImGui::MenuItem("Reset Layout")) {
					ResetLayout();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Editor")) {
				if (ImGui::MenuItem("Play", "Space")) {
					editorState_.isPlayMode = !editorState_.isPlayMode;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

}

/*********************************************************************
 * \file EngineStartupTests.cpp
 * \brief 空文書と指定文書を実エンジンで2フレーム描画して終了する
 *********************************************************************/
#include "application/EngineApp.h"
#include <iostream>
#include <stdexcept>
namespace {
	class StartupApp final : public MagEngine::EngineApp {
	public:
		explicit StartupApp(const MagEngine::EditorLaunchOptions &options) : EngineApp(options), root_(options.projectRoot), editing_(options.editor) {}
	private:
		void InitializeApplication() override {
			EngineApp::InitializeApplication();
#ifdef _DEBUG
			// 配置の自動保存もテスト専用領域へ限定する。
			editorUiSystem_->ConfigureWorkspace(root_, editing_ ? MagEngine::EditorWorkspaceMode::Edit : MagEngine::EditorWorkspaceMode::Game, root_ / "workspace-layouts");
#endif // _DEBUG
		}
		void UpdateApplication(const FrameTime &time) override {
			EngineApp::UpdateApplication(time);
			if (++frames_ == 3) isEndRequest_ = true;
		}
		int frames_ = 0;
		std::filesystem::path root_;
		bool editing_;
	};
}
int main(int argc, char **argv) {
	try {
		using namespace MagEngine;
		if (argc < 3) throw std::runtime_error("source root and fixture root required");
		const auto source = std::filesystem::absolute(argv[1]);
		const auto root = std::filesystem::absolute(argv[2]) / "engine-startup";
		std::filesystem::create_directories(root);
		// 元資産にテスト文書や配置状態を書かないため、実行用コピーを使用する。
		std::filesystem::copy(source / "resources", root / "resources", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		EditorLaunchOptions options;
		if (argc > 3) {
			EditorDocument document; EditorObjectRecord object; object.modelPath = "model/Bullet.obj"; AddObject(document, object);
			EditorDocumentStore store(root); std::string error;
			options.scene = "resources/config/scenes/startup-scene.json";
			std::filesystem::create_directories((root / options.scene).parent_path());
			std::error_code ignored; std::filesystem::remove(root / options.scene, ignored);
			if (!store.Save(root / options.scene, document, error)) throw std::runtime_error(error);
		}
#ifdef _DEBUG
		constexpr bool editorAvailable = true;
#else
		constexpr bool editorAvailable = false;
#endif // _DEBUG
		std::string error;
		if (!ResolveEngineLaunchOptions(options, root, editorAvailable, error)) throw std::runtime_error(error);
		std::filesystem::current_path(root);
		StartupApp app(options); app.Run();
		std::cout << "EngineStartup PASS: " << (options.scene.empty() ? "empty" : "explicit scene") << '\n'; return 0;
	} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}

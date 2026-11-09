/*********************************************************************
 * \file EditorHost.h
 * \brief Game側で編集文書の描画実体と再生セッションを所有する
 *********************************************************************/
#pragma once
#include "EditorLaunchOptions.h"
#include "engine/editor/EditorDocumentStore.h"
#include "engine/editor/EditorPanels.h"
#include "engine/base/framework/EngineContext.h"
#include "engine/base/time/FrameTime.h"
#include "Camera.h"
#include "Object3d.h"
#include "DescriptorHandle.h"
#include <array>
#include <map>
namespace MagEngine {
	class EditorHost {
	public:
		void Initialize(const EngineContext &context, const EditorLaunchOptions &options);
		bool ApplyDocument(const EditorDocument &document, std::string &error);
		void Update(const FrameTime &frameTime);
		void RegisterRenderables(RenderWorld &world);
		void Finalize();
		bool CanClose();
		/// @brief 再生用のゲーム側更新。外部資源を持つ場合は終了前に解除する
		void SetRuntimeUpdate(std::function<void(const FrameTime &, EditorDocument &)> callback) { runtimeUpdate_ = std::move(callback); }
		EditorTypeRegistry &GetTypeRegistry() { return registry_; }
		bool IsViewportInputEnabled() const { return panels_ && panels_->viewportFocused; }
	private:
		enum class Request { None, Save, Load, Import, Play, Stop };
		void Save();
		void ProcessRequest();
		EditorBounds Bounds(const EditorObjectRecord &record) const;
		const EngineContext *context_ = nullptr; // Framework所有。Finalizeまで有効
		EditorLaunchOptions options_;
		EditorDocument document_, saved_, applied_;
		EditorHistory history_;
		EditorPlaySession session_;
		EditorTypeRegistry registry_;
		std::unique_ptr<EditorDocumentStore> store_;
		std::unique_ptr<EditorPanels> panels_;
		std::map<EditorObjectId, std::unique_ptr<EditorRenderable>> objects_;
		std::map<EditorObjectId, EditorBounds> bounds_;
		Camera camera_;
		std::array<DescriptorHandle, 2> imageDescriptors_{};
		std::function<void(const FrameTime &, EditorDocument &)> runtimeUpdate_;
		Request request_ = Request::None;
		bool hasApplied_ = false;
	};
}

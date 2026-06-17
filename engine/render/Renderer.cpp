#include "Renderer.h"

#include "IRenderPass.h"
#include "OpaqueRenderPass.h"
#include "RenderContext.h"
#include "RenderWorld.h"

namespace MagEngine {
	void Renderer::Initialize(Object3dSetup &object3dSetup) {
		renderPasses_.clear();
		renderPasses_.push_back(std::make_unique<OpaqueRenderPass>(object3dSetup));
	}

	void Renderer::Render(RenderContext &renderContext, const RenderWorld &renderWorld) {
		for(const std::unique_ptr<IRenderPass> &renderPass : renderPasses_) {
			renderPass->Execute(renderContext, renderWorld);
		}
	}
}

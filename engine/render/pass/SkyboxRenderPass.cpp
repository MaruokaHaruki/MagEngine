#include "SkyboxRenderPass.h"

#include "RenderWorld.h"
#include "Skybox.h"
#include "SkyboxSetup.h"

namespace MagEngine {
	SkyboxRenderPass::SkyboxRenderPass(SkyboxSetup &skyboxSetup)
		: skyboxSetup_(skyboxSetup) {
	}

	void SkyboxRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const SkyboxRenderItem *item = renderWorld.GetSkybox();
		if(!item || !item->visible || !item->skybox) {
			return;
		}

		skyboxSetup_.CommonDrawSetup();
		item->skybox->Draw();
	}
}

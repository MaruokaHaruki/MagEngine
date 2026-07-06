#include "CloudRenderPass.h"

#include "Cloud.h"
#include "CloudSetup.h"
#include "RenderWorld.h"

#include <cassert>

namespace MagEngine {
	CloudRenderPass::CloudRenderPass(CloudSetup &cloudSetup)
		: cloudSetup_(cloudSetup) {
	}

	void CloudRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const CloudRenderItem *item = renderWorld.GetCloud();
		if(!item) {
			return;
		}

		assert(item->cloud && "Registered CloudRenderItem must have a Cloud pointer.");
		if(!item->visible || !item->cloud) {
			return;
		}

		cloudSetup_.CommonDrawSetup();
		item->cloud->Draw();
	}
}

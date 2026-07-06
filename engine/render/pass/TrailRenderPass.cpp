#include "TrailRenderPass.h"

#include "RenderWorld.h"
#include "TrailEffect.h"
#include "TrailEffectSetup.h"

#include <cassert>

namespace MagEngine {
	TrailRenderPass::TrailRenderPass(TrailEffectSetup &trailEffectSetup)
		: trailEffectSetup_(trailEffectSetup) {
	}

	void TrailRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const std::vector<TrailRenderItem> &items = renderWorld.GetTrailItems();
		if (items.empty()) {
			return;
		}

		trailEffectSetup_.CommonDrawSetup();

		for (const TrailRenderItem &item : items) {
			assert(item.trail && "TrailRenderItem::trail must not be null.");
			if (!item.visible || item.trail == nullptr) {
				continue;
			}

			item.trail->Draw();
		}
	}
}

#include "SpriteRenderPass.h"

#include "RenderWorld.h"
#include "Sprite.h"
#include "SpriteSetup.h"

#include <cassert>

namespace MagEngine {
	SpriteRenderPass::SpriteRenderPass(SpriteSetup &spriteSetup)
		: spriteSetup_(spriteSetup) {
	}

	void SpriteRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const std::vector<SpriteRenderItem> &items = renderWorld.GetSpriteItems();
		if (items.empty()) {
			return;
		}

		SpriteRenderMode currentRenderMode = SpriteRenderMode::Ui;
		bool hasBoundPipeline = false;
		for (const SpriteRenderItem &item : items) {
			assert(item.sprite && "SpriteRenderItem::sprite must not be null.");
			if (!item.visible || item.sprite == nullptr) {
				continue;
			}

			if (!hasBoundPipeline || currentRenderMode != item.renderMode) {
				currentRenderMode = item.renderMode;
				spriteSetup_.CommonDrawSetup(currentRenderMode);
				hasBoundPipeline = true;
			}

			item.sprite->Draw();
		}
	}
}

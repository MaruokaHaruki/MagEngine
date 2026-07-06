#include "LineRenderPass.h"

#include "LineManager.h"
#include "RenderWorld.h"

#include <algorithm>
#include <cassert>

namespace MagEngine {
	LineRenderPass::LineRenderPass(LineManager &lineManager)
		: lineManager_(lineManager) {
	}

	void LineRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const std::vector<LineRenderItem> &items = renderWorld.GetLineItems();
		if(items.empty()) {
			return;
		}

		const auto itemIt = std::find_if(items.begin(), items.end(), [&](const LineRenderItem &item) {
			return item.visible && item.lineManager == &lineManager_;
		});
		if(itemIt == items.end()) {
			return;
		}

		assert(itemIt->lineManager && "LineRenderItem::lineManager must not be null.");
		// NOTE: LineManagerは内部LineバッファをDraw後にクリアするため、1フレーム1回だけ実行する。
		lineManager_.Draw();
	}
}

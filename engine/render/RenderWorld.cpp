#include "RenderWorld.h"

namespace MagEngine {
	void RenderWorld::Clear() {
		opaqueItems_.clear();
	}

	void RenderWorld::AddOpaque(const OpaqueRenderItem &item) {
		opaqueItems_.push_back(item);
	}
}

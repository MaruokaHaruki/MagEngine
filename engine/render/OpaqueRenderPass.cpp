#include "OpaqueRenderPass.h"

#include "Object3d.h"
#include "Object3dSetup.h"
#include "RenderContext.h"
#include "RenderWorld.h"

namespace MagEngine {
	OpaqueRenderPass::OpaqueRenderPass(Object3dSetup &object3dSetup)
		: object3dSetup_(object3dSetup) {
	}

	void OpaqueRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		object3dSetup_.CommonDrawSetup();

		for(const OpaqueRenderItem &item : renderWorld.GetOpaqueItems()) {
			if(!item.visible || !item.object) {
				continue;
			}
			item.object->Draw();
		}
	}
}

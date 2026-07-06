#include "ParticleRenderPass.h"

#include "Particle.h"
#include "ParticleSetup.h"
#include "RenderWorld.h"

namespace MagEngine {
	ParticleRenderPass::ParticleRenderPass(ParticleSetup &particleSetup)
		: particleSetup_(particleSetup) {
	}

	void ParticleRenderPass::Execute(RenderContext &, const RenderWorld &renderWorld) {
		const std::vector<ParticleRenderItem> &items = renderWorld.GetParticleItems();
		if(items.empty()) {
			return;
		}

		particleSetup_.CommonDrawSetup();

		for(const ParticleRenderItem &item : items) {
			if(!item.visible || !item.particle) {
				continue;
			}
			item.particle->Draw();
		}
	}
}

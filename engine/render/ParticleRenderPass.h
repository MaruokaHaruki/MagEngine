/*********************************************************************
 * \file   ParticleRenderPass.h
 * \brief  Particle描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class ParticleSetup;

	class ParticleRenderPass final : public IRenderPass {
	public:
		explicit ParticleRenderPass(ParticleSetup &particleSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: Setupの所有権はMagFrameworkにあるため、Passは描画状態の参照だけを保持する。
		ParticleSetup &particleSetup_;
	};
}

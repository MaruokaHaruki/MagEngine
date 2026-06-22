/*********************************************************************
 * \file   TrailRenderPass.h
 * \brief  Trail描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class TrailEffectSetup;

	class TrailRenderPass final : public IRenderPass {
	public:
		explicit TrailRenderPass(TrailEffectSetup &trailEffectSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: PSO/RootSignatureはSetupが所有し、Passは描画時の状態設定だけを行う。
		TrailEffectSetup &trailEffectSetup_;
	};
}

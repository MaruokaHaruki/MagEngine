/*********************************************************************
 * \file   SkyboxRenderPass.h
 * \brief  Skybox描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class SkyboxSetup;

	class SkyboxRenderPass final : public IRenderPass {
	public:
		explicit SkyboxRenderPass(SkyboxSetup &skyboxSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: Setupの所有権はMagFrameworkにあるため、Passは描画状態の参照だけを保持する。
		SkyboxSetup &skyboxSetup_;
	};
}

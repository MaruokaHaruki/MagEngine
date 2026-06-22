/*********************************************************************
 * \file   CloudRenderPass.h
 * \brief  Cloud描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class CloudSetup;

	class CloudRenderPass final : public IRenderPass {
	public:
		explicit CloudRenderPass(CloudSetup &cloudSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: Setupの所有権はMagFrameworkにあるため、Passは描画状態の参照だけを保持する。
		CloudSetup &cloudSetup_;
	};
}

/*********************************************************************
 * \file   LineRenderPass.h
 * \brief  LineManagerに蓄積されたLine描画をRenderPassとして実行する
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class LineManager;

	class LineRenderPass final : public IRenderPass {
	public:
		explicit LineRenderPass(LineManager &lineManager);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: LineManagerの所有権はMagFrameworkに残し、Passは描画実行だけを担当する。
		LineManager &lineManager_;
	};
}

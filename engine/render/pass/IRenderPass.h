/*********************************************************************
 * \file   IRenderPass.h
 * \brief  RenderPass共通インターフェース
 *********************************************************************/
#pragma once

namespace MagEngine {
	struct RenderContext;
	class RenderWorld;

	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		virtual void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) = 0;
	};
}

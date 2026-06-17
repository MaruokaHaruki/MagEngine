/*********************************************************************
 * \file   Renderer.h
 * \brief  RenderPassの所有と実行を担当するクラス
 *********************************************************************/
#pragma once

#include <memory>
#include <vector>

#include "IRenderPass.h"

namespace MagEngine {
	class Object3dSetup;
	struct RenderContext;
	class RenderWorld;

	class Renderer {
	public:
		void Initialize(Object3dSetup &object3dSetup);
		void Render(RenderContext &renderContext, const RenderWorld &renderWorld);

	private:
		// NOTE: Passの実行順は描画順そのものなので、登録順を明示的に維持する。
		std::vector<std::unique_ptr<IRenderPass>> renderPasses_;
	};
}

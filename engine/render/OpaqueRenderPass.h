/*********************************************************************
 * \file   OpaqueRenderPass.h
 * \brief  3D不透明描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class Object3dSetup;

	class OpaqueRenderPass final : public IRenderPass {
	public:
		explicit OpaqueRenderPass(Object3dSetup &object3dSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: Setupの所有権はMagFrameworkにあるため、Passは非所有参照だけを保持する。
		Object3dSetup &object3dSetup_;
	};
}

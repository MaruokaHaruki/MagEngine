/*********************************************************************
 * \file   PostEffectRenderPass.h
 * \brief  SceneColorからPresentColorへ最終合成するRenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class DirectXCore;
	class PostEffectManager;
	class TextureManager;

	class PostEffectRenderPass final : public IRenderPass {
	public:
		PostEffectRenderPass(DirectXCore &dxCore, PostEffectManager &postEffectManager, TextureManager &textureManager);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: SwapChainの所有権はDirectXCoreに残し、PassはPresentColorへの描画準備だけを依頼する。
		DirectXCore &dxCore_;
		PostEffectManager &postEffectManager_;
		TextureManager &textureManager_;
	};
}

#include "PostEffectRenderPass.h"

#include "DirectXCore.h"
#include "PostEffectManager.h"
#include "TextureManager.h"

namespace MagEngine {
	PostEffectRenderPass::PostEffectRenderPass(DirectXCore &dxCore, PostEffectManager &postEffectManager, TextureManager &textureManager)
		: dxCore_(dxCore), postEffectManager_(postEffectManager), textureManager_(textureManager) {
	}

	void PostEffectRenderPass::Execute(RenderContext &, const RenderWorld &) {
		// NOTE: PresentやFenceはフレーム終了処理に残し、ここではBackBufferへのFullscreen描画だけを行う。
		dxCore_.BeginPresentRenderTarget();
		postEffectManager_.ApplyEffects(textureManager_);
	}
}

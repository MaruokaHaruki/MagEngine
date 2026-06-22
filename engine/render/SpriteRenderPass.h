/*********************************************************************
 * \file   SpriteRenderPass.h
 * \brief  Sprite描画用RenderPass
 *********************************************************************/
#pragma once

#include "IRenderPass.h"

namespace MagEngine {
	class SpriteSetup;

	class SpriteRenderPass final : public IRenderPass {
	public:
		explicit SpriteRenderPass(SpriteSetup &spriteSetup);

		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;

	private:
		// NOTE: SpriteSetupが所有するPSO/RootSignatureを利用し、Passは描画状態の設定だけを担当する。
		SpriteSetup &spriteSetup_;
	};
}

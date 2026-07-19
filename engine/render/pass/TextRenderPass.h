#pragma once
#include "IRenderPass.h"
namespace MagEngine {
	/// @brief TextRendererが登録した文字列を通常のスプライト描画後に実行するRenderPass
	/// @note テキスト専用の描画順を維持するため、RenderGraph上の登録順を変更する場合はUIとの前後関係を確認する。
	class TextRenderPass final : public IRenderPass {
	public:
		/// @brief 現フレームの文字列描画コマンドをGPUへ発行
		void Execute(RenderContext &renderContext, const RenderWorld &renderWorld) override;
	};
}

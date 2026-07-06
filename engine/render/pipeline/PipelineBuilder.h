/*********************************************************************
 * \file   PipelineBuilder.h
 * \brief  PipelineRecipeからD3D12 PSOを生成する共通Builder
 *********************************************************************/
#pragma once

#include "PipelineRecipe.h"

#include <d3d12.h>
#include <wrl.h>

namespace MagEngine {
	class DirectXCore;

	class PipelineBuilder {
	public:
		PipelineBuilder(ID3D12Device &device, DirectXCore &shaderCompiler);

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateGraphicsPipeline(const PipelineRecipe &recipe) const;

	private:
		ID3D12Device &device_;
		DirectXCore &shaderCompiler_;
	};
}

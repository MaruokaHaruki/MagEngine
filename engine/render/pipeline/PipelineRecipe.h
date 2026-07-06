/*********************************************************************
 * \file   PipelineRecipe.h
 * \brief  各描画SetupがPSO生成条件を宣言するための軽量Recipe
 *********************************************************************/
#pragma once

#include <d3d12.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace MagEngine {
	struct PipelineShaderSource {
		std::filesystem::path path;
		std::wstring entryPoint = L"main";
		std::wstring profile;
	};

	enum class PipelineRecipeValidationError : uint8_t {
		None,
		MissingVertexShader,
		MissingPixelShader,
		MissingRootSignature,
		MissingRenderTargetFormat,
	};

	struct PipelineRecipeValidationResult {
		bool isValid = true;
		PipelineRecipeValidationError error = PipelineRecipeValidationError::None;
	};

	struct PipelineRecipe {
		PipelineShaderSource vertexShader;
		PipelineShaderSource pixelShader;
		ID3D12RootSignature *rootSignature = nullptr;
		D3D12_BLEND_DESC blendState{};
		D3D12_RASTERIZER_DESC rasterizerState{};
		D3D12_DEPTH_STENCIL_DESC depthStencilState{};
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
		DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// NOTE: GPUを使わないCPUテストでRecipeの必須項目だけを検証する。
		[[nodiscard]]
		PipelineRecipeValidationResult Validate() const;
	};
}

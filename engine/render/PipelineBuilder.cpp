#include "PipelineBuilder.h"

#include "DirectXCore.h"

#include <dxcapi.h>
#include <stdexcept>

namespace MagEngine {
	PipelineBuilder::PipelineBuilder(ID3D12Device &device, DirectXCore &shaderCompiler)
		: device_(device),
		  shaderCompiler_(shaderCompiler) {
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineBuilder::CreateGraphicsPipeline(const PipelineRecipe &recipe) const {
		const PipelineRecipeValidationResult validation = recipe.Validate();
		if(!validation.isValid) {
			throw std::runtime_error("PipelineRecipe validation failed.");
		}

		// NOTE: 既存のCompileShaderを再利用し、エントリーポイント変更やReflectionは今回導入しない。
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
			shaderCompiler_.CompileShader(recipe.vertexShader.path.wstring(), recipe.vertexShader.profile.c_str());
		if(!vertexShaderBlob) {
			throw std::runtime_error("PipelineBuilder failed to compile vertex shader.");
		}

		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
			shaderCompiler_.CompileShader(recipe.pixelShader.path.wstring(), recipe.pixelShader.profile.c_str());
		if(!pixelShaderBlob) {
			throw std::runtime_error("PipelineBuilder failed to compile pixel shader.");
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
		desc.pRootSignature = recipe.rootSignature;
		desc.InputLayout = {recipe.inputLayout.data(), static_cast<UINT>(recipe.inputLayout.size())};
		desc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
		desc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
		desc.BlendState = recipe.blendState;
		desc.RasterizerState = recipe.rasterizerState;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = recipe.renderTargetFormat;
		desc.PrimitiveTopologyType = recipe.primitiveTopologyType;
		desc.SampleDesc.Count = 1;
		desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		desc.DepthStencilState = recipe.depthStencilState;
		desc.DSVFormat = recipe.depthStencilFormat;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
		const HRESULT hr = device_.CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));
		if(FAILED(hr)) {
			throw std::runtime_error("PipelineBuilder failed to create graphics pipeline state.");
		}
		return pipelineState;
	}
}

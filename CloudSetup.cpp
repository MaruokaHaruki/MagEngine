#include "CloudSetup.h"
#include "DirectXCore.h"
#include "Logger.h"
#include <stdexcept>
using namespace Logger;

void CloudSetup::Initialize(DirectXCore *dxCore) {
	// if (!dxCore) {
	//	throw std::invalid_argument("CloudSetup::Initialize requires DirectXCore.");
	// }
	dxCore_ = dxCore;
	CreateGraphicsPipeline();
}

void CloudSetup::CommonDrawSetup() {
	auto commandList = dxCore_->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void CloudSetup::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.BaseShaderRegister = 0;
	descriptorRange.NumDescriptors = 1;
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 1;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.ShaderRegister = 0;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = rootParameters;
	desc.NumParameters = _countof(rootParameters);
	desc.pStaticSamplers = &staticSampler;
	desc.NumStaticSamplers = 1;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		throw std::runtime_error(errorBlob ? reinterpret_cast<const char *>(errorBlob->GetBufferPointer())
										   : "CloudSetup root signature serialization failed.");
	}
	hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
												   IID_PPV_ARGS(&rootSignature_));
	if (FAILED(hr)) {
		throw std::runtime_error("CloudSetup root signature creation failed.");
	}
	Log("Cloud root signature created.", LogLevel::Success);
}

void CloudSetup::CreateGraphicsPipeline() {
	CreateRootSignature();

	D3D12_INPUT_ELEMENT_DESC elements[2] = {};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].AlignedByteOffset = 0;

	elements[1].SemanticName = "TEXCOORD";
	elements[1].SemanticIndex = 0;
	elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayout{};
	inputLayout.pInputElementDescs = elements;
	inputLayout.NumElements = _countof(elements);

	Microsoft::WRL::ComPtr<IDxcBlob> vs = dxCore_->CompileShader(L"resources/shader/Cloud.VS.hlsl", L"vs_6_0");
	if (!vs) {
		throw std::runtime_error("Cloud vertex shader compile failed.");
	}
	Microsoft::WRL::ComPtr<IDxcBlob> ps = dxCore_->CompileShader(L"resources/shader/Cloud.PS.hlsl", L"ps_6_0");
	if (!ps) {
		throw std::runtime_error("Cloud pixel shader compile failed.");
	}

	D3D12_BLEND_DESC blend{};
	blend.AlphaToCoverageEnable = FALSE;
	blend.IndependentBlendEnable = FALSE;
	D3D12_RENDER_TARGET_BLEND_DESC rtBlend{};
	rtBlend.BlendEnable = TRUE;
	rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blend.RenderTarget[0] = rtBlend;

	D3D12_RASTERIZER_DESC raster{};
	raster.CullMode = D3D12_CULL_MODE_NONE;
	raster.FillMode = D3D12_FILL_MODE_SOLID;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.InputLayout = inputLayout;
	desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
	desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
	desc.BlendState = blend;
	desc.RasterizerState = raster;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.SampleDesc.Count = 1;

	D3D12_DEPTH_STENCIL_DESC depth{};
	depth.DepthEnable = FALSE;
	depth.StencilEnable = FALSE;
	desc.DepthStencilState = depth;

	HRESULT hr = dxCore_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		throw std::runtime_error("Cloud graphics pipeline creation failed.");
	}
	Log("Cloud graphics pipeline created.", LogLevel::Success);
}

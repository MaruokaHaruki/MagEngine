/*********************************************************************
 * \file   ParticleSetup.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#include "ParticleSetup.h"

///=============================================================================
///						初期化
void ParticleSetup::Initialize(DirectXCore *dxCore, SrvSetup *srvSetup) {
	/// ===引数でdxManagerを受取=== ///
	dxCore_ = dxCore;

	//========================================
	// SrvSetupの取得
	srvSetup_ = srvSetup;

	/// ===グラフィックスパイプラインの生成=== ///
	CreateGraphicsPipeline();
}

///=============================================================================
///						共通化処理
void ParticleSetup::CommonDrawSetup() {
	// コマンドリストの取得
	//  NOTE:Getを複数回呼び出すのは非効率的なので、変数に保持しておく
	auto commandList = dxCore_->GetCommandList();
	// ルートシグネイチャのセット
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	// グラフィックスパイプラインステートをセット
	commandList->SetPipelineState(graphicsPipelineState_.Get());
	// プリミティブトポロジーをセットする
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

///=============================================================================
///						ルートシグネチャーの作成
void ParticleSetup::CreateRootSignature() {
	/// ===RootSignature作成=== ///
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/// ===DescriptorRangeの設定=== ///
	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0; // から始まる
	descriptorRangeForInstancing[0].NumDescriptors = 1;		//
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	/// ===RootParameter作成=== ///
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0; // b0

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

	/// ===DescropterTable=== ///
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

	/// ====DirectionalLight=== ///
	// rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	   //CBV
	// rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;    //PixelShader
	// rootParameters[3].Descriptor.ShaderRegister = 1; // b1

	descriptionRootSignature.pParameters = rootParameters;			   // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	/// ===Samplerの設定=== ///
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;			   // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // 配列の長さ

	/// ===シリアライズしてバイナリにする=== ///
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
											 D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		throw std::runtime_error(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
	}

	/// ===バイナリを元に生成=== ///
	hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
												   signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	if (FAILED(hr)) {
		throw std::runtime_error("ENGINE MESSAGE: Particle Failed to create root signature");
	}
	Log("Particle Root signature created successfully :)");
}

///=============================================================================
///						グラフィックスパイプラインの作成
void ParticleSetup::CreateGraphicsPipeline() {
	//========================================
	// RoorSignatureの作成
	CreateRootSignature();

	//========================================
	// InputLayoutの設定を行う
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	// 頂点データ
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	// 画像座標データ
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	// 法線データ
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//========================================
	// BlendStateの設定を行う
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // アルファ値に基づいて加算
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;		// 背景色をそのまま使用（加算合成）
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//========================================
	// RasterizerStateの設定を行う
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//========================================
	// Shaderをcompileする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCore_->CompileShader(L"resources/shader/Particle.VS.hlsl", L"vs_6_0");
	if (!vertexShaderBlob) {
		throw std::runtime_error("ENGINE MESSAGE: Particle Failed to compile vertex shader :(");
	}
	Log("Particle Vertex shader created successfully :)", LogLevel::Success);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCore_->CompileShader(L"resources/shader/Particle.PS.hlsl", L"ps_6_0");
	if (!pixelShaderBlob) {
		throw std::runtime_error("ENGINE MESSAGE: Particle Failed to compile pixel shader :(");
	}
	Log("Particle Pixel shader state created successfully :)", LogLevel::Success);

	//========================================
	// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//========================================
	// DepthStencilStateの設定を行う
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//========================================
	// 実際に生成
	HRESULT hr = dxCore_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
																   IID_PPV_ARGS(&graphicsPipelineState_));
	if (FAILED(hr)) {
		throw std::runtime_error("Particle Failed to create graphics pipeline state :(");
	}
	Log("Particle Graphics pipeline state created successfully :)", LogLevel::Success);
}

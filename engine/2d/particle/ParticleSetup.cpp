/*********************************************************************
 * \file   ParticleSetup.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#include "ParticleSetup.h"
#include "engine/render/PipelineBuilder.h"
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						初期化
	void ParticleSetup::Initialize(DirectXCore *dxCore, SrvSetup *srvSetup, TextureManager &textureManager) {
		//========================================
		// 引数でdxManagerを受取
		dxCore_ = dxCore;
		//========================================
		// SrvSetupの取得
		srvSetup_ = srvSetup;
		// テクスチャ管理器の所有権はFrameworkに固定し、Particle側は参照だけを保持する
		textureManager_ = &textureManager;
		//========================================
		// グラフィックスパイプラインの生成
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
		if(FAILED(hr)) {
			throw std::runtime_error(reinterpret_cast<char *>( errorBlob->GetBufferPointer() ));
		}

		/// ===バイナリを元に生成=== ///
		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
			signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		if(FAILED(hr)) {
			throw std::runtime_error("ENGINE MESSAGE: Particle Failed to create root signature");
		}
		Logger::Log("Particle Root signature created successfully :)");
	}

	///=============================================================================
	///						グラフィックスパイプラインの作成
	PipelineRecipe ParticleSetup::CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Particle.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Particle.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].BlendEnable = TRUE;
		recipe.blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		recipe.blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		recipe.blendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		recipe.blendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe ParticleSetup::CreateRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get());
	}

	void ParticleSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: Particle固有の加算BlendとDepth Write無効設定はRecipe側に閉じ込める。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		graphicsPipelineState_ = builder.CreateGraphicsPipeline(CreateRecipe());
		Logger::Log("Particle Graphics pipeline state created successfully :)", Logger::LogLevel::Success);
	}
}

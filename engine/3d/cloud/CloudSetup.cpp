/*********************************************************************
 * \file   CloudSetup.cpp
 * \brief  雲描画のセットアップクラス実装
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#include "CloudSetup.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "engine/render/PipelineBuilder.h"
#include <stdexcept>
using namespace Logger;
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						初期化
	void CloudSetup::Initialize(DirectXCore *dxCore) {
		/// ===引数でdxCoreを受取=== ///
		dxCore_ = dxCore;

		/// ===グラフィックスパイプラインの生成=== ///
		CreateGraphicsPipeline();
	}

	///=============================================================================
	///						 共通描画設定
	void CloudSetup::CommonDrawSetup() {
		// COMMENT: コマンドリストを複数回取得するのは非効率的（フレーム毎に1回キャッシュすること）
		auto commandList = dxCore_->GetCommandList();
		// ルートシグネイチャのセット
		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		// COMMENT: パイプラインステート（PSO）キャッシング - 複数フレーム間で再利用可能
		commandList->SetPipelineState(pipelineState_.Get());
		// プリミティブトポロジーをセット（三角形リスト）
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	///=============================================================================
	///						 ルートシグネイチャーの作成
	void CloudSetup::CreateRootSignature() {
		//========================================
		// DescriptorRangeの設定（テクスチャ用）
		D3D12_DESCRIPTOR_RANGE descriptorRange{};
		// ウェザーマップテクスチャ用
		descriptorRange.BaseShaderRegister = 0;
		descriptorRange.NumDescriptors = 1;
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//========================================
		// RootParameterの設定（7つのパラメータ）
		D3D12_ROOT_PARAMETER rootParameters[7] = {};
		// カメラ定数バッファ（b0）
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// 雲レンダリングパラメータ定数バッファ（b1）
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[1].Descriptor.ShaderRegister = 1;

		// 弾痕バッファ定数バッファ（b2）
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].Descriptor.ShaderRegister = 2;

		// ウェザーマップテクスチャ（t0）
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRange;
		rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

		// 並行光源定数バッファ（b3）
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4].Descriptor.ShaderRegister = 3;

		// ポイントライト定数バッファ（b4）
		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[5].Descriptor.ShaderRegister = 4;

		// スポットライト定数バッファ（b5）
		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[6].Descriptor.ShaderRegister = 5;

		//========================================
		// StaticSamplerの設定
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

		//========================================
		// RootSignatureDescの設定
		D3D12_ROOT_SIGNATURE_DESC desc{};
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		desc.pParameters = rootParameters;
		desc.NumParameters = _countof(rootParameters);
		desc.pStaticSamplers = &staticSampler;
		desc.NumStaticSamplers = 1;

		//========================================
		// シリアライズしてルートシグネチャを作成
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			throw std::runtime_error(errorBlob ? reinterpret_cast<const char *>(errorBlob->GetBufferPointer())
											   : "CloudSetup root signature serialization failed.");
		}

		//========================================
		// ルートシグネチャの実際の生成
		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
													   IID_PPV_ARGS(&rootSignature_));
		if (FAILED(hr)) {
			throw std::runtime_error("CloudSetup root signature creation failed.");
		}
		Log("Cloud root signature created.", LogLevel::Success);
	}

	///=============================================================================
	///						 グラフィックスパイプラインの作成
	PipelineRecipe CloudSetup::CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Cloud.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Cloud.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.AlphaToCoverageEnable = FALSE;
		recipe.blendState.IndependentBlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].BlendEnable = TRUE;
		recipe.blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		recipe.blendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe CloudSetup::CreatePipelineRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get());
	}

	void CloudSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: Noise/SDF/Descriptor処理は描画データ側に残し、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		pipelineState_ = builder.CreateGraphicsPipeline(CreatePipelineRecipe());
		Log("Cloud graphics pipeline created.", LogLevel::Success);
	}
}

/*********************************************************************
 * \file   TrailEffectSetup.cpp
 * \brief  トレイルエフェクトセットアップクラス実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#include "TrailEffectSetup.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "SrvSetup.h"
#include "engine/render/PipelineBuilder.h"
#include <stdexcept>

using namespace Logger;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void TrailEffectSetup::Initialize(DirectXCore *dxCore) {
		//========================================
		// 引数でdxCoreを受取
		dxCore_ = dxCore;

		//========================================
		// グラフィックスパイプラインの生成
		CreateGraphicsPipeline();

		Log("TrailEffectSetup initialized.", LogLevel::Success);
	}

	///=============================================================================
	///						 共通描画設定
	void TrailEffectSetup::CommonDrawSetup() {
		// コマンドリストの取得
		auto commandList = dxCore_->GetCommandList();

		// ルートシグネイチャのセット
		commandList->SetGraphicsRootSignature(rootSignature_.Get());

		// グラフィックスパイプラインステートをセット
		commandList->SetPipelineState(pipelineState_.Get());

		// プリミティブトポロジーをセット（三角形リスト）
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//========================================
		// テクスチャはColorのグラデーションで表現される
		// ポイントのageに応じてStartColorからEndColorへ補間される
	}

	///=============================================================================
	///						 ルートシグネイチャーの作成
	void TrailEffectSetup::CreateRootSignature() {
		//========================================
		// RootParameterの設定（2つのパラメータ）
		D3D12_ROOT_PARAMETER rootParameters[2] = {};

		// パラメータ定数バッファ（b0）
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// カメラ定数バッファ（b1）
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 1;

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
			throw std::runtime_error(
				errorBlob ? reinterpret_cast<const char *>(errorBlob->GetBufferPointer())
						  : "TrailEffectSetup root signature serialization failed.");
		}

		//========================================
		// ルートシグネチャの実際の生成
		hr = dxCore_->GetDevice()->CreateRootSignature(
			0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_));
		if (FAILED(hr)) {
			throw std::runtime_error("TrailEffectSetup root signature creation failed.");
		}

		Log("Trail root signature created.", LogLevel::Success);
	}

	///=============================================================================
	///						 グラフィックスパイプラインの作成
	PipelineRecipe TrailEffectSetup::CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Trail.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Trail.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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

	PipelineRecipe TrailEffectSetup::CreatePipelineRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get());
	}

	void TrailEffectSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: Trail頂点生成やBuffer更新は描画データ側に残し、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		pipelineState_ = builder.CreateGraphicsPipeline(CreatePipelineRecipe());
		Log("Trail graphics pipeline created.", LogLevel::Success);
	}
}

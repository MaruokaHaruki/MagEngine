/*********************************************************************
 * \file   SkyboxSetup.cpp
 * \brief  スカイボックス共通設定クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "SkyboxSetup.h"
#include "Logger.h"
#include "engine/render/pipeline/PipelineBuilder.h"
using namespace Logger;
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void SkyboxSetup::Initialize(DirectXCore *dxCore, TextureManager &textureManager) {
		/// ===引数でdxManagerを受取=== ///
		dxCore_ = dxCore;
		// テクスチャ管理器はFramework側の所有物として扱う
		textureManager_ = &textureManager;

		/// ===グラフィックスパイプラインの生成=== ///
		CreateGraphicsPipeline();
	}

	///=============================================================================
	///						 共通描画設定
	void SkyboxSetup::CommonDrawSetup() {
		// コマンドリストの取得
		auto commandList = dxCore_->GetCommandList();
		// ルートシグネイチャのセット
		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		// グラフィックスパイプラインステートをセット
		commandList->SetPipelineState(graphicsPipelineState_.Get());
		// プリミティブトポロジーをセットする
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	///=============================================================================
	///						 ルートシグネチャーの作成
	void SkyboxSetup::CreateRootSignature() {
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// スカイボックス用のルートパラメータ（変換行列、ライト、テクスチャ）
		D3D12_ROOT_PARAMETER rootParameters[5] = {};
		// 変換行列用（b0）
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// テクスチャ用（キューブマップ、t0）
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		// 並行光源定数バッファ（b1）
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].Descriptor.ShaderRegister = 1;

		// ポイントライト定数バッファ（b2）
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 2;

		// スポットライト定数バッファ（b3）
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4].Descriptor.ShaderRegister = 3;

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = _countof(rootParameters);

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0;
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			throw std::runtime_error(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
		}

		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create skybox root signature");
		}
		Log("Skybox Root signature created successfully :)", LogLevel::Success);
	}

	///=============================================================================
	///						 グラフィックスパイプラインの作成
	PipelineRecipe SkyboxSetup::CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Skybox.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Skybox.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.AlphaToCoverageEnable = FALSE;
		recipe.blendState.IndependentBlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].BlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].LogicOpEnable = FALSE;
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

	PipelineRecipe SkyboxSetup::CreatePipelineRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get());
	}

	void SkyboxSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: Cubemap用Descriptor設定は変更せず、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		graphicsPipelineState_ = builder.CreateGraphicsPipeline(CreatePipelineRecipe());
		Log("Skybox Graphics pipeline state created successfully :)", LogLevel::Success);
	}
}

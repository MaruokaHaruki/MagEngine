/*********************************************************************
 * \file   Object3dSetup.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "Object3dSetup.h"
#include "Logger.h"
#include "engine/render/PipelineBuilder.h"
using namespace Logger;
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						初期化
	void Object3dSetup::Initialize(DirectXCore *dxCore) {
		/// ===引数でdxManagerを受取=== ///
		dxCore_ = dxCore;

		/// ===グラフィックスパイプラインの生成=== ///
		CreateGraphicsPipeline();
	}

	///=============================================================================
	///						 共通描画設定
	void Object3dSetup::CommonDrawSetup() {
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
	///						 ルートシグネイチャーの作成
	void Object3dSetup::CreateRootSignature() {
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
		// 通常テクスチャ用
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// 環境マップテクスチャ用
		descriptorRange[1].BaseShaderRegister = 1;
		descriptorRange[1].NumDescriptors = 1;
		descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// ルートパラメータを8つに変更（環境マップ用に1つ追加）
		D3D12_ROOT_PARAMETER rootParameters[8] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		// 通常テクスチャ
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;

		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4].Descriptor.ShaderRegister = 2;

		// ポイントライト用のパラメータ
		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[5].Descriptor.ShaderRegister = 3;

		// スポットライト用のパラメータ
		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[6].Descriptor.ShaderRegister = 4;

		// 環境マップテクスチャ用のパラメータ
		rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[7].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
		rootParameters[7].DescriptorTable.NumDescriptorRanges = 1;

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
		if(FAILED(hr)) {
			throw std::runtime_error(reinterpret_cast<char *>( errorBlob->GetBufferPointer() ));
		}

		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		if(FAILED(hr)) {
			throw std::runtime_error("Failed to create root signature");
		}
		Log("Object3d Root signature created successfully :), LogLevel::SUCCESS");
	}

	///=============================================================================
	///						 グラフィックスパイプラインの作成
	PipelineRecipe Object3dSetup::CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Object3d.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Object3d.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.AlphaToCoverageEnable = FALSE;
		recipe.blendState.IndependentBlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].BlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].LogicOpEnable = FALSE;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe Object3dSetup::CreatePipelineRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get());
	}

	void Object3dSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: Object3D固有設定はRecipeに残し、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		graphicsPipelineState_ = builder.CreateGraphicsPipeline(CreatePipelineRecipe());
		Log("Object3d Graphics pipeline state created successfully :)", LogLevel::Success);
	}
}

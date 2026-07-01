/*********************************************************************
 * \file   GrayscaleEffect.h
 * \brief  グレースケールエフェクトクラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#include "GrayscaleEffect.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "TextureManager.h"
#include "engine/render/PipelineBuilder.h"
using namespace Logger;
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///                        初期化
	void GrayscaleEffect::Initialize(DirectXCore *dxCore) {
		//========================================
		// シェーダーパスの設定
		vertexShaderPath_ = L"resources/shader/FullScreen.VS.hlsl";
		pixelShaderPath_ = L"resources/shader/Grayscale.PS.hlsl";
		//=======================================
		// DirectXCoreのポインタを取得
		dxCore_ = dxCore;
		// パイプラインの作成
		CreatePipeline();
		// ルートシグネチャの作成
		CreateRootSignature();
	}

	///=============================================================================
	///                        描画
	void GrayscaleEffect::PreDraw() {
		//========================================
		// 以下の内容を繰り返す
		dxCore_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
		dxCore_->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());
	}

	///=============================================================================
	///                        描画後処理
	void GrayscaleEffect::PostDraw() {
	}

	///=============================================================================
	///                        パイプラインの作成
	PipelineRecipe GrayscaleEffect::CreateDefaultRecipe(ID3D12RootSignature *rootSignature, const std::wstring &vertexShaderPath, const std::wstring &pixelShaderPath) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {vertexShaderPath, L"main", L"vs_6_0"};
		recipe.pixelShader = {pixelShaderPath, L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		// NOTE: Fullscreen描画は頂点IDで生成する既存Shader仕様のため、InputLayoutは空のまま維持する。
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = false;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe GrayscaleEffect::CreatePipelineRecipe() const {
		return CreateDefaultRecipe(rootSignature_.Get(), vertexShaderPath_, pixelShaderPath_);
	}

	void GrayscaleEffect::CreatePipeline() {
		CreateRootSignature();
		// NOTE: Descriptor設定とDraw順はPostEffectManager側に残し、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		graphicsPipelineState_ = builder.CreateGraphicsPipeline(CreatePipelineRecipe());
	}
	///=============================================================================
	///                        ルートシグネチャの作成
	void GrayscaleEffect::CreateRootSignature() {
		HRESULT hr;
		//========================================
		// ルートシグネチャの設定
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//========================================
		// SRV の Descriptor Range
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;													 // t0: Shader Register
		descriptorRange[0].NumDescriptors = 1;														 // 1つのSRV
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;								 // SRV
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 自動計算

		//========================================
		// Root Parameter: SRV (gTexture)
		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用
																			// rootParameters[0].DescriptorTable.NumDescriptorRanges = 1; // 1つの範囲
																			// rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange; // SRV の範囲

		rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;			   // Tableの中身の配列を指定
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

		//========================================
		// Static Sampler
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;						// 全MipMap使用
		staticSamplers[0].ShaderRegister = 0;								// s0: Shader Register
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用

		//========================================
		// ルートシグネチャの構築
		descriptionRootSignature.pParameters = rootParameters;				   // ルートパラメーター配列へのポインタ
		descriptionRootSignature.NumParameters = _countof(rootParameters);	   // 配列の長さ
		descriptionRootSignature.pStaticSamplers = staticSamplers;			   // サンプラー配列へのポインタ
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // サンプラーの数

		//========================================
		// シリアライズしてバイナリにする
		// レンダーテクスチャのシグネチャ
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_ = nullptr;
		// レンダーテクスチャのエラーログ
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);
		if(FAILED(hr)) {
			Logger::Log(reinterpret_cast<char *>( errorBlob_->GetBufferPointer() ));
			assert(false);
		}

		//========================================
		// バイナリを元に生成
		rootSignature_ = nullptr;
		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));
	}
}

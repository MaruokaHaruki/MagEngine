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
		// コマンドリストの取得
		// NOTE:Getを複数回呼び出すのは非効率的なので、変数に保持しておく
		auto commandList = dxCore_->GetCommandList();
		// ルートシグネイチャのセット
		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		// グラフィックスパイプラインステートをセット
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
		// RootParameterの設定（4つのパラメータ）
		D3D12_ROOT_PARAMETER rootParameters[4] = {};
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
	void CloudSetup::CreateGraphicsPipeline() {
		//========================================
		// RoorSignatureの作成
		CreateRootSignature();

		//========================================
		// InputLayoutの設定を行う
		D3D12_INPUT_ELEMENT_DESC elements[2] = {};
		// 頂点データ（位置）
		elements[0].SemanticName = "POSITION";
		elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		elements[0].AlignedByteOffset = 0;

		// テクスチャ座標データ
		elements[1].SemanticName = "TEXCOORD";
		elements[1].SemanticIndex = 0;
		elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayout{};
		inputLayout.pInputElementDescs = elements;
		inputLayout.NumElements = _countof(elements);

		//========================================
		// Shaderをcompileする
		Microsoft::WRL::ComPtr<IDxcBlob> vs = dxCore_->CompileShader(L"resources/shader/Cloud.VS.hlsl", L"vs_6_0");
		if (!vs) {
			throw std::runtime_error("Cloud vertex shader compile failed.");
		}
		Log("Cloud Vertex shader created successfully :)", LogLevel::Success);

		Microsoft::WRL::ComPtr<IDxcBlob> ps = dxCore_->CompileShader(L"resources/shader/Cloud.PS.hlsl", L"ps_6_0");
		if (!ps) {
			throw std::runtime_error("Cloud pixel shader compile failed.");
		}
		Log("Cloud Pixel shader created successfully :)", LogLevel::Success);

		//========================================
		// BlendStateの設定を行う（αブレンディング）
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

		//========================================
		// RasterizerStateの設定を行う
		D3D12_RASTERIZER_DESC raster{};
		raster.CullMode = D3D12_CULL_MODE_NONE; // 両面描画
		raster.FillMode = D3D12_FILL_MODE_SOLID;

		//========================================
		// PSOを生成する
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

		//========================================
		// DepthStencilStateの設定を行う
		D3D12_DEPTH_STENCIL_DESC depth{};
		depth.DepthEnable = TRUE;
		depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// 深度書き込みを有効に
		depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Object3dと同じ比較関数
		desc.DepthStencilState = depth;
		desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//========================================
		// 実際に生成
		HRESULT hr = dxCore_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
		if (FAILED(hr)) {
			throw std::runtime_error("Cloud graphics pipeline creation failed.");
		}
		Log("Cloud graphics pipeline created.", LogLevel::Success);
	}
}
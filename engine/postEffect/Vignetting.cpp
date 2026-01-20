
/*********************************************************************
 * \file   Vignetting.h
 * \brief  ビネットエフェクトクラス
 *
 * \author Harukichimaru
 * \date   January 2026
 *********************************************************************/
#include "Vignetting.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "TextureManager.h"
using namespace Logger;
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///                        初期化
	void Vignetting::Initialize(DirectXCore *dxCore) {
		//========================================
		// シェーダーパスの設定
		vertexShaderPath_ = L"resources/shader/FullScreen.VS.hlsl";
		pixelShaderPath_ = L"resources/shader/Vignetting.hlsl";
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
	void Vignetting::PreDraw() {
		//========================================
		// 以下の内容を繰り返す
		dxCore_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
		dxCore_->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());
	}

	///=============================================================================
	///                        描画後処理
	void Vignetting::PostDraw() {
	}

	///=============================================================================
	///                        パイプラインの作成
	void Vignetting::CreatePipeline() {
		CreateRootSignature();

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].SemanticIndex = 0;
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = nullptr;
		inputLayoutDesc.NumElements = 0;

		// BlendStateの設定
		D3D12_BLEND_DESC blendDesc{};

		// すべての要素数を書き込む
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// RasiterzerStateの設定
		D3D12_RASTERIZER_DESC rasterizerDesc{};

		// 裏面(時計回り)を表示しない
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

		// 三角形の中を塗りつぶす
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		// Shaderをコンパイルする
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCore_->CompileShader(vertexShaderPath_, L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCore_->CompileShader(pixelShaderPath_, L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		// DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

		// Depthの機能を有効化する
		depthStencilDesc.DepthEnable = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
		graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();										  // RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;												  // InputLayout
		graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // VertexShader
		graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};	  // PixelShader
		graphicsPipelineStateDesc.BlendState = blendDesc;														  // BlendState
		graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;												  // RasterizerState

		// 書き込むRTVの情報
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		// 利用するトポロジ(形状)のタイプ.。三角形
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// どのように画面に色をつけるか
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// DepthStencilの設定
		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 実際に生成
		graphicsPipelineState_ = nullptr;
		dxCore_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
	}
	///=============================================================================
	///                        ルートシグネチャの作成
	void Vignetting::CreateRootSignature() {
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
		if (FAILED(hr)) {
			Logger::Log(reinterpret_cast<char *>(errorBlob_->GetBufferPointer()));
			assert(false);
		}

		//========================================
		// バイナリを元に生成
		rootSignature_ = nullptr;
		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));
	}
}

/*********************************************************************
 * \file   SpriteSetup.cpp
 * \brief  スプライト管理クラス実装 - スプライト描画システムの共通設定
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note   スプライト描画に必要なグラフィックスパイプラインを構築
 *********************************************************************/
#include "SpriteSetup.h"
#include "Logger.h"

using namespace Logger;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///                        初期化
	///=============================================================================

	///--------------------------------------------------------------
	///                         初期化
	void SpriteSetup::Initialize(DirectXCore *dxCore) {
		// DirectXCoreを記録
		dxCore_ = dxCore;

		// グラフィックスパイプラインの生成
		CreateGraphicsPipeline();
	}

	///=============================================================================
	///                        描画設定
	///=============================================================================

	///--------------------------------------------------------------
	///                         共通描画設定
	void SpriteSetup::CommonDrawSetup() {
		// コマンドリストを取得（変数にキャッシュして効率化）
		auto commandList = dxCore_->GetCommandList();

		// ルートシグネチャを設定
		commandList->SetGraphicsRootSignature(rootSignature_.Get());

		// グラフィックスパイプラインステートを設定
		commandList->SetPipelineState(graphicsPipelineState_.Get());

		// プリミティブトポロジーを設定（三角形リスト）
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	///=============================================================================
	///                        内部処理
	///=============================================================================

	///--------------------------------------------------------------
	///                    ルートシグネチャの作成
	void SpriteSetup::CreateRootSignature() {
		//---------------------------------------
		// RootSignatureの記述子を作成
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//---------------------------------------
		// DescriptorRange（テクスチャ用）の設定
		D3D12_DESCRIPTOR_RANGE descriptorRange{};
		descriptorRange.BaseShaderRegister = 0;	  // t0から始まる
		descriptorRange.NumDescriptors = 1;		  // 1つのテクスチャ
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	  // SRV（ShaderResourceView）
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//---------------------------------------
		// RootParameter（4つ）の設定
		D3D12_ROOT_PARAMETER rootParameters[4]{};

		// b0: Material（マテリアル定数バッファ）
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0;	  // b0

		// b0: TransformationMatrix（変換行列定数バッファ）
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 0;	  // b0

		// t0: Texture（テクスチャ）
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

		// b1: DirectionalLight（ライト定数バッファ - 将来の拡張用）
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;	  // b1

		descriptionRootSignature.pParameters = rootParameters;
		descriptionRootSignature.NumParameters = 4;

		//---------------------------------------
		// Sampler（サンプラー）の設定
		D3D12_STATIC_SAMPLER_DESC staticSamplers{};
		staticSamplers.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	  // リニアフィルタリング
		staticSamplers.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	  // U方向ラップ
		staticSamplers.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	  // V方向ラップ
		staticSamplers.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;	  // W方向ラップ
		staticSamplers.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers.MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers.ShaderRegister = 0;	  // s0
		staticSamplers.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		descriptionRootSignature.pStaticSamplers = &staticSamplers;
		descriptionRootSignature.NumStaticSamplers = 1;

		//---------------------------------------
		// シリアライズしてバイナリに変換
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

		HRESULT hr = D3D12SerializeRootSignature(
			&descriptionRootSignature,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&signatureBlob,
			&errorBlob);

		if (FAILED(hr)) {
			throw std::runtime_error(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
		}

		//---------------------------------------
		// バイナリを元にルートシグネチャを生成
		hr = dxCore_->GetDevice()->CreateRootSignature(
			0,
			signatureBlob->GetBufferPointer(),
			signatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_));

		if (FAILED(hr)) {
			throw std::runtime_error("ENGINE MESSAGE: Sprite failed to create root signature");
		}

		Log("Sprite root signature created successfully", LogLevel::Success);
	}

	///--------------------------------------------------------------
	///                    グラフィックスパイプラインの作成
	void SpriteSetup::CreateGraphicsPipeline() {
		//---------------------------------------
		// ルートシグネチャの作成
		CreateRootSignature();

		//---------------------------------------
		// InputLayout（入力レイアウト）の設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3]{};

		// POSITION: 頂点座標
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		// TEXCOORD: テクスチャ座標
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].SemanticIndex = 0;
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		// NORMAL: 法線
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].SemanticIndex = 0;
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = 3;

		//---------------------------------------
		// BlendState（ブレンドステート）の設定
		D3D12_BLEND_DESC blendDesc{};

		// アルファブレンディングを有効化（半透明描画対応）
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;			  // ソース: アルファ値
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;				  // 加算合成
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;	  // デスティネーション: 1 - アルファ
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		//---------------------------------------
		// RasterizerState（ラスタライザーステート）の設定
		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;	  // 背面カリング
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;  // ソリッド描画

		//---------------------------------------
		// Shaderのコンパイル
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
			dxCore_->CompileShader(L"resources/shader/Sprite.VS.hlsl", L"vs_6_0");

		if (!vertexShaderBlob) {
			throw std::runtime_error("Sprite: Failed to compile vertex shader");
		}
		Log("Sprite vertex shader compiled successfully", LogLevel::Success);

		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
			dxCore_->CompileShader(L"resources/shader/Sprite.PS.hlsl", L"ps_6_0");

		if (!pixelShaderBlob) {
			throw std::runtime_error("Sprite: Failed to compile pixel shader");
		}
		Log("Sprite pixel shader compiled successfully", LogLevel::Success);

		//---------------------------------------
		// PSO（Pipeline State Object）の生成
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

		//---------------------------------------
		// DepthStencilState（深度ステンシルステート）の設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;	  // 深度テスト有効
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	  // 深度書き込み有効
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;	  // 手前のものを描画

		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//---------------------------------------
		// グラフィックスパイプラインステートの実際の生成
		HRESULT hr = dxCore_->GetDevice()->CreateGraphicsPipelineState(
			&graphicsPipelineStateDesc,
			IID_PPV_ARGS(&graphicsPipelineState_));

		if (FAILED(hr)) {
			throw std::runtime_error("ENGINE MESSAGE: Sprite failed to create graphics pipeline state");
		}

		Log("Sprite graphics pipeline state created successfully", LogLevel::Success);
	}
}

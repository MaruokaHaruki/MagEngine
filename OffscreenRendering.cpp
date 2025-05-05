/*********************************************************************
 * \file   OffscreenRendering.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#include "OffscreenRendering.h"

///=============================================================================
///						初期化
void OffscreenRendering::Initialize(DirectXCore *dxCore, SrvSetup *srvSetup) {
	//========================================
	// NULL検出
	assert(dxCore);
	assert(srvSetup);
	//========================================
	// メンバ変数に記録
	this->dxCore_ = dxCore;
	this->srvSetup_ = srvSetup;
	//========================================
	// ルートシグネチャーの作成
	CreateRootSignature();
	//========================================
	// グラフィックスパイプラインの作成
	CreateGraphicsPipeline();
}

///=============================================================================
///						共通化処理
void OffscreenRendering::CommonDrawSetup(uint32_t renderTargetIndex, uint32_t renderResourceIndex) {
    // コマンドリストの取得
    auto commandList = dxCore_->GetCommandList();

    // 描画先のRTVのインデックス
    uint32_t renderTextureIndex = 2 + renderTargetIndex;

    // 描画先のRTVを設定する
    commandList->OMSetRenderTargets(1, &*dxCore_->GetRTVHandle(renderTextureIndex), false, nullptr);

    // ルートシグネチャの設定	
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // パイプラインステートの設定
    commandList->SetPipelineState(graphicsPipelineState_.Get());

    // renderTextureのSrvHandleを取得
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = srvSetup_->GetSRVGPUDescriptorHandle(renderResourceIndex);

    // SRVを設定
    commandList->SetGraphicsRootDescriptorTable(1, srvHandle);

    // 描画
    commandList->DrawInstanced(3, 1, 0, 0);
}

///=============================================================================
///                        レンダーテクスチャをSwapChainに描画
void OffscreenRendering::DrawToSwapChain() {
   // 書き込む方のインデックス
   uint32_t targetIndex = dxCore_->GetRenderTargetIndex();
   // 読み込む方のインデックス
   uint32_t resourceIndex = dxCore_->GetRenderResourceIndex();

   // 今読み込む方を描画用に変換（先に状態を変更）
   D3D12_RESOURCE_BARRIER barrier1{};
   barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   barrier1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier1.Transition.pResource = dxCore_->GetRenderTextureResource(targetIndex).Get();
   barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
   barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
   barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

   dxCore_->GetCommandList()->ResourceBarrier(1, &barrier1);

   // トポロジー設定
   dxCore_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   // この時点でtargetIndexのリソースはRENDER_TARGET状態なので描画可能
   CommonDrawSetup(targetIndex, resourceIndex);

   // 描画が終わったので現在書き込んだ方を読み込み用に変換
   D3D12_RESOURCE_BARRIER barrier2{};
   barrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   barrier2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier2.Transition.pResource = dxCore_->GetRenderTextureResource(targetIndex).Get();
   barrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
   barrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
   barrier2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

   dxCore_->GetCommandList()->ResourceBarrier(1, &barrier2);

   // インデックスを交換して次のフレームに備える
   std::swap(targetIndex, resourceIndex);

   dxCore_->SetRenderTargetIndex(targetIndex);
   dxCore_->SetRenderResourceIndex(resourceIndex);
}

///=============================================================================
///						ルートシグネチャーの作成
void OffscreenRendering::CreateRootSignature() {
	// ディスクリプタレンジの設定 (追加)
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].RegisterSpace = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// rootParametersの配列サイズを2に変更し、2つ目のパラメータを追加
	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	// 定数バッファ（TransformationMatrix）の設定（b0、頂点シェーダーで使用）
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[0].Descriptor.ShaderRegister = 0; // b0
	rootParameters[0].Descriptor.RegisterSpace = 0;

	// ディスクリプタテーブル (テクスチャ)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;

	// 静的サンプラーの設定 (追加)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers); // 追加
	rootSignatureDesc.pStaticSamplers = staticSamplers; // 追加
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// シリアライズとルートシグネチャの作成
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if(FAILED(hr)) {
		throw std::runtime_error(reinterpret_cast<char *>( errorBlob->GetBufferPointer() ));
	}

	hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	if(FAILED(hr)) {
		throw std::runtime_error("Failed to create OffscreenRendering root signature");
	}
	Log("OffscreenRendering Root signature created successfully :)", LogLevel::Success);
}

///=============================================================================
///						グラフィックスパイプラインの作成
void OffscreenRendering::CreateGraphicsPipeline() {
	//========================================
	// RoorSignatureの作成
	CreateRootSignature();

	//========================================
	// InputElementの設定
	/*D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};*/
	// 位置データ
	//inputElementDescs[0].SemanticName = "POSITION";
	//inputElementDescs[0].SemanticIndex = 0;
	//inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputElementDescs[0].InputSlot = 0;
	//inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	//inputElementDescs[0].InstanceDataStepRate = 0;
	// カラーデータ
	//inputElementDescs[1].SemanticName = "COLOR";
	//inputElementDescs[1].SemanticIndex = 0;
	//inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputElementDescs[1].InputSlot = 0;
	//inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	//inputElementDescs[1].InstanceDataStepRate = 0;

	//========================================
	// InputLayoutの設定を行う
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	//========================================
	// BlendStateの設定を行う
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//========================================
	// RasterizerStateの設定を行う
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//========================================
	// VertexShaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCore_->CompileShader(L"resources/shader/CopyImage.VS.hlsl", L"vs_6_0");
	if(!vertexShaderBlob) {
		Log("OffscreenRendering Failed to compile vertex shader :(", LogLevel::Error);
		throw std::runtime_error("OffscreenRendering Failed to compile vertex shader :(");
	}
	Log("OffscreenRendering Vertex shader created successfully :)", LogLevel::Success);
	//========================================
	// PixelShaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCore_->CompileShader(L"resources/shader/CopyImage.PS.hlsl", L"ps_6_0");
	if(!pixelShaderBlob) {
		Log("OffscreenRendering Failed to compile pixel shader :(", LogLevel::Error);
		throw std::runtime_error("OffscreenRendering Failed to compile pixel shader :(");
	}
	Log("OffscreenRendering Pixel shader state created successfully :)", LogLevel::Success);

	//========================================
	// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
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
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//========================================
	// 実際に生成
	HRESULT hr = dxCore_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState_));
	if(FAILED(hr)) {
		Log("OffscreenRendering Failed to create graphics pipeline state :(", LogLevel::Error);
		throw std::runtime_error("OffscreenRendering Failed to create graphics pipeline state :(");
	}
	Log(" OffscreenRendering pipeline state created successfully :)", LogLevel::Success);
}

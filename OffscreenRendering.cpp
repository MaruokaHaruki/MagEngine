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
void OffscreenRendering::CommonDrawSetup() {
    // コマンドリストの取得
	auto commandList = dxCore_->GetCommandList();



    // パイプラインステートとルートシグネチャの設定
    commandList->SetPipelineState(graphicsPipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    // レンダーテクスチャのSRVをシェーダーにバインド
    uint32_t resourceIndex = dxCore_->GetRenderResourceIndex();
    commandList->SetGraphicsRootDescriptorTable(1, srvSetup_->GetSRVGPUDescriptorHandle(resourceIndex));

    // プリミティブトポロジーの設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 頂点バッファなしで描画（頂点シェーダーでVertexIDベースで生成）
    commandList->DrawInstanced(3, 1, 0, 0);

}

///=============================================================================
///                        レンダーテクスチャをSwapChainに描画
void OffscreenRendering::DrawToSwapChain() {
    // コマンドリストの取得
    auto commandList = dxCore_->GetCommandList();
    
    // 読み込むレンダーテクスチャのインデックス（現在のリソースインデックス）
    uint32_t resourceIndex = dxCore_->GetRenderResourceIndex();

    // SwapChainをレンダーターゲットとして設定するための準備
    // SwapChainのStateをRENDER_TARGETに変更
    D3D12_RESOURCE_BARRIER barrierToRenderTargzzet{};
    barrierToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToRenderTarget.Transition.pResource = dxCore_->GetSwapChainResource();
    barrierToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    commandList->ResourceBarrier(1, &barrierToRenderTarget);
    
    // SwapChainのRTVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCore_->GetSwapChainRTVHandle();
    
    // レンダーターゲットをSwapChainに設定
    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    
    // ビューポートとシザー矩形の設定
    D3D12_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(dxCore_->GetWindowWidth());
    viewport.Height = static_cast<float>(dxCore_->GetWindowHeight());
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    
    D3D12_RECT scissorRect{};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(dxCore_->GetWindowWidth());
    scissorRect.bottom = static_cast<LONG>(dxCore_->GetWindowHeight());
    
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    
    // グラフィックスパイプラインとルートシグネチャの設定
    commandList->SetPipelineState(graphicsPipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    
    // レンダーテクスチャをt0スロットにセット
    commandList->SetGraphicsRootDescriptorTable(1, srvSetup_->GetGPUDescriptorHandle(
        dxCore_->GetRenderTextureSRVIndex(resourceIndex)));
    
    // 頂点がシェーダー内部で生成されるので、DrawInstanced(3,1,0,0)を呼び出す
    commandList->DrawInstanced(3, 1, 0, 0);
    
    // SwapChainのStateをPRESENTに戻す
    D3D12_RESOURCE_BARRIER barrierToPresent{};
    barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToPresent.Transition.pResource = dxCore_->GetSwapChainResource();
    barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
    commandList->ResourceBarrier(1, &barrierToPresent);
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

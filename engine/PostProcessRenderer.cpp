#include "PostProcessRenderer.h"
#include "Logger.h"
#include <d3dcompiler.h>

// 頂点構造体
struct Vertex {
    float position[3];
    float texCoord[2];
};

// 静的メンバ変数の実体化
PostProcessRenderer* PostProcessRenderer::instance_ = nullptr;

PostProcessRenderer* PostProcessRenderer::GetInstance() {
    if (!instance_) {
        instance_ = new PostProcessRenderer();
    }
    return instance_;
}

void PostProcessRenderer::Initialize() {
    if (isInitialized_) {
        return;
    }
    
    // シェーダー作成
    CreateShaders();
    
    // ルートシグネチャ作成
    CreateRootSignature();
    
    // パイプライン作成
    CreateGraphicsPipeline();
    
    // 頂点バッファ作成
    CreateVertexBuffer();
    
    isInitialized_ = true;
    Log("PostProcessRenderer initialized successfully", LogLevel::Success);
}

void PostProcessRenderer::CreateShaders() {
    // 頂点シェーダーのコンパイル
    HRESULT result = D3DCompileFromFile(
        L"resources/shader/CopyImage.VS.hlsl", 
        nullptr, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &vsBlob_, nullptr);
    
    if (FAILED(result)) {
        Log("Failed to compile vertex shader for post-processing", LogLevel::Error);
        return;
    }
    
    // ピクセルシェーダーのコンパイル
    result = D3DCompileFromFile(
        L"resources/shader/CopyImage.PS.hlsl", 
        nullptr, 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0, &psBlob_, nullptr);
    
    if (FAILED(result)) {
        Log("Failed to compile pixel shader for post-processing", LogLevel::Error);
        return;
    }
}

void PostProcessRenderer::CreateRootSignature() {
    // デスクリプタレンジの設定
    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0; // t0
    descriptorRange.RegisterSpace = 0;
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    
    // ルートパラメータの設定
    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 1;
    rootParam.DescriptorTable.pDescriptorRanges = &descriptorRange;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    // スタティックサンプラーの設定
    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0; // s0
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
    // ルートシグネチャの設定
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = &rootParam;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    // シリアライズとルートシグネチャの作成
    Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT result = D3D12SerializeRootSignature(
        &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
        &rootSigBlob, &errorBlob);
    
    if (FAILED(result)) {
        Log("Failed to serialize root signature for post-processing", LogLevel::Error);
        return;
    }
    
    result = dxCore_->GetDevice()->CreateRootSignature(
        0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    
    if (FAILED(result)) {
        Log("Failed to create root signature for post-processing", LogLevel::Error);
        return;
    }
}

void PostProcessRenderer::CreateGraphicsPipeline() {
    // 頂点レイアウト
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[0].InstanceDataStepRate = 0;
    
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs[1].InstanceDataStepRate = 0;
    
    // パイプラインステートの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.pRootSignature = rootSignature_.Get();
    
    // シェーダーの設定
    pipelineDesc.VS.pShaderBytecode = vsBlob_->GetBufferPointer();
    pipelineDesc.VS.BytecodeLength = vsBlob_->GetBufferSize();
    pipelineDesc.PS.pShaderBytecode = psBlob_->GetBufferPointer();
    pipelineDesc.PS.BytecodeLength = psBlob_->GetBufferSize();
    
    // ブレンドステートの設定
    pipelineDesc.BlendState.AlphaToCoverageEnable = false;
    pipelineDesc.BlendState.IndependentBlendEnable = false;
    pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    
    // ラスタライザステートの設定
    pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 裏面カリングなし
    pipelineDesc.RasterizerState.FrontCounterClockwise = FALSE;
    pipelineDesc.RasterizerState.DepthBias = 0;
    pipelineDesc.RasterizerState.DepthBiasClamp = 0.0f;
    pipelineDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
    pipelineDesc.RasterizerState.DepthClipEnable = TRUE;
    pipelineDesc.RasterizerState.MultisampleEnable = FALSE;
    pipelineDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    pipelineDesc.RasterizerState.ForcedSampleCount = 0;
    pipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    
    // 深度ステンシルステートの設定 (無効化)
    pipelineDesc.DepthStencilState.DepthEnable = false;
    
    // 入力レイアウトの設定
    pipelineDesc.InputLayout.pInputElementDescs = inputElementDescs;
    pipelineDesc.InputLayout.NumElements = _countof(inputElementDescs);
    
    // その他の設定
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    pipelineDesc.SampleDesc.Count = 1;
    pipelineDesc.SampleDesc.Quality = 0;
    pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    
    // パイプラインステートの作成
    HRESULT result = dxCore_->GetDevice()->CreateGraphicsPipelineState(
        &pipelineDesc, IID_PPV_ARGS(&pipelineState_));
    
    if (FAILED(result)) {
        Log("Failed to create pipeline state for post-processing", LogLevel::Error);
        return;
    }
}

void PostProcessRenderer::CreateVertexBuffer() {
    // フルスクリーン三角形を使って画面を覆う (最適化)
    // 大きな三角形を使用して、スクリーン全体をカバー
    // この三角形は画面の外まではみ出るが、ピクセルシェーダーでクリッピングされる
    Vertex vertices[3] = {
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }, // 左下
        { {-1.0f,  3.0f, 0.0f}, {0.0f, -1.0f} }, // 左上（画面外）
        { { 3.0f, -1.0f, 0.0f}, {2.0f, 1.0f} }  // 右下（画面外）
    };
    
    // ヒープの設定
    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    
    // リソースの設定
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(vertices);
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    
    // バーテックスバッファの作成
    HRESULT result = dxCore_->GetDevice()->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_));
    
    if (FAILED(result)) {
        Log("Failed to create vertex buffer for post-processing", LogLevel::Error);
        return;
    }
    
    // マップしてデータをコピー
    Vertex* vertexMap = nullptr;
    result = vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
    if (SUCCEEDED(result)) {
        memcpy(vertexMap, vertices, sizeof(vertices));
        vertexBuffer_->Unmap(0, nullptr);
    }
    
    // バーテックスバッファビューの作成
    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(vertices);
    vertexBufferView_.StrideInBytes = sizeof(Vertex);
}

void PostProcessRenderer::Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle) {
	ID3D12GraphicsCommandList *commandList = dxCore_->GetCommandList().Get();
    
    // パイプラインステートとルートシグネチャをセット
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    
    // 頂点バッファをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // テクスチャをセット
    commandList->SetGraphicsRootDescriptorTable(0, textureSrvHandle);
    
    // 描画（3頂点で1つの三角形）
    commandList->DrawInstanced(3, 1, 0, 0);
}
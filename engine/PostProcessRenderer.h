#pragma once
#include "DirectXCore.h"
#include <d3d12.h>
#include <wrl/client.h>

class PostProcessRenderer {
public:
    // インスタンス取得
    static PostProcessRenderer* GetInstance();
    
    // 初期化
    void Initialize();
    
    // 描画
    void Draw(D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle);

    void SetDxCore(DirectXCore* dxCore) { dxCore_ = dxCore; }
private:
    PostProcessRenderer() = default;
    ~PostProcessRenderer() = default;
    PostProcessRenderer(const PostProcessRenderer&) = delete;
    PostProcessRenderer& operator=(const PostProcessRenderer&) = delete;

    void CreateShaders();
    void CreateRootSignature();
    void CreateGraphicsPipeline();
    void CreateVertexBuffer();

private:
    static PostProcessRenderer* instance_;
    DirectXCore* dxCore_ = nullptr;
    
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob_ = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
    
    // 最適化: 大きな三角形を使った描画用のバーテックスバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    
    bool isInitialized_ = false;
};
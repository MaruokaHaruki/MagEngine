#pragma once
#include "Vector4.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

/// @brief レンダーターゲット管理クラス
class RenderTargetManager {
public:
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

	// RTV作成
	void CreateRTV(
		uint32_t index,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		Microsoft::WRL::ComPtr<ID3D12Device> device);

	// DSV作成
	void CreateDSV(
		Microsoft::WRL::ComPtr<ID3D12Resource> depthResource,
		Microsoft::WRL::ComPtr<ID3D12Device> device);

	// レンダーテクスチャ作成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTexture(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		const Vector4 &clearColor);

	// ハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	uint32_t rtvDescriptorSize_ = 0;
	uint32_t dsvDescriptorSize_ = 0;
};
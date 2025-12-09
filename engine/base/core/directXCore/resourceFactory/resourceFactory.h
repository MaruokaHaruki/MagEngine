#pragma once
#include "DirectXTex.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

/// @brief リソース生成ユーティリティクラス（静的メソッドのみ）
class ResourceFactory {
public:
	// バッファリソース
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		size_t sizeInBytes);

	// テクスチャリソース
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		const DirectX::TexMetadata &metadata);

	// 深度ステンシルリソース
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencil(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		uint32_t width,
		uint32_t height);

	// ディスクリプタヒープ
	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t numDescriptors,
		bool shaderVisible);

	// テクスチャアップロード
	static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTexture(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> texture,
		const DirectX::ScratchImage &mipImages);
};
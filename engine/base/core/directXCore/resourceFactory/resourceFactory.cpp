#include "DirectXCore.h"
#include "PostEffectManager.h"
#include "TextureManager.h"
//========================================
// 標準ライブラリ
#include <vector>
//========================================
// DirectXTex
#include "d3dx12.h"
#pragma comment(lib, "winmm.lib")
// HOTFIX:リンクエラー対策
#include <fstream>
#include <iostream>
#include <string>

///=============================================================================
///						バッファリソースの生成
Microsoft::WRL::ComPtr<ID3D12Resource> ResourceFactory::CreateBuffer(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	size_t sizeInBytes) {

	// バッファリソースの設定を作成
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// アップロードヒープのプロパティを設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// リソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource));

	// エラーチェック
	if (FAILED(hr) || !resource) {
		Log("Failed to create buffer resource.", LogLevel::Error);
		return nullptr;
	}

	return resource;
}

///=============================================================================
///						テクスチャリソースの生成
Microsoft::WRL::ComPtr<ID3D12Resource> ResourceFactory::CreateTexture(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	const DirectX::TexMetadata &metadata) {

	// metadataを元にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// resourceを生成する
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource));

#ifdef _DEBUG
	assert(SUCCEEDED(hr));
#endif
	if (FAILED(hr)) {
		Log("Failed to create texture resource.", LogLevel::Error);
		return nullptr;
	}

	return resource;
}

///=============================================================================
///						深度ステンシルリソースの生成
Microsoft::WRL::ComPtr<ID3D12Resource> ResourceFactory::CreateDepthStencil(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	uint32_t width,
	uint32_t height) {

	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 設定を元にResourceの生成を行う
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource));

#ifdef _DEBUG
	assert(SUCCEEDED(hr));
#endif
	if (FAILED(hr)) {
		Log("Failed to create depth stencil resource.", LogLevel::Error);
		return nullptr;
	}

	return resource;
}

///=============================================================================
///						ディスクリプタヒープの生成
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> ResourceFactory::CreateDescriptorHeap(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	uint32_t numDescriptors,
	bool shaderVisible) {

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = type;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
#ifdef _DEBUG
	assert(SUCCEEDED(hr));
#endif
	if (FAILED(hr)) {
		Log("Failed to create descriptor heap.", LogLevel::Error);
		return nullptr;
	}

	Log("Descriptor heap created successfully.", LogLevel::Success);
	return descriptorHeap;
}

///=============================================================================
///						テクスチャアップロード
Microsoft::WRL::ComPtr<ID3D12Resource> ResourceFactory::UploadTexture(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> texture,
	const DirectX::ScratchImage &mipImages) {

	// 中間リソースの作成
	std::vector<D3D12_SUBRESOURCE_DATA> subresource;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresource);

	// Subresourceの数を元に、コピー元となるintermediateResourceに必要なサイズを計算する
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresource.size()));

	// 計算したサイズでintermediateResourceを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBuffer(device, intermediateSize);

	// データ転送をコマンドに積む
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresource.size()), subresource.data());

	// 読み込み変更コマンド
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

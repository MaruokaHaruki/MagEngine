#include "renderTargetManager.h"
#include "ResourceFactory.h"
#include <cassert>

///=============================================================================
///						初期化
void RenderTargetManager::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	// RTVヒープ作成（スワップチェーン2つ + レンダーテクスチャ2つ）
	rtvHeap_ = ResourceFactory::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, false);
	rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// DSVヒープ作成
	dsvHeap_ = ResourceFactory::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	dsvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

///=============================================================================
///						RTV作成
void RenderTargetManager::CreateRTV(
	uint32_t index,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	Microsoft::WRL::ComPtr<ID3D12Device> device) {

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = GetRTVHandle(index);
	device->CreateRenderTargetView(resource.Get(), &rtvDesc, handle);
}

///=============================================================================
///						DSV作成
void RenderTargetManager::CreateDSV(
	Microsoft::WRL::ComPtr<ID3D12Resource> depthResource,
	Microsoft::WRL::ComPtr<ID3D12Device> device) {

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = GetDSVHandle();
	device->CreateDepthStencilView(depthResource.Get(), &dsvDesc, handle);
}

///=============================================================================
///						レンダーテクスチャ作成
Microsoft::WRL::ComPtr<ID3D12Resource> RenderTargetManager::CreateRenderTexture(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const Vector4 &clearColor) {

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

///=============================================================================
///						RTVハンドル取得
D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetRTVHandle(uint32_t index) const {
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += (rtvDescriptorSize_ * index);
	return handle;
}

///=============================================================================
///						DSVハンドル取得
D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetManager::GetDSVHandle() const {
	return dsvHeap_->GetCPUDescriptorHandleForHeapStart();
}

#include "swapChainManager.h"
#include "WinApp.h"
#include <cassert>

///=============================================================================
///						初期化
void SwapChainManager::Initialize(
	Microsoft::WRL::ComPtr<IDXGIFactory7> factory,
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
	WinApp *winApp) {

	CreateSwapChain(factory, commandQueue, winApp);
	GetBackBuffers();
}

///=============================================================================
///						Present
void SwapChainManager::Present(bool vsync) {
	swapChain_->Present(vsync ? 1 : 0, 0);
}

///=============================================================================
///						現在のバックバッファインデックス取得
uint32_t SwapChainManager::GetCurrentBackBufferIndex() const {
	return swapChain_->GetCurrentBackBufferIndex();
}

///=============================================================================
///						バックバッファ取得
Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainManager::GetBackBuffer(uint32_t index) const {
	return backBuffers_[index];
}

///=============================================================================
///						スワップチェーンの生成
void SwapChainManager::CreateSwapChain(
	Microsoft::WRL::ComPtr<IDXGIFactory7> factory,
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
	WinApp *winApp) {

	swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = winApp->GetWindowWidth();
	swapChainDesc.Height = winApp->GetWindowHeight();
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	HRESULT hr = factory->CreateSwapChainForHwnd(
		commandQueue.Get(), winApp->GetWindowHandle(), &swapChainDesc,
		nullptr, nullptr, &swapChain1);
	assert(SUCCEEDED(hr));

	hr = swapChain1.As(&swapChain_);
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						バックバッファの取得
void SwapChainManager::GetBackBuffers() {
	HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffers_[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&backBuffers_[1]));
	assert(SUCCEEDED(hr));
}

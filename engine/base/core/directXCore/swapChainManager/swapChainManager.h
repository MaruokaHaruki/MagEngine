#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

class WinApp;

/// @brief スワップチェーン管理クラス
class SwapChainManager {
public:
	void Initialize(
		Microsoft::WRL::ComPtr<IDXGIFactory7> factory,
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		WinApp *winApp);

	void Present(bool vsync = true);
	uint32_t GetCurrentBackBufferIndex() const;
	Microsoft::WRL::ComPtr<ID3D12Resource> GetBackBuffer(uint32_t index) const;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> GetSwapChain() const {
		return swapChain_;
	}

private:
	void CreateSwapChain(
		Microsoft::WRL::ComPtr<IDXGIFactory7> factory,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		WinApp *winApp);
	void GetBackBuffers();

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers_[2];
};
#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

class WinApp;

/// @brief DirectX12デバイス初期化・管理クラス
class GraphicsDevice {
public:
	void Initialize(WinApp *winApp);
	void Finalize();

	// Getter
	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const {
		return device_;
	}
	Microsoft::WRL::ComPtr<IDXGIFactory7> GetFactory() const {
		return dxgiFactory_;
	}

private:
	void CreateDebugLayer();
	void CreateDxgiFactory();
	void SelectAdapter();
	void CreateD3D12Device();
	void SetupErrorHandling();

	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
#endif
};
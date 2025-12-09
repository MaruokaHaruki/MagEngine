#pragma once
//========================================
// 標準ライブラリ
#include <chrono>
#include <cstdint>
#include <format>
#include <string>
#include <thread>
#include <wrl.h>
//========================================
// 自作関数
#include "WstringUtility.h"
using namespace WstringUtility;
#include "Logger.h"
using namespace Logger;
#include "CommandContext.h"
#include "FullscreenPassRendere.h"
#include "GraphicsDevice.h"
#include "RenderTargetManager.h"
#include "ResourceFactory.h"
#include "ShaderCompiler.h"
#include "SwapChainManager.h"
#include "Vector4.h"
#include "WinApp.h"
//========================================
// ReportLiveObj
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
//========================================
// DX12 include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
// DXtec
#include "DirectXTex.h"
//========================================
// imgui
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
/// @brief コマンド実行管理クラス
class CommandContext {
public:
	void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);
	void Finalize();

	// コマンド操作
	void Begin();	   // コマンドリストのリセット
	void Close();	   // コマンドリストのクローズ
	void Execute();	   // コマンドキューに送信
	void WaitForGPU(); // GPU完了待機

	// Getter
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const {
		return commandList_;
	}
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const {
		return commandQueue_;
	}

private:
	void CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> device);
	void CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device);
	void CreateCommandList(Microsoft::WRL::ComPtr<ID3D12Device> device);
	void CreateFence(Microsoft::WRL::ComPtr<ID3D12Device> device);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;
};
#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <windows.h>

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
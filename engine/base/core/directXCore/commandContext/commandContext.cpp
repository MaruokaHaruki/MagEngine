#include "commandContext.h"
#include <cassert>

///=============================================================================
///						初期化
void CommandContext::Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	CreateCommandQueue(device);
	CreateCommandAllocator(device);
	CreateCommandList(device);
	CreateFence(device);
}

///=============================================================================
///						終了処理
void CommandContext::Finalize() {
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

///=============================================================================
///						コマンドリストのリセット
void CommandContext::Begin() {
	HRESULT hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						コマンドリストのクローズ
void CommandContext::Close() {
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						コマンドキューに送信
void CommandContext::Execute() {
	ID3D12CommandList *commandLists[] = {commandList_.Get()};
	commandQueue_->ExecuteCommandLists(1, commandLists);
}

///=============================================================================
///						GPU完了待機
void CommandContext::WaitForGPU() {
	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	if (fence_->GetCompletedValue() < fenceValue_) {
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}

///=============================================================================
///						コマンドキューの生成
void CommandContext::CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	commandQueue_ = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	HRESULT hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						コマンドアロケータの生成
void CommandContext::CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	commandAllocator_ = nullptr;
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						コマンドリストの生成
void CommandContext::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	commandList_ = nullptr;
	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
										   commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						フェンスの生成
void CommandContext::CreateFence(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	fence_ = nullptr;
	fenceValue_ = 0;
	HRESULT hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

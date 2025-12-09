#include "GraphicsDevice.h"
#include "Logger.h"
#include "WinApp.h"
#include "WstringUtility.h"
#include <cassert>
#include <format>

using namespace Logger;
using namespace WstringUtility;

///=============================================================================
///						初期化
void GraphicsDevice::Initialize(WinApp *winApp) {
	CreateDebugLayer();
	CreateDxgiFactory();
	SelectAdapter();
	CreateD3D12Device();
	SetupErrorHandling();
}

///=============================================================================
///						終了処理
void GraphicsDevice::Finalize() {
#ifdef _DEBUG
	if (debugController_) {
		debugController_.Reset();
	}
	if (infoQueue_) {
		infoQueue_.Reset();
	}
#endif
}

///=============================================================================
///						デバッグレイヤーの生成
void GraphicsDevice::CreateDebugLayer() {
#ifdef _DEBUG
	debugController_ = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		debugController_->EnableDebugLayer();
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}
#endif
}

///=============================================================================
///						DXGIファクトリーの生成
void GraphicsDevice::CreateDxgiFactory() {
	dxgiFactory_ = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));
}

///=============================================================================
///						アダプタの選択
void GraphicsDevice::SelectAdapter() {
	useAdapter_ = nullptr;
	DXGI_ADAPTER_DESC3 adapterDesc{};

	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
															  DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) !=
					 DXGI_ERROR_NOT_FOUND;
		 i++) {

		HRESULT hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			Log(ConvertString(std::format(L"Use Adapter;{}", adapterDesc.Description)), LogLevel::Info);
			break;
		}
		useAdapter_ = nullptr;
	}
	assert(useAdapter_ != nullptr);
}

///=============================================================================
///						D3D12デバイスの作成
void GraphicsDevice::CreateD3D12Device() {
	device_ = nullptr;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
	const char *featureLevelStrings[] = {"12.2", "12.1", "12.0"};

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		HRESULT hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr)) {
			Log(std::format("FeatureLevel : {}", featureLevelStrings[i]), LogLevel::Info);
			break;
		}
	}

	assert(device_ != nullptr);
	Log("Complete create D3D12Device!!!", LogLevel::Success);
}

///=============================================================================
///						エラーハンドリングの設定
void GraphicsDevice::SetupErrorHandling() {
#ifdef _DEBUG
	infoQueue_ = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE};
		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue_->PushStorageFilter(&filter);
	}
#endif
}

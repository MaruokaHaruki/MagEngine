/*********************************************************************
 * \file   DirectXCore.cpp
 * \brief  DirectX 12のコア機能を管理するクラスの実装
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note   デバイス、コマンドキューなどの初期化・管理処理を実装
 *********************************************************************/
#include "DirectXCore.h"
#include "engine/render/graph/RenderBarrierRecorder.h"
#include "engine/render/pipeline/PipelineBuilder.h"
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
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						描画前処理
// TODO:ループ内の前処理後処理を作成
	void DirectXCore::BeginPresentRenderTarget() {
		/// バックバッファの決定
		SettleCommandList();
		/// バリア設定
		SetupTransitionBarrier();
		// 描画ターゲットの設定とクリア
		RenderTargetPreference();

		// ViewPortとScissorRectの設定
		commandList_->RSSetViewports(1, &viewport_);
		commandList_->RSSetScissorRects(1, &scissorRect_);

		commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void DirectXCore::SetRenderBarrierRecorder(RenderBarrierRecorder *recorder) {
		renderBarrierRecorder_ = recorder;
	}

	void DirectXCore::SetRenderTransitionExecutor(RenderTransitionExecutor *executor) {
		renderTransitionExecutor_ = executor;
	}

	ID3D12Resource *DirectXCore::ResolveRenderResource(RenderResourceId resourceId) {
		switch(resourceId) {
		case RenderResourceId::SceneColor:
			return renderTextureResources_[renderResourceIndex_].Get();
		case RenderResourceId::PresentColor:
			return swapChainResource_[currentBackBufferIndex_].Get();
		case RenderResourceId::SceneDepth:
			return depthStencilResource_.Get();
		}
		return nullptr;
	}

	void DirectXCore::TransitionResource(
		RenderResourceId resourceId,
		ID3D12Resource &resource,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState,
		RenderBarrierPoint point) {
		assert(commandList_);
		assert(beforeState != afterState);

		if(renderBarrierRecorder_) {
			renderBarrierRecorder_->Transition(*commandList_.Get(), resourceId, resource, beforeState, afterState, point);
			return;
		}

		// NOTE: DirectX初期化中はRenderer/RenderGraphが未生成のため、低レベルBarrierだけを発行する。
		barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier_.Transition.pResource = &resource;
		barrier_.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier_.Transition.StateBefore = beforeState;
		barrier_.Transition.StateAfter = afterState;
		commandList_->ResourceBarrier(1, &barrier_);
	}

	///=============================================================================
	///						描画後処理
	void DirectXCore::PostDraw() {
		// FPS固定
		UpdateFixFPS();
		// コマンドリストのクローズと実行
		CloseCommandList();
		ExecuteCommandList();
	}

	///=============================================================================
	///						DirectXの初期化
	void DirectXCore::InitializeDirectX(WinApp *winApp) {
		//=======================================
		// FPS固定初期化
		InitializeFixFPS();
		//=======================================
		/// WinApp
		/// NULL検出
		assert(winApp);
		/// メンバ変数に記録
		this->winApp_ = winApp;
		//=======================================
		// ウィンドウハンドルの取得
		CreateDebugLayer();
		// DXGIファクトリーの生成
		CreateDxgiFactory();
		// アダプタの選択
		SelectAdapter();
		// D3D12デバイスの生成
		CreateD3D12Device();
		// コマンドキューの生成
		SetupErrorHandling();
		// コマンドキューの生成
		CreateCommandQueue();
		// フレームコンテキストとコマンドリストの生成
		InitializeFrameContextsAndCommandList();
		// コマンドリストの生成
		CreateSwapChain();
		// フェンスの生成
		CreateFence();
		// DescriptorAllocatorの初期化
		InitializeDescriptorAllocators();
		// 深度バッファの生成
		CreateDepthBuffer();
		// スワップチェーンからリソースを取得
		GetResourcesFromSwapChain();
		// RTVの生成
		CreateRenderTargetViews();
		//=======================================
		// レンダーテクスチャのRTVを生成
		CreateRenderTextureRTV();
		//=======================================
		// コマンドリストの決定
		SettleCommandList();
		// バリアの設定
		SetupTransitionBarrier();
		// DXCコンパイラーの初期化
		CreateDXCCompiler();
		// ビューポートとシザーレクトの生成
		CreateViewportAndScissorRect();
		//=======================================
		// コマンドリストの設定
		CloseCommandList();
		// コマンドキック
		ExecuteCommandList();
		//=======================================
		// オフスクリーンの初期化
		CreateOffScreenPipeLine();
	}

	///=============================================================================
	///						開放処理
	void DirectXCore::ReleaseDirectX() {
		/// 開放処理
		ReleaseResources();
	}

	///=============================================================================
	///						GPU完了待ち
	void DirectXCore::WaitForGpuIdle() {
		if(!commandQueue_ || !fence_ || !fenceEvent_) {
			return;
		}

		const uint64_t fenceValue = ++nextFenceValue_;
		hr_ = commandQueue_->Signal(fence_.Get(), fenceValue);
		assert(SUCCEEDED(hr_));

		if(fence_->GetCompletedValue() < fenceValue) {
			hr_ = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
			assert(SUCCEEDED(hr_));
			WaitForSingleObject(fenceEvent_, INFINITE);
		}

		for(FrameContext &frameContext : frameContexts_) {
			frameContext.SetFenceValue(fenceValue);
		}
	}

	///=============================================================================
	///						デバックレイヤーの生成
	void DirectXCore::CreateDebugLayer() {
#ifdef _DEBUG
		debugController_ = nullptr;
		if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
			// NOTE: Device生成前だけ有効。Graphics Tools未導入環境では起動継続する。
			debugController_->EnableDebugLayer();
			Logger::Log("D3D12 Debug Layer enabled.", Logger::LogLevel::Success);
#if defined(MAGENGINE_ENABLE_GPU_VALIDATION)
			// NOTE: GPU-Based Validationは重いため、明示フラグを定義した開発時だけ有効化する。
			debugController_->SetEnableGPUBasedValidation(TRUE);
			Logger::Log("D3D12 GPU-Based Validation enabled.", Logger::LogLevel::Warning);
#endif
		} else {
			Logger::Log("D3D12 Debug Layer is unavailable. Install Graphics Tools to enable it.", Logger::LogLevel::Warning);
		}
#endif
	}

	///=============================================================================
	///						DXGIファクトリーの生成
	void DirectXCore::CreateDxgiFactory() {
		dxgiFactory_ = nullptr;
		// HRESULTはWindows系のエラーコードであり、
		// 開数が成功したかどうかをSUCCEEDマクロで判断できる
		hr_ = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
		// 初期化の根本的なエラーを判断するためassertにする
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						使用するアダプタ用変数
	void DirectXCore::SelectAdapter() {
		useAdapter_ = nullptr;
		// 良い順にアダプタを頼む
		for(UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) !=
			DXGI_ERROR_NOT_FOUND;
			i++) {
		   // アダプターの情報を取得
			hr_ = useAdapter_->GetDesc3(&adapterDesc_);
			assert(SUCCEEDED(hr_)); // 取得不可
			// ソフトウェアアダプタでなければ採用
			if(!( adapterDesc_.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE )) {
				// 採用したアダプタの情報をログに出力。Wstringの方に注意
				Logger::Log(WstringUtility::ConvertString(std::format(L"Use Adapter;{}", adapterDesc_.Description)), Logger::LogLevel::Info);
				break;
			}
			useAdapter_ = nullptr; // ソフトウェアアダプタの場合は見なかったことにできる
		}
		// 適切なアダプタが見つからなかったので起動不可
		assert(useAdapter_ != nullptr);
	}

	///=============================================================================
	///						D3D12Deviceの作成
	void DirectXCore::CreateD3D12Device() {
		device_ = nullptr;
		//=======================================
		// 機能レベルとログ出力用の文字列
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
		const char *feartureLevelStrings[] = { "12.2", "12.1", "12.0" };
		// 高い順に生成できるか試してみる
		for(size_t i = 0; i < _countof(featureLevels); ++i) {
			// 採用したアダプタでデバイスを作成
			hr_ = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
			// 指定した機能レベルでデバイスが生成できたかを確認
			if(SUCCEEDED(hr_)) {
				// 生成できたのでログ出力を行ってループを抜ける
				Logger::Log(std::format("FeatureLevel : {}", feartureLevelStrings[i]), Logger::LogLevel::Info);
				break;
			}
		}
		//=======================================
		// デバイスの生成がうまくいかなかったので起動できない
		assert(device_ != nullptr);
		// 初期化完了のログの出力
		Logger::Log("Complete create D3D12Device!!!", Logger::LogLevel::Success);
	}

	///=============================================================================
	///						エラー・警告の場合即停止(初期化完了のあとに行う)
	void DirectXCore::SetupErrorHandling() {
		ConfigureDebugInfoQueue();
	}

	///=============================================================================
	///						Debug InfoQueueの設定
	void DirectXCore::ConfigureDebugInfoQueue() {
#ifdef _DEBUG
		infoQueue_ = nullptr;
		if(SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
			// NOTE: Smoke Testでは重大度の高い破綻だけ停止し、WARNINGはログ確認対象にする。
			infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			//=======================================
			// 特定のエラーの無視など
			// 抑制するメッセージのID
			D3D12_MESSAGE_ID denyIds[] = {
				// NOTE:Windows11でのDXGIデバックレイヤーとDX12デバックレイヤーの相互作用バグによるエラーメッセージ
				// https://stakoverflow.com/question/69805245/directx-12-application-is-crashing-in-windows-11
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
			// 抑制するレベル
			D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			// 指定したメッセージの表示を抑制する
			infoQueue_->PushStorageFilter(&filter);
			Logger::Log("D3D12 InfoQueue configured. Break on CORRUPTION and ERROR.", Logger::LogLevel::Info);
		} else {
			Logger::Log("D3D12 InfoQueue is unavailable.", Logger::LogLevel::Warning);
		}
#endif //  _DEBUG
	}

	///=============================================================================
	///						コマンドキューを作成する
	void DirectXCore::CreateCommandQueue() {
		commandQueue_ = nullptr;
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
		hr_ = device_->CreateCommandQueue(&commandQueueDesc,
			IID_PPV_ARGS(&commandQueue_));
// コマンドキューの生成がうまくいかなかったので起動できない
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						コマンドアロケータを生成する
	void DirectXCore::InitializeFrameContextsAndCommandList() {
		for(FrameContext &frameContext : frameContexts_) {
			frameContext.Initialize(*device_.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		}
		// コマンドリスト
		commandList_ = nullptr;
		hr_ = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContexts_[currentFrameContextIndex_].GetCommandAllocator(), nullptr,
			IID_PPV_ARGS(&commandList_));
// コマンドリストの生成がうまくいかなかったので起動できない
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						スワップチェーンを生成する
	void DirectXCore::CreateSwapChain() {
		swapChain_ = nullptr;
		swapChainDesc_.Width = winApp_->GetWindowWidth();
		swapChainDesc_.Height = winApp_->GetWindowHeight();
		swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc_.SampleDesc.Count = 1;
		swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc_.BufferCount = SwapChainBufferCount;
		swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		// コマンドキュー、ウィンドウバレル、設定を渡して生成する
		hr_ = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), winApp_->GetWindowHandle(), &swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1 **>( swapChain_.GetAddressOf() ));
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						FenceとEventの生成
	void DirectXCore::CreateFence() {
		// 初期値0でFenceを作る
		fence_ = nullptr;
		nextFenceValue_ = 0;
		hr_ = device_->CreateFence(nextFenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
		assert(SUCCEEDED(hr_));
		// FenceのSignalを持つためのイベントを生成する
		fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent_ != nullptr);
	}

	///=============================================================================
	///						深度バッファの生成
	void DirectXCore::CreateDepthBuffer() {
		//=======================================
		// DepthStencilTextureをウィンドウのサイズで作成
		depthStencilResource_ = CreateDepthStencilTextureResource(winApp_->GetWindowWidth(), winApp_->GetWindowHeight());
		// NOTE:DSV DescriptorはDirectXCore所有Allocatorから確保し、Depthリソースより長く保持する
		dsvHandle_ = dsvAllocator_.Allocate();
		//=======================================
		// dsvの設定
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		   // Format。基本的にはResourceに合わせる
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2dTexture
		// DSVHeapの先頭にDSVを作る
		device_.Get()->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHandle_.cpuHandle);
	}

	///=============================================================================
	///						DescriptorAllocatorの初期化
	void DirectXCore::InitializeDescriptorAllocators() {
		// NOTE:現在は永続Descriptorのみを単調増加で扱う。フレーム一時DescriptorはFrameContext導入時に分離する
		rtvAllocator_.Initialize(*device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DescriptorCapacity::Rtv, false);
		dsvAllocator_.Initialize(*device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DescriptorCapacity::Dsv, false);
		resourceAllocator_.Initialize(*device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescriptorCapacity::Resource, true);
		samplerAllocator_.Initialize(*device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DescriptorCapacity::Sampler, true);
	}

	///=============================================================================
	///						SwapChainからResource
	void DirectXCore::GetResourcesFromSwapChain() {
		for(uint32_t i = 0; i < SwapChainBufferCount; ++i) {
			// SwapChainのBufferCountと一致する範囲だけ取得する
			hr_ = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResource_[i]));
			assert(SUCCEEDED(hr_));
		}
	}

	///=============================================================================
	///						RTVの作成
	void DirectXCore::CreateRenderTargetViews() {
		rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		// 出力結果をSRGBに変換して書き込む
		rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む
		// NOTE:SwapChain BackBufferのRTVはallocatorから順に確保し、手動ptr加算を行わない
		for(uint32_t i = 0; i < SwapChainBufferCount; ++i) {
			swapChainRtvHandles_[i] = rtvAllocator_.Allocate();
			device_->CreateRenderTargetView(swapChainResource_[i].Get(), &rtvDesc_, swapChainRtvHandles_[i].cpuHandle);
		}
	}

	///=============================================================================
	///						コマンド積み込んで確定させる
	void DirectXCore::SettleCommandList() {
		// これから書き込むバックバッファのインデックスを取得
		currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
	}

	///=============================================================================
	///						TransitionBarrierを張る
	void DirectXCore::SetupTransitionBarrier() {
		if(renderTransitionExecutor_) {
			renderTransitionExecutor_->ExecuteBoundary(*commandList_.Get(), RenderTransitionBoundary::BeginPresentRenderTarget);
			return;
		}
		TransitionResource(
			RenderResourceId::PresentColor,
			*swapChainResource_[currentBackBufferIndex_].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			RenderBarrierPoint::BeginPresentRenderTarget);
	}

	///=============================================================================
	///						RenderTargetの設定
	void DirectXCore::RenderTargetPreference() {
		// 描画先のRTVを設定する
		commandList_->OMSetRenderTargets(1, &swapChainRtvHandles_[currentBackBufferIndex_].cpuHandle, false, &dsvHandle_.cpuHandle);
		// 指定した色で画面全体をクリアする	背景色！
		float clearColor[] = { 0.05f, 0.05f, 0.05f, 1.0f }; // この色を変更することでウィンドウの色を黒に変更できます
		commandList_->ClearRenderTargetView(swapChainRtvHandles_[currentBackBufferIndex_].cpuHandle, clearColor, 0, nullptr);
		commandList_->ClearDepthStencilView(dsvHandle_.cpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);
	}

	///=============================================================================
	///						ビューポートとシザーレクトの生成
	void DirectXCore::CreateViewportAndScissorRect() {
		//========================================
		// ビューポート
		// クライアント領域のサイズと一緒にして画面全体に表示
		viewport_.Width = static_cast<float>( winApp_->GetWindowWidth() );
		viewport_.Height = static_cast<float>( winApp_->GetWindowHeight() );
		viewport_.TopLeftX = 0;
		viewport_.TopLeftY = 0;
		viewport_.MinDepth = 0;
		viewport_.MaxDepth = 1.0f;
		//========================================
		// シザー矩形
		// 基本的にビューポートと同じ矩形が構成されるようになる
		scissorRect_.left = 0;
		scissorRect_.right = winApp_->GetWindowWidth();
		scissorRect_.top = 0;
		scissorRect_.bottom = winApp_->GetWindowHeight();
	}

	///=============================================================================
	///						コマンドリストの決定
	void DirectXCore::CloseCommandList() {
		// 画面に書く処理はすべて終わり。画面に映すので状態を遷移
		// 今回はRenderTargetからPresebtにする
		if(renderTransitionExecutor_) {
			renderTransitionExecutor_->ExecuteBoundary(*commandList_.Get(), RenderTransitionBoundary::BeforePresent);
		} else {
			TransitionResource(
				RenderResourceId::PresentColor,
				*swapChainResource_[currentBackBufferIndex_].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT,
				RenderBarrierPoint::BeforePresent);
		}
		// コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
		hr_ = commandList_->Close();
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						コマンドのキック
	void DirectXCore::ExecuteCommandList() {
		EndFrame();
		BeginFrame();
	}

	///=============================================================================
	///						フレーム終了
	void DirectXCore::EndFrame() {
		// GPUにコマンドリストの実行を行わせる
		ID3D12CommandList *commandLists[] = { commandList_.Get() };
		commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

		// GPUとOSに画面の交換を行うように通知する
		hr_ = swapChain_->Present(1, 0);
		assert(SUCCEEDED(hr_));

		const uint64_t fenceValue = ++nextFenceValue_;
		hr_ = commandQueue_->Signal(fence_.Get(), fenceValue);
		assert(SUCCEEDED(hr_));
		frameContexts_[currentFrameContextIndex_].SetFenceValue(fenceValue);

		// NOTE:既存の弾・エフェクト・シーン破棄は即時解放のため、DeferredReleaseQueue導入まではGPU参照中破棄を防ぐ。
		WaitForFrameContext(frameContexts_[currentFrameContextIndex_]);

		currentFrameContextIndex_ = (currentFrameContextIndex_ + 1) % FramesInFlight;
	}

	///=============================================================================
	///						フレーム開始
	void DirectXCore::BeginFrame() {
		FrameContext &frameContext = frameContexts_[currentFrameContextIndex_];
		WaitForFrameContext(frameContext);

		// 次フレーム用のコマンドリストを準備
		frameContext.ResetCommandAllocator();
		hr_ = commandList_->Reset(frameContext.GetCommandAllocator(), nullptr);
		assert(SUCCEEDED(hr_));
		currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
	}

	///=============================================================================
	///						FrameContextのGPU完了待ち
	void DirectXCore::WaitForFrameContext(const FrameContext &frameContext) {
		const uint64_t fenceValue = frameContext.GetFenceValue();
		if(fenceValue == 0 || fence_->GetCompletedValue() >= fenceValue) {
			return;
		}

		hr_ = fence_->SetEventOnCompletion(fenceValue, fenceEvent_);
		assert(SUCCEEDED(hr_));
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	///=============================================================================
	///						開放処理
	void DirectXCore::ReleaseResources() {
		// GPU処理の完了を待つ（リソース破棄前に必須）
		WaitForGpuIdle();
		
		if(fenceEvent_) {
			CloseHandle(fenceEvent_);
			fenceEvent_ = nullptr;
		}
#ifdef _DEBUG
	// debugController_->Release();
#endif // DEBUG
		CloseWindow(winApp_->GetWindowHandle());
	}

	///=============================================================================
	///						リソースリークチェック
	void DirectXCore::CheckResourceLeaks() {
#ifdef _DEBUG
		Microsoft::WRL::ComPtr<IDXGIDebug> debug;
		if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			// NOTE: 外部ライブラリ由来のLive Objectも出るため、Smoke Testでは内容を確認して分類する。
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		} else {
			Logger::Log("DXGI debug interface is unavailable. Live Object report skipped.", Logger::LogLevel::Warning);
		}
#endif
	}

	///=============================================================================
	///						Debug Layerメッセージの出力
	void DirectXCore::ReportDebugMessages() const {
#ifdef _DEBUG
		if(!infoQueue_) {
			Logger::Log("D3D12 InfoQueue is unavailable. Debug messages cannot be reported.", Logger::LogLevel::Warning);
			return;
		}

		const UINT64 messageCount = infoQueue_->GetNumStoredMessages();
		Logger::Log(std::format("D3D12 InfoQueue stored messages: {}", messageCount), messageCount == 0 ? Logger::LogLevel::Success : Logger::LogLevel::Warning);

		for(UINT64 i = 0; i < messageCount; ++i) {
			SIZE_T messageLength = 0;
			if(FAILED(infoQueue_->GetMessage(i, nullptr, &messageLength)) || messageLength == 0) {
				continue;
			}
			std::vector<char> messageBuffer(messageLength);
			auto *message = reinterpret_cast<D3D12_MESSAGE *>(messageBuffer.data());
			if(SUCCEEDED(infoQueue_->GetMessage(i, message, &messageLength))) {
				Logger::Log(std::format("D3D12 Message[{}]: {}", i, message->pDescription), Logger::LogLevel::Warning);
			}
		}
#endif
	}

	///=============================================================================
	///						DXCコンパイラーの初期化
	void DirectXCore::CreateDXCCompiler() {
		// dxcCompilerを初期化
		hr_ = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
		assert(SUCCEEDED(hr_));
		hr_ = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
		assert(SUCCEEDED(hr_));
		// 現時点でincludeはしないが、includeに対応するために設定を行う
		hr_ = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
		assert(SUCCEEDED(hr_));
	}

	///=============================================================================
	///						深度BufferステンシルBufferの生成関数
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateDepthStencilTextureResource(int32_t width, int32_t height) {
		//=======================================
		// 生成するResouceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;									  // テクスチャの幅
		resourceDesc.Height = height;								  // テクスチャの高さ
		resourceDesc.MipLevels = 1;									  // mipmapの数
		resourceDesc.DepthOrArraySize = 1;							  // 奥行きor配列Textureの配列数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		  // DepthStencillとして利用可能なFormat
		resourceDesc.SampleDesc.Count = 1;							  // サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencillとして使う通知
		//=======================================
		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
		//=======================================
		// 深度値のクリア設定
		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.DepthStencil.Depth = 1.0f;				// 1.0F(最大値)でクリア
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる
		//=======================================
		// 設定を元にResourceの生成を行う
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device_->CreateCommittedResource(
			&heapProperties,				  // Heapの設定
			D3D12_HEAP_FLAG_NONE,			  // heepの特殊な設定。特になし。
			&resourceDesc,					  // Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
			&depthClearValue,				  // Clear最適値
			IID_PPV_ARGS(&resource));		  // 作成するResourceポインタへのポインタ
											  //========================================
											  // エラーチェック
#ifdef _DEBUG
		assert(SUCCEEDED(hr));
#endif // _DEBUG
		if(FAILED(hr)) {
			// 深度ステンシルテクスチャの生成がうまくいかなかったので起動できない
			Logger::Log("Failed to create depth stencil texture resource.", Logger::LogLevel::Error);
			return nullptr;
		}
		//========================================
		// 出力
		return resource;
	}

	// HOTFIX :
	void WriteToFile(const std::string &fileName, const std::string &text) {
		// 出力ファイルストリームを生成
		std::ofstream outputFile(fileName);

		// ファイルが開けなかった場合のエラーチェック
		if(!outputFile) {
			std::cerr << "ファイルを開けませんでした: " << fileName << std::endl;
			return;
		}

		// 指定の文字列を書き込む
		outputFile << text;

		// ファイルを閉じる（ofstreamはスコープを抜けると自動で閉じられるが明示的に閉じてもよい）
		outputFile.close();

		std::cout << "ファイルに書き込みました: " << fileName << std::endl;
	}

	///=============================================================================
	///						シェーダーのコンパイル
	IDxcBlob *DirectXCore::CompileShader(const std::wstring &filePath, const wchar_t *profile) {
		//=======================================
		// hlseファイルを読む
		// これからシェーダーをコンパイルする旨をログに出す
		Logger::Log(WstringUtility::ConvertString(std::format(L"Begin Compiler,path:{},profile:{}", filePath, profile)), Logger::LogLevel::Info);
		// hlseファイルを読む
		IDxcBlobEncoding *shaderSource = nullptr;
		HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
		// 読めなかったら止める
		assert(SUCCEEDED(hr));
		// 読み込んだファイルの内容を設定する
		DxcBuffer shaderSourceBuffer = {};
		shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
		shaderSourceBuffer.Size = shaderSource->GetBufferSize();
		shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8の文字コードであることを通知
		//=======================================
		// コンパイルする
		LPCWSTR arguments[] = {
			filePath.c_str(),
			L"-E",
			L"main",
			L"-T",
			profile,
			L"-Zi",
			L"-Qembed_debug",
			L"-Od",
			L"-Zpr",
			L"-I", L"resources/shader/" };
		// 実際にShaderをコンパイルする
		IDxcResult *shaderResult = nullptr;
		hr = dxcCompiler_->Compile(
			&shaderSourceBuffer,
			arguments,
			_countof(arguments),
			includeHandler_,
			IID_PPV_ARGS(&shaderResult));
		// コンパイルエラーではなくdxcが起動できないと致命的な状況
		assert(SUCCEEDED(hr));
		//=======================================
		// 警告・エラーがでてないか確認する
		IDxcBlobUtf8 *shaderError = nullptr;
		if(shaderResult->HasOutput(DXC_OUT_ERRORS)) {
			shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
		}
		// エラーがある場合はエラーを出力して終了
		if(shaderError != nullptr && shaderError->GetStringLength() != 0) {
			Log(shaderError->GetStringPointer(), Logger::LogLevel::Error);
			if(shaderResult->HasOutput(DXC_OUT_ERRORS)) {
				shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
			}

			// HOTFIX:エラー内容をファイルに書き出す
			WriteToFile("shaderError.txt", shaderError->GetStringPointer());

			// 警告・エラーダメ絶対
			assert(false);
		}
		//=======================================
		// Compile結果を受け取って返す
		// コンパイル結果から実行用のバイナリ部分を取得
		IDxcBlob *shaderBlob = nullptr;
		hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		// 成功したログを出す
		Logger::Log(WstringUtility::ConvertString(std::format(L"Compile Succeeded, path:{},profile:{}", filePath, profile)), Logger::LogLevel::Success);
		// もう使わないリソースを開放
		shaderSource->Release();
		shaderResult->Release();
		// 実行用のバイナリを返却
		return shaderBlob;
	}

	///=============================================================================
	///						バッファーリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateBufferResource(size_t sizeInByte) { // バッファリソースの設定を作成
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInByte;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//=======================================
		// アップロードヒープのプロパティを設定
		// 頂点リソース用のヒープ設定
		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		//=======================================
		// リソースを作成
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device_->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource));
		//=======================================
		// エラーチェック
		if(FAILED(hr) || !resource) {
			// リソースの作成に失敗した場合、エラーメッセージを出力して nullptr を返す
			return nullptr;
		}

		return resource;
	}

	///=============================================================================
	///						テクスチャリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateTextureResource(const DirectX::TexMetadata &metadata) {
		//=======================================
		// 1.metadataを元にResouceの設定
		D3D12_RESOURCE_DESC resouceDesc{};
		resouceDesc.Width = UINT(metadata.width);							  // Textureの幅
		resouceDesc.Height = UINT(metadata.height);							  // Textureの高さ
		resouceDesc.MipLevels = UINT16(metadata.mipLevels);					  // mipmapの数
		resouceDesc.DepthOrArraySize = UINT16(metadata.arraySize);			  // 奥行き or 配列Textureの配列数
		resouceDesc.Format = metadata.format;								  // TextureのFormat
		resouceDesc.SampleDesc.Count = 1;									  // サンプリングカウント
		resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段つかているのは2次元。
		//=======================================
		// 2.利用するHeapの設定
		// TODO:リソースの場所を変更する03_00_ex
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 細かい設定を行う
		//=======================================
		// 3.resouceを生成する
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device_->CreateCommittedResource(
			&heapProperties,				// Heapの設定
			D3D12_HEAP_FLAG_NONE,			// Heapの特殊な設定、特になし
			&resouceDesc,					// Resourceの設定
			D3D12_RESOURCE_STATE_COPY_DEST, // 初回のResouceState。Textureは基本読むだけ
			nullptr,
			IID_PPV_ARGS(&resource));
		//========================================
		// エラーチェック
#ifdef _DEBUG
		assert(SUCCEEDED(hr));
#endif // _DEBUG
		if(FAILED(hr)) {
			// 深度ステンシルテクスチャの生成がうまくいかなかったので起動できない
			Log("Failed to create depth stencil texture resource.", Logger::LogLevel::Error);
			return nullptr;
		}
		//========================================
		// 出力
		return resource;
	}

	///=============================================================================
	///						テクスチャデータの転送
	// NOTE:以下の手順を行う
	// 3.CPUで書き込む用にUploadHeapnnoResourceを作成
	// 4.3に対してCPUでデータを書き込む
	// 5.CommandListに3を2に転送するコマンドを積む
	// NOTE:以下の文字は属性というもの。戻り値を破棄してはならないことを示す。
	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage &mipImages) {
		//=======================================
		// 中間リソースの作成
		std::vector<D3D12_SUBRESOURCE_DATA> subresource;
		DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresource);
		// Subresourceの数を元に、コピー元となるintermediateResourceに必要なサイズを計算する
		uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresource.size()));
		// 計算したサイズでintermediateResourceを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);
		//=======================================
		// データ転送をコマンドに積む
		// interにsubreのデータを書き込み、textureに転送する
		UpdateSubresources(commandList_.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresource.size()), subresource.data());
		//=======================================
		// 読み込み変更コマンド
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = texture.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		commandList_->ResourceBarrier(1, &barrier);
		return intermediateResource;
	}

	///=============================================================================
	///						DXTecを使ってファイルを読む
	DirectX::ScratchImage DirectXCore::LoadTexture(const std::string &filePath) {
		//=======================================
		// テクスチャファイルを読んでプログラムを扱えるようにする
		DirectX::ScratchImage image{};
		std::wstring filePathW = WstringUtility::ConvertString(filePath);
		HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
		assert(SUCCEEDED(hr));
		//=======================================
		// ミニマップの作成
		DirectX::ScratchImage mipImages{};
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
		assert(SUCCEEDED(hr));
		//=======================================
		// ミニマップ付きのデータを返す
		return mipImages;
	}

	///=============================================================================
	///						レンダーテクスチャ系
	///--------------------------------------------------------------
	///						 レンダーテクスチャの前処理
	void DirectXCore::RenderTexturePreDraw() {
		if(renderTransitionExecutor_) {
			renderTransitionExecutor_->ExecuteBoundary(*commandList_.Get(), RenderTransitionBoundary::RenderTexturePreDraw);
		} else {
			TransitionResource(
				RenderResourceId::SceneColor,
				*renderTextureResources_[renderResourceIndex_].Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				RenderBarrierPoint::RenderTexturePreDraw);
		}
		//=======================================
		// NOTE:RenderTexture RTVはAllocatorから確保済みのHandleを保持して使う
		commandList_->OMSetRenderTargets(1, &renderTextureRtvHandles_[renderResourceIndex_].cpuHandle, false, &dsvHandle_.cpuHandle);
		//=======================================
		// 指定した色で画面全体をクリアする#4c6cb3
		float clearColor[] = { 0.298f, 0.427f, 0.698f, 1.0f }; // この色を変更することでウィンドウの色を黒に変更できます
		commandList_->ClearRenderTargetView(renderTextureRtvHandles_[renderResourceIndex_].cpuHandle, clearColor, 0, nullptr);
		//=======================================
		// 画面全体の深度をクリア
		commandList_->ClearDepthStencilView(dsvHandle_.cpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList_->RSSetViewports(1, &viewport_);	   // Viewportを設定
		commandList_->RSSetScissorRects(1, &scissorRect_); // Scissorを設定
	}

	///--------------------------------------------------------------
	///						 レンダーテクスチャの後処理
	void DirectXCore::RenderTexturePostDraw() {
		if(renderTransitionExecutor_) {
			renderTransitionExecutor_->ExecuteBoundary(*commandList_.Get(), RenderTransitionBoundary::RenderTexturePostDraw);
			return;
		}
		TransitionResource(
			RenderResourceId::SceneColor,
			*renderTextureResources_[renderResourceIndex_].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			RenderBarrierPoint::RenderTexturePostDraw);
	}

	///--------------------------------------------------------------
	///                        レンダーテクスチャの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, const MagMath::Vector4 &clearColor, D3D12_RESOURCE_STATES initialState) {
		//========================================
		// 1.metadataを元にResouceの設定
		D3D12_RESOURCE_DESC resouceDesc{};
		resouceDesc.Width = UINT(width);							 // Textureの幅
		resouceDesc.Height = UINT(height);							 // Textureの高さ
		resouceDesc.MipLevels = 1;									 // mipmapの数
		resouceDesc.DepthOrArraySize = 1;							 // 奥行き or 配列Textureの配列数
		resouceDesc.Format = format;								 // TextureのFormat
		resouceDesc.SampleDesc.Count = 1;							 // サンプリングカウント
		resouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	 // Textureの次元数。普段つかているのは2次元。
		resouceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして使う通知

		//========================================
		// 2.利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // 当然VRAM上に作る

		//========================================
		// 2.5 深度値のクリア設定
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = format;			// Format。Resourceと合わせる
		clearValue.Color[0] = clearColor.x; // クリア色の設定
		clearValue.Color[1] = clearColor.y; // クリア色の設定
		clearValue.Color[2] = clearColor.z; // クリア色の設定
		clearValue.Color[3] = clearColor.w; // クリア色の設定

		//========================================
		// 3.resouceを生成する
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device_->CreateCommittedResource(
			&heapProperties,	  // Heapの設定
			D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定、特になし
			&resouceDesc,		  // Resourceの設定
			// NOTE: 以下のResourceStateは、RenderTargetとして使うため、初期状態をD3D12_RESOURCE_STATE_RENDER_TARGETに設定
			// TODO:また､引数で変更できたほうが便利なので、引数で受け取るようにしておくか検討
			initialState, // 初回のResouceState。Textureは基本読むだけ
			&clearValue,  // Clear最適値
			IID_PPV_ARGS(&resource));

		//========================================
		// エラーチェック
#ifdef _DEBUG
		assert(SUCCEEDED(hr));
#endif // _DEBUG
		if(FAILED(hr)) {
			// 深度ステンシルテクスチャの生成がうまくいかなかったので起動できない
			Log("Failed to create depth stencil texture resource.", Logger::LogLevel::Error);
			return nullptr;
		}

		//========================================
		// 出力
		return resource;
	}
	///--------------------------------------------------------------
	///                        レンダーテクスチャのRTVを生成
	void DirectXCore::CreateRenderTextureRTV() {
		//========================================
		// わかりやすくするために、赤色でクリアする
		const MagMath::Vector4 kRenderTargetClearValue{ 0.298f, 0.427f, 0.698f, 1.0f };
		//========================================
		// 1つ目の作成
		renderTextureResources_[0] = CreateRenderTextureResource(
			winApp_->GetWindowWidth(),
			winApp_->GetWindowHeight(),
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			kRenderTargetClearValue,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		// ========================================
		// NOTE:RTV DescriptorはAllocatorから確保し、BackBuffer数に依存した手動計算を避ける
		renderTextureRtvHandles_[0] = rtvAllocator_.Allocate();
		// レンダーターのビューを作成
		device_->CreateRenderTargetView(renderTextureResources_[0].Get(), &rtvDesc_, renderTextureRtvHandles_[0].cpuHandle);
		// assertはデバッグビルド時にのみ有効になる
		// もし条件がfalseの場合、プログラムは終了する
		renderTextureResources_[0]->SetName(L"renderTexture0");
		assert(renderTextureResources_[0]);
		//========================================
		// 2つ目の作成
		renderTextureResources_[1] = CreateRenderTextureResource(
			winApp_->GetWindowWidth(),
			winApp_->GetWindowHeight(),
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			kRenderTargetClearValue,
			// NOTE: 2つ目のRenderTargetはSRVとしても使うので、初期状態をD3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCEに設定
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		//========================================
		// RTVの設定
		renderTextureRtvHandles_[1] = rtvAllocator_.Allocate();
		// レンダーターのビューを作成
		device_->CreateRenderTargetView(renderTextureResources_[1].Get(), &rtvDesc_, renderTextureRtvHandles_[1].cpuHandle);
		// assertはデバッグビルド時にのみ有効になる
		// もし条件がfalseの場合、プログラムは終了する
		renderTextureResources_[1]->SetName(L"renderTexture1");
		assert(renderTextureResources_[1]);

		//========================================
		// レンダーテクスチャ
		renderResourceIndex_ = 0;
		renderTargetIndex_ = 1;
	}

	///--------------------------------------------------------------
	///                        オフスクリーン用のルートシグネチャを生成
	void DirectXCore::CreateOffScreenRootSignature() {
		HRESULT hr;

		// ルートシグネチャの設定
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// SRV の Descriptor Range
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;													 // t0: Shader Register
		descriptorRange[0].NumDescriptors = 1;														 // 1つのSRV
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;								 // SRV
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 自動計算

		// Root Parameter: SRV (gTexture)
		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用
		// rootParameters[0].DescriptorTable.NumDescriptorRanges = 1; // 1つの範囲
		// rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange; // SRV の範囲

		rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;			   // Tableの中身の配列を指定
		rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

		// Static Sampler
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;						// 全MipMap使用
		staticSamplers[0].ShaderRegister = 0;								// s0: Shader Register
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用

		// ルートシグネチャの構築
		descriptionRootSignature.pParameters = rootParameters;				   // ルートパラメーター配列へのポインタ
		descriptionRootSignature.NumParameters = _countof(rootParameters);	   // 配列の長さ
		descriptionRootSignature.pStaticSamplers = staticSamplers;			   // サンプラー配列へのポインタ
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // サンプラーの数

		// シリアライズしてバイナリにする
		// D3D12SerializeRootSignatureは、ルートシグネチャをバイナリ形式に変換する関数
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_ = nullptr;
		// エラーが発生した場合のBlob
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_ = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);
		if(FAILED(hr)) {
			Logger::Log(reinterpret_cast<char *>( errorBlob_->GetBufferPointer() ));
			assert(false);
		}

		// バイナリを元に生成
		renderTextureRootSignature_ = nullptr;
		hr = device_->CreateRootSignature(0, signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&renderTextureRootSignature_));
		assert(SUCCEEDED(hr));
	}

	///--------------------------------------------------------------
	///                        オフスクリーン用のパイプラインを生成
	PipelineRecipe DirectXCore::CreateRenderTexturePipelineRecipe() const {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/FullScreen.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/FullScreen.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = renderTextureRootSignature_.Get();
		// NOTE: Fullscreen描画は頂点IDで生成する既存Shader仕様のため、InputLayoutは空のまま維持する。
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = false;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	void DirectXCore::CreateOffScreenPipeLine() {
		CreateOffScreenRootSignature();
		// NOTE: RenderTextureやBarrier管理は既存経路に残し、PSO生成の定型処理だけBuilderへ委譲する。
		PipelineBuilder builder(*device_.Get(), *this);
		renderTextureGraphicsPipelineState_ = builder.CreateGraphicsPipeline(CreateRenderTexturePipelineRecipe());
	}

	///=============================================================================
	///						60FPS固定の処理
	///--------------------------------------------------------------
	///						 InitializeFixFPS
	void DirectXCore::InitializeFixFPS() {
		// システムタイマーの分解能を上げる
		timeBeginPeriod(1);
		// 現在時間を記録する
		reference_ = std::chrono::steady_clock::now();
	}

	///--------------------------------------------------------------
	///						 UpdateFixFPS
	void DirectXCore::UpdateFixFPS() {
		// 1/60秒ピッタリの時間
		const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
		// 1/60秒よりわずかに短い時間
		const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

		// 現在時間を取得する
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		// 前回記録からの経過時間を取得する
		std::chrono::microseconds elapsed =
			std::chrono::duration_cast<std::chrono::microseconds>( now - reference_ );

		// 1/60秒(よりわずかに短い時間)経っていない場合
		if(elapsed < kMinCheckTime) {
			// 1/60秒経過するまで軽微なスリープを繰り返す
			while(std::chrono::steady_clock::now() - reference_ < kMinTime) {
				// 1マイクロ秒スリープ
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
		}

		// 現在の時間を記録する
		reference_ = std::chrono::steady_clock::now();
	}
}

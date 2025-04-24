/*********************************************************************
 * \file   DirectXCore.h
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 *********************************************************************/
#pragma once
//========================================
// 標準ライブラリ
#include <cstdint>
#include <string>
#include <format>
#include <cassert>
#include <wrl.h>
#include <chrono>
#include <thread>
//========================================
// 自作関数
#include "WstringUtility.h"
using namespace WstringUtility;
#include "Logger.h"
using namespace Logger;
#include "WinApp.h"
#include "SrvSetup.h"
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
///=============================================================================
///						クラス
class DirectXCore {
public:
	///--------------------------------------------------------------
	///						 メンバ関数

	/// @brief PreDraw ループ前処理
	void PreDraw();

	/// @brief PostDraw ループ後処理
	void PostDraw();

	/// @brief InitializeDirectX DirectXの初期化
	/// @param winApp ウィンドウクラス
	void InitializeDirectX(WinApp *winApp);

	/// @brief ReleaseDirectX DirectXの開放
	/// @note ループ終了時に呼び出す
	void ReleaseDirectX();

	/// @brief CreateDebugLayer デバックレイヤーの生成
	/// @note デバックビルド時に呼び出す
	void CreateDebugLayer();

	/// @brief CreateDxgiFactory DXGIファクトリーの生成
	/// @note DXGIファクトリーはアダプタの選択に必要
	void CreateDxgiFactory();

	/// @brief SelectAdapter 使用するアダプタの選択
	/// @note アダプタはGPUのこと
	void SelectAdapter();

	/// @brief CreateD3D12Device D3D12デバイスの生成
	/// @note D3D12デバイスはコマンドキューの生成に必要
	void CreateD3D12Device();

	/// @brief SetupErrorHandling エラーハンドリングの設定
	/// @note エラーハンドリングはコマンドキューの生成に必要
	void SetupErrorHandling();

	/// @brief CreateCommandQueue コマンドキューの生成
	/// @note コマンドキューはコマンドリストの生成に必要
	void CreateCommandQueue();

	/// @brief CreateCommandAllocator コマンドアロケータの生成
	/// @note コマンドアロケータはコマンドリストの生成に必要
	void CreateCommandAllocator();

	/// @brief CreateSwapChain スワップチェインの生成
	/// @note スワップチェインはレンダーターゲットの生成に必要
	void CreateSwapChain();

	/// @brief CreateFence フェンスの生成
	/// @note フェンスはコマンドリストの実行に必要
	void CreateFence();

	/// @brief CreateDepthBuffer 深度Bufferの生成
	
	void CreateDepthBuffer();

	/// @brief CreateVariousDescriptorHeap ディスクリプタヒープの生成
	void CreateVariousDescriptorHeap();

	/// @brief CreateRTVDescriptorHeap RTVディスクリプタヒープの生成
	/// @note スワップチェインの生成に必要
	void CreateRTVDescriptorHeap();

	/// @brief GetResourcesFromSwapChain スワップチェインからリソースを取得
	/// @note スワップチェインの生成に必要
	void GetResourcesFromSwapChain();

	/// @brief CreateRenderTargetViews レンダーターゲットビューの生成
	/// @note スワップチェインの生成に必要
	void CreateRenderTargetViews();

	/// @brief FenceGeneration フェンスの生成
	/// @note コマンドリストの実行に必要
	void FenceGeneration();

	/// @brief SetttleCommandList コマンドリストの決定
	/// @note コマンドリストの実行に必要
	void SettleCommandList();

	/// @brief SetupTransitionBarrier 遷移バリアの設定
	/// @note コマンドリストの実行に必要
	void SetupTransitionBarrier();

	/// @brief RenderTargetPreference レンダーターゲットの設定
	/// @note コマンドリストの実行に必要
	void RenderTargetPreference();

	/// @brief CreateVirePortAndScissorRect ビューポートとシザーレクトの生成
	/// @note コマンドリストの実行に必要
	/// @note ビューポートは描画領域を指定するために必要
	void CreateVirePortAndScissorRect();

	/// @brief CloseCommandList コマンドリストの決定
	/// @note コマンドリストの実行に必要
	void CloseCommandList();

	/// @brief ExecuteCommandList コマンドリストの実行
	/// @note コマンドリストの実行に必要
	void ExecuteCommandList();

	/// @brief SetupTransitionBarrier 遷移バリアの設定
	/// @note コマンドリストの実行に必要
	void ReleaseResources();

	/// @brief CheckResourceLeaks リソースリークのチェック
	/// @note デバックビルド時に呼び出す
	/// @note リソースリークがあった場合、エラーを出力する
	void CheckResourceLeaks();

	/// @brief ImGuiInitialize ImGuiの初期化
	/// @note ImGuiは描画に必要
	/// @note ImGuiはコマンドリストの実行に必要
	void ImGuiInitialize();

	/// @brief ImGuiDraw ImGuiの描画
	/// @note ImGuiは描画に必要
	/// @note ImGuiはコマンドリストの実行に必要
	void CreateDXCCompiler();


	///--------------------------------------------------------------
	///						 生成系メンバ関数
	/// @brief CreateDepthStencilTextureResource 深度ステンシルテクスチャリソースの生成
	/// @param width 
	/// @param height 
	/// @return 
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

	/// @brief CreateDescriptorHeap ディスクリプタヒープの生成
	/// @param heapType ヒープタイプ
	/// @param numDescriptors ディスクリプタ数
	/// @param shaderVisible シェーダーから見えるか
	/// @return 
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// @brief CompileShader シェーダーのコンパイル
	/// @param filePath	ファイルパス
	/// @param profile	プロファイル
	/// @note	プロファイルは、L"vs_5_0"やL"ps_5_0"などを指定する。
	/// @return 
	IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile);

	/// @brief CreateBufferResource バッファリソースの生成
	/// @param sizeInByte サイズ
	/// @return 
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(size_t sizeInByte);
	
	/// @brief CreateTextureResource テクスチャリソースの生成
	/// @param metadata メタデータ
	/// @return 
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata &metadata);

	/// @brief UploadTextureData テクスチャデータのアップロード
	/// @param texture テクスチャ
	/// @param mipImages ミップマップ
	/// @return アップロードしたテクスチャ
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr <ID3D12Resource> texture, const DirectX::ScratchImage &mipImages);

	/// @brief LoadTexture テクスチャの読み込み
	/// @param filePath ファイルパス
	/// @return 
	static DirectX::ScratchImage LoadTexture(const std::string &filePath);




	/// @brief CreateRenderTextureResource レンダーテクスチャリソースの生成
	/// @param width 幅
	/// @param height 高さ
	/// @param format フォーマット
	/// @param clearColor クリアカラー
	/// @return
	Microsoft::WRL::ComPtr<ID3D12Resource> 
		CreateRenderTextureResource(uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);

	/// @brief CreateRenderTextureRTV レンダーテクスチャのRTVを生成
	void CreateRenderTextureRTV();

	/// @brief CreateDepthStencilTextureResource 深度ステンシルテクスチャリソースの生成
	void CreateRenderTextureSRV();	

	/// @brief CreateDepthStencilDSV 深度ステンシルビューの生成
	void CreateOffScreenRootSignature();

	/// @brief CreateOffScreenRootSignature オフスクリーン用ルートシグネチャの生成
	void CreateOffScreenPipelineState();

    // レンダーテクスチャをターゲットとして設定
    void SetRenderTextureAsTarget();
    
    // スワップチェーンをターゲットとして設定
    void SetSwapChainAsTarget();
	
	/// @brief DrawRenderTextureToSwapChain レンダーテクスチャをスワップチェインに描画
	void DrawRenderTextureToSwapChain();

	/// @brief DrawRenderTextureToSwapChain レンダーテクスチャをスワップチェインに描画
	void SwapRenderTargets();
    
    // レンダーテクスチャのSRV GPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetRenderTextureSrvHandleGPU() const {
        // サービスロケータパターンでSrvSetupから取得する場合
    	return srvSetup_->GetSRVGPUDescriptorHandle(renderTextureSrvIndices_[renderTargetIndex_]);
    }
	
	///--------------------------------------------------------------
	///						 
private:
	/// ===CPU=== ///
	/// @brief GetCPUDescriptorHandle CPUディスクリプタハンドルの取得
	/// @param descriptorHeap ディスクリプタヒープ
	/// @param descriptorSize ディスクリプタサイズ
	/// @param index インデックス
	/// @return CPUディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// ===GPU=== ///
	/// @brief GetGPUDescriptorHandle GPUディスクリプタハンドルの取得
	/// @param descriptorHeap ディスクリプタヒープ
	/// @param descriptorSize ディスクリプタサイズ
	/// @param index インデックス
	/// @return GPUディスクリプタハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// @brief InitializeFixFPS FPS固定更新の初期化
	/// @note FPS固定更新は、ゲームの更新処理をFPSに合わせて固定するために必要
	void InitializeFixFPS();

	/// @brief UpdateFixFPS FPS固定更新の更新
	void UpdateFixFPS();

	///--------------------------------------------------------------
	///						 入出力関数
public:

	/// @brief GetReference 記録時間の取得
	/// @return 記録時間
	WinApp GetWinApp() { return *winApp_; }

	/// @brief GetDebugController デバックコントローラーの取得
	/// @param sHr HRESULT
	void SetHr(HRESULT sHr) { this->hr_ = sHr; }

	/// @brief GetHr HRESULTの取得
	/// @return	HRESULT
	HRESULT GetHr() const { return hr_; }

	/// @brief SetDevice デバイスの設定
	/// @param sDevice デバイス
	void SetDevice(Microsoft::WRL::ComPtr <ID3D12Device> sDevice) { this->device_ = sDevice; }

	/// @brief GetDevice デバイスの取得
	/// @return デバイス
	Microsoft::WRL::ComPtr <ID3D12Device> GetDevice() { return device_; }

	/// @brief SetCommandAllocator コマンドアロケータの設定
	/// @param sCommandList コマンドアロケータ
	/// @note コマンドアロケータは、コマンドリストの生成に必要
	void SetCommandList(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> sCommandList) { this->commandList_ = sCommandList; }

	/// @brief GetCommandList コマンドリストの取得
	/// @return コマンドリスト
	/// @note 取得したコマンドリストは、Reset()を呼び出すことで、リセットすることができる。
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> GetCommandList() { return commandList_.Get(); }

	/// @brief GetSwapChainDesc スワップチェインの設定を取得するゲッター関数。
	/// @return スワップチェインの設定
	/// @note スワップチェインの設定は、DXGI_SWAP_CHAIN_DESC1型である。
	DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const { return swapChainDesc_; }

	/// @brief GetRtvDesc RTVの設定を取得するゲッター関数。
	/// @return RTVの設定
	/// @note RTVの設定は、D3D12_RENDER_TARGET_VIEW_DESC型である。
	D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc() const { return rtvDesc_; }

	/// @brief GetRtvDescriptorHeap RTVディスクリプタヒープを取得するゲッター関数。
	/// @return  RTVディスクリプタヒープ
	/// @note  RTVディスクリプタヒープは、ID3D12DescriptorHeap型である。
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> GetRtvDescriptorHeap() { return rtvDescriptorHeap_; }

	/// @brief SetSrvSetup SrvSetupの設定
	void SetSrvSetup(SrvSetup *srvSetup) { this->srvSetup_ = srvSetup; }


	///--------------------------------------------------------------
	///						 メンバ変数
private:
	//========================================
	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	//========================================
	// WindowsAPI
	WinApp *winApp_ = nullptr;

	//========================================
	// SrvSetupポインタ
	SrvSetup *srvSetup_ = nullptr;

	//========================================
	// デバックレイヤーの生成
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;

	//========================================
	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	HRESULT hr_ = S_OK;  // Initialize HRESULT to S_OK

	//========================================
	// 使用するアダプタ用変数
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;

	//========================================
	// D3D12Deviceの作成
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	//========================================
	// エラー・警告の場合即停止(初期化完了のあとに行う)
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
	//アダプターの情報を取得
	DXGI_ADAPTER_DESC3 adapterDesc_{};

	//========================================
	// コマンドキューを作成する
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

	//========================================
	// コマンドアロケータを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	//========================================
	// スワップチェーンを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

	//========================================
	// Fenceの生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;  // Initialize to nullptr

	//========================================
	// 深度バッファ
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	//========================================
	// DescriptorHeapサイズ
	//uint32_t descriptorSizeSRV = 0;  // SRV
	uint32_t descriptorSizeRTV = 0;  // RTV
	uint32_t descriptorSizeDSV = 0;  // DSV

	//========================================
	// RTVディスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	//RTVの数
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc_{};
	//RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStarHandle_{};
	//RTVを4つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[4]{};
	//これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex_ = 0;
	//TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier_{};

	//========================================
	// SwapChainからResource
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResource_[4] = { nullptr, nullptr };

	//========================================
	// DXCコンパイラ
	IDxcCompiler3 *dxcCompiler_ = nullptr;
	// DXCユーティリティ
	IDxcUtils *dxcUtils_ = nullptr;
	// DXCライブラリ
	IDxcIncludeHandler *includeHandler_ = nullptr;

	//========================================
	// ビューポート
	D3D12_VIEWPORT viewport_{};

	//========================================
	// シザー矩形
	D3D12_RECT scissorRect_{};

	//========================================================
	// ポストエフェクト用
	// レンダーターゲット用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResources[2] = {0, 0};
	// レンダーターゲット用のSRV
	uint32_t renderResourceIndex_ = 0;
	uint32_t renderTargetIndex_ = 1;

	// レンダーテクスチャ関連のリソース
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTextureRtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTextureSrvHeap_;
	
	Microsoft::WRL::ComPtr< ID3D12RootSignature> rootSignature_;
	// パイプラインステートオブジェクト
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
	// シェーダーのコンパイル結果
	Microsoft::WRL::ComPtr< ID3DBlob> signatureBlob_ = nullptr;
	// シェーダーのコンパイル結果
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_ = nullptr;
};


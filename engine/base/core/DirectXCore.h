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
#include "Vector4.h"
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

	/// @brief PostDraw 描画前処理
	void PreDraw();

	/// @brief 描画前処理
	void PostDraw();

	/// @brief InitializeDirectX ダイレクトXの初期化
	/// @param winApp ウィンドウズアプリケーション
	void InitializeDirectX(WinApp *winApp);

	/// @brief ReleaseDirectX ダイレクトXの開放
	void ReleaseDirectX();

	/**----------------------------------------------------------------------------
	 * \brief  CreateDebugLayer デバッグレイヤーの生成
	 */
	void CreateDebugLayer();

	/**----------------------------------------------------------------------------
	 * \brief  CreateDxgiFactory DXGIファクトリーの生成
	 */
	void CreateDxgiFactory();

	/**----------------------------------------------------------------------------
	 * \brief  SelectAdapter アダプタの選択
	 */
	void SelectAdapter();

	/**----------------------------------------------------------------------------
	 * \brief  CreateD3D12Device デバイスの生成
	 */
	void CreateD3D12Device();

	/**----------------------------------------------------------------------------
	 * \brief  SetupErrorHandling エラーハンドリングの設定
	 */
	void SetupErrorHandling();

	/**----------------------------------------------------------------------------
	 * \brief  CreateCommandQueue コマンドキューの生成
	 */
	void CreateCommandQueue();

	/**----------------------------------------------------------------------------
	 * \brief  CreateCommandAllocator コマンドアロケータの生成
	 */
	void CreateCommandAllocator();

	/**----------------------------------------------------------------------------
	 * \brief  CreateSwapChain SwapChainの生成
	 */
	void CreateSwapChain();

	/**----------------------------------------------------------------------------
	 * \brief  CreateFence FenceとEventの生成
	 */
	void CreateFence();

	/**----------------------------------------------------------------------------
	 * \brief  CreateDepthBuffer 深度Bufferの生成
	 */
	void CreateDepthBuffer();

	/**----------------------------------------------------------------------------
	 * \brief  CreateVariousDescriptorHeap ディスクリプタヒープの生成
	 */
	void CreateVariousDescriptorHeap();

	/**----------------------------------------------------------------------------
	 * \brief  CreateRTVDescriptorHeap RTVディスクリプタヒープ

	 */
	void CreateRTVDescriptorHeap();

	/**----------------------------------------------------------------------------
	 * \brief  GetResourcesFromSwapChain SwapChainからResource

	 */
	void GetResourcesFromSwapChain();

	/**----------------------------------------------------------------------------
	 * \brief  CreateRenderTargetViews RTVの作成

	 */
	void CreateRenderTargetViews();

	/**----------------------------------------------------------------------------
	 * \brief  FenceGeneration フェンスの生成
	 */
	void FenceGeneration();

	/**----------------------------------------------------------------------------
	 * \brief  SettleCommandList コマンドリストの決定
	 */
	void SettleCommandList();

	/**----------------------------------------------------------------------------
	 * \brief  SetupTransitionBarrier TransitionBarrierの設定
	 */
	void SetupTransitionBarrier();

	/**----------------------------------------------------------------------------
	 * \brief  RenderTargetPreference レンダーターゲットの設定
	 */
	void RenderTargetPreference();

	/**----------------------------------------------------------------------------
	 * \brief  CreateVirePortAndScissorRect ビューポートとシザーレクトの生成
	 */
	void CreateVirePortAndScissorRect();

	/**----------------------------------------------------------------------------
	 * \brief  CloseCommandList コマンドリストの終了
	 */
	void CloseCommandList();

	/**----------------------------------------------------------------------------
	 * \brief  ExecuteCommandList コマンドリストの実行
	 */
	void ExecuteCommandList();

	/**----------------------------------------------------------------------------
	 * \brief  ReleaseResources リソースの開放
	 */
	void ReleaseResources();

	/**----------------------------------------------------------------------------
	 * \brief  CheckResourceLeaks リソースリークのチェック
	 */
	void CheckResourceLeaks();

	/**----------------------------------------------------------------------------
	 * \brief  ImGuiInitialize ImGuiの初期化
	 */
	void ImGuiInitialize();

	/**----------------------------------------------------------------------------
	 * \brief  CreateDXCCompiler DXCコンパイラーの初期化
	 */
	void CreateDXCCompiler();


	///--------------------------------------------------------------
	///						 生成系メンバ関数
	/**----------------------------------------------------------------------------
	 * \brief  CreateDepthStencilTextureResource 深度BufferステンシルBufferの生成
	 * \param  width
	 * \param  height
	 * \return
	 */
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

	/**----------------------------------------------------------------------------
	 * \brief  CreateDescriptorHeap ディスクリプタヒープの生成
	 * \param  heapType ヒープタイプ
	 * \param  numDescriptors ディスクリプタ数
	 * \param  shaderVisible シェーダーから見えるか
	 * \return
	 */
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/**----------------------------------------------------------------------------
	 * \brief  CompileShader シェーダーのコンパイル
	 * \param  filePath ファイルパス
	 * \param  profile プロファイル
	 * \return
	 */
	IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile);

	/**----------------------------------------------------------------------------
	 * \brief  CreateBufferResource バッファリソースの生成
	 * \param  sizeInByte サイズ
	 * \return
	 */
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(size_t sizeInByte);

	/**----------------------------------------------------------------------------
	 * \brief  CreateTextureResource テクスチャリソースの生成
	 * \param  metadata メタデータ
	 * \return
	 */
	Microsoft::WRL::ComPtr <ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata &metadata);

	/**----------------------------------------------------------------------------
	 * \brief  UploadTextureData テクスチャデータのアップロード
	 * \param  texture テクスチャ
	 * \param  mipImages ミップマップ
	 * \return
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr <ID3D12Resource> texture, const DirectX::ScratchImage &mipImages);

	/**----------------------------------------------------------------------------
	 * \brief  LoadTexture テクスチャの読み込み
	 * \param  filePath ファイルパス
	 * \return
	 */
	static DirectX::ScratchImage LoadTexture(const std::string &filePath);

	//========================================
	// 以下はレンダーテクスチャ用
public:
	/// @brief RenderTexturePreDraw レンダーテクスチャ描画前処理
	void RenderTexturePreDraw();

	/// @brief RenderTexturePostDraw レンダーテクスチャの描画後処理
	void RendertexturePostDraw();

	/// @brief CreateRenderTextureResource レンダーテクスチャリソースの生成
	/// @param width 幅
	/// @param height 高さ
	/// @param format フォーマット
	/// @param clearColor クリアカラー
	/// @return
	Microsoft::WRL::ComPtr<ID3D12Resource> 
		CreateRenderTextureResource(
			uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor, D3D12_RESOURCE_STATES initialState);

	/// @brief CreateRenderTextureRTV レンダーテクスチャのRTVを生成
	/// @param SRVはマネージャの方にあるので、RTVだけ生成する
	void CreateRenderTextureRTV();
		
	///--------------------------------------------------------------
	///                        静的メンバ関数
private:
	/// ===CPU=== ///
	/// @brief GetCPUDescriptorHandle CPUディスクリプタハンドルの取得
	/// @param descriptorHeap ディスクリプタヒープ
	/// @param descriptorSize 
	/// @param index 
	/// @return 
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// ===GPU=== ///
	/// @brief GetGPUDescriptorHandle GPUディスクリプタハンドルの取得
	/// @param descriptorHeap 
	/// @param descriptorSize 
	/// @param index 
	/// @return 
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// @brief InitializeFixFPS FPS固定更新の初期化
	void InitializeFixFPS();

	/// @brief UpdateFixFPS FPS固定更新
	void UpdateFixFPS();


	///--------------------------------------------------------------
	///						 入出力関数
public:

	/**----------------------------------------------------------------------------
	 * \brief  GetWinApp WindowsAPI取得
	 */
	WinApp GetWinApp() { return *winApp_; }

	/**----------------------------------------------------------------------------
	 * \brief  SetHr HRESULT型の変数を設定するセッター関数。
	 * \param  sHr HRESULT型の変数
	 */
	void SetHr(HRESULT sHr) { this->hr_ = sHr; }

	/**----------------------------------------------------------------------------
	 * \brief  GetHr HRESULT型の変数を取得するゲッター関数。
	 */
	HRESULT GetHr() const { return hr_; }

	/**----------------------------------------------------------------------------
	 * \brief  SetDevice デバイスの設定
	 * \param  sDevice デバイス
	 */
	void SetDevice(Microsoft::WRL::ComPtr <ID3D12Device> sDevice) { this->device_ = sDevice; }

	/**----------------------------------------------------------------------------
	 * \brief  GetDevice デバイスの取得
	 */
	Microsoft::WRL::ComPtr <ID3D12Device> GetDevice() { return device_; }

	/**----------------------------------------------------------------------------
	 * \brief  SetCommandList コマンドリストの設定
	 * \param  sCommandList
	 */
	void SetCommandList(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> sCommandList) { this->commandList_ = sCommandList; }

	/**----------------------------------------------------------------------------
	 * \brief  GetCommandList コマンドリストの取得
	 * \return
	 */
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> GetCommandList() { return commandList_.Get(); }

	/**----------------------------------------------------------------------------
	 * \brief  GetSwapChainDesc スワップチェーンの設定の取得
	 */
	DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const { return swapChainDesc_; }

	/**----------------------------------------------------------------------------
	 * \brief  GetRtvDesc RTVディスクリプタの取得
	 */
	D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc() const { return rtvDesc_; }

	/**----------------------------------------------------------------------------
	 * \brief  GetRtvDescriptorHeap RTVディスクリプタヒープの取得
	 */
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> GetRtvDescriptorHeap() { return rtvDescriptorHeap_; }

	/// @brief GetRenderTextureResources レンダーテクスチャリソースの取得
	/// @return
	Microsoft::WRL::ComPtr<ID3D12Resource> GetRenderTextureResource(uint32_t index) { return renderTextureResources_[index]; }

private:
	//========================================
	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

	//========================================
	// WindowsAPI
	WinApp *winApp_ = nullptr;

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
	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[4]{}; //2つ分のRTVを作るので4つ用意する
	//これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex_ = 0;
	//TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier_{};

	//========================================
	// SwapChainからResource
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResource_[2] = { nullptr, nullptr };

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

	//========================================
	// レンダーテクスチャリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResources_[2] = { nullptr, nullptr };
    // レンダーテクスチャの現在の状態を追跡
    D3D12_RESOURCE_STATES renderTextureStates_[2] = {
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    };
	// レンダーリソース
	uint32_t renderResourceIndex_ = 0;
	// レンダーターゲットインデックス
	uint32_t renderTargetIndex_ = 1;
};


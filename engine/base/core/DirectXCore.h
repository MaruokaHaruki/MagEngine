/*********************************************************************
 * \file   DirectXCore.h
 * \brief  DirectX 12のコア機能を管理するクラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note   デバイス、コマンドキューなどの総合管理
 *********************************************************************/
#pragma once
//========================================
// 標準ライブラリ
#include <cassert>
#include <chrono>
#include <cstdint>
#include <format>
#include <string>
#include <thread>
#include <wrl.h>
//========================================
// 自作関数
#include "MagMath.h"
#include "WstringUtility.h"
#include "DescriptorAllocator.h"
#include "Logger.h"
#include "FullscreenPassRendere.h"
#include "GrayscaleEffect.h"
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

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						クラス
	class PostEffectManager;
	class TextureManager;
	class DirectXCore {
	public:
		///--------------------------------------------------------------
		///						 メンバ関数

		//========================================
		/// @brief PostDraw 描画前処理
		/// @param postEffectManager ポストエフェクトマネージャ
		void PreDraw(PostEffectManager *postEffectManager, TextureManager &textureManager);

		//========================================
		/// @brief 描画前処理
		void PostDraw();

		//========================================
		/// @brief InitializeDirectX ダイレクトXの初期化
		/// @param winApp ウィンドウズアプリケーション
		void InitializeDirectX(WinApp *winApp);

		//========================================
		/// @brief ReleaseDirectX ダイレクトXの開放
		void ReleaseDirectX();

		//========================================
		/// @brief WaitForGpu GPUに積まれた処理の完了を待つ
		void WaitForGpu();

		///--------------------------------------------------------------
		///						 ダイレクトXの初期化系
		//========================================
		/// @brief CreateDebugLayer デバッグレイヤーの生成
		void CreateDebugLayer();

		//========================================
		/// @brief CreateDxgiFactory DXGIファクトリーの生成
		void CreateDxgiFactory();

		//========================================
		/// @brief SelectAdapter アダプタの選択
		void SelectAdapter();

		//========================================
		/// @brief CreateD3D12Device デバイスの生成
		void CreateD3D12Device();

		//========================================
		/// @brief SetupErrorHandling エラーハンドリングの設定
		void SetupErrorHandling();

		//========================================
		/// @brief CreateCommandQueue コマンドキューの生成
		void CreateCommandQueue();

		//========================================
		/// @brief CreateCommandAllocator コマンドアロケータの生成
		void CreateCommandAllocator();

		//========================================
		/// @brief CreateSwapChain SwapChainの生成
		void CreateSwapChain();

		//========================================
		/// @brief CreateFence FenceとEventの生成
		void CreateFence();

		//========================================
		/// @brief CreateDepthBuffer 深度Bufferの生成
		void CreateDepthBuffer();

		//========================================
		/// @brief InitializeDescriptorAllocators DescriptorAllocatorの初期化
		void InitializeDescriptorAllocators();

		//========================================
		/// @brief GetResourcesFromSwapChain スワップチェーンからリソースを取得
		void GetResourcesFromSwapChain();

		//========================================
		/// @brief CreateRenderTargetViews RTVの作成
		void CreateRenderTargetViews();

		//========================================
		/// @brief SettleCommandList コマンドリストの決定
		void SettleCommandList();

		//========================================
		/// @brief SetupTransitionBarrier TransitionBarrierの設定
		void SetupTransitionBarrier();

		//========================================
		/// @brief RenderTargetPreference レンダーターゲットの設定
		void RenderTargetPreference();

		//========================================
		/// @brief CreateViewportAndScissorRect ビューポートとシザーレクトの生成
		void CreateViewportAndScissorRect();

		//========================================
		/// @brief CloseCommandList コマンドリストの終了
		void CloseCommandList();

		//========================================
		/// @brief ExecuteCommandList コマンドリストの実行
		void ExecuteCommandList();

		//========================================
		/// @brief ReleaseResources リソースの開放
		void ReleaseResources();

		//========================================
		/// @brief CheckResourceLeaks リソースリークのチェック
		void CheckResourceLeaks();

		//========================================
		/// @brief CreateDXCCompiler DXCコンパイラーの初期化
		void CreateDXCCompiler();

		///--------------------------------------------------------------
		///						 生成系メンバ関数
		//========================================
		/// @brief  CreateDepthStencilTextureResource 深度BufferステンシルBufferの生成
		/// @param  width
		/// @param  height
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

		//========================================
		/// @brief CreateBufferResource バッファリソースの生成
		/// @param sizeInByte サイズ
		/// @return
		IDxcBlob *CompileShader(const std::wstring &filePath, const wchar_t *profile);

		//========================================
		/// @brief CreateBufferResource バッファリソースの生成
		/// @param sizeInByte サイズ
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInByte);

		//========================================
		/// @brief CreateTextureResource テクスチャリソースの生成
		/// @param metadata メタデータ
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata &metadata);

		//========================================
		/// @brief UploadTextureData テクスチャデータのアップロード
		/// @param texture テクスチャリソース
		/// @param mipImages ミップマップ画像
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage &mipImages);

		//========================================
		/// @brief CompileShader シェーダーのコンパイル
		/// @param filePath ファイルパス
		/// @param profile プロファイル
		/// @return
		static DirectX::ScratchImage LoadTexture(const std::string &filePath);

		///--------------------------------------------------------------
		///						レンダーテクスチャ系
	public:
		//========================================
		/// @brief RenderTexturePreDraw レンダーテクスチャ描画前処理
		void RenderTexturePreDraw();

		//========================================
		/// @brief RenderTexturePostDraw レンダーテクスチャの描画後処理
		void RenderTexturePostDraw();

		//========================================
		/// @brief CreateRenderTextureResource レンダーテクスチャリソースの生成
		/// @param width 幅
		/// @param height 高さ
		/// @param format フォーマット
		/// @param clearColor クリアカラー
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource>
			CreateRenderTextureResource(
				uint32_t width, uint32_t height, DXGI_FORMAT format, const MagMath::Vector4 &clearColor, D3D12_RESOURCE_STATES initialState);

			//========================================
			/// @brief CreateRenderTextureRTV レンダーテクスチャのRTVを生成
			/// @param SRVはマネージャの方にあるので、RTVだけ生成する
		void CreateRenderTextureRTV();

		//========================================
		/// @brief CreateOffScreenRootSignature オフスクリーン用のルートシグネチャを生成
		void CreateOffScreenRootSignature();

		//========================================
		/// @brief CreateOffScreenPipeLine オフスクリーン用のパイプラインを生成
		void CreateOffScreenPipeLine();

		///--------------------------------------------------------------
		///                        静的メンバ関数
	private:
		//========================================
		/// @brief InitializeFixFPS FPS固定更新の初期化
		void InitializeFixFPS();

		//========================================
		/// @brief UpdateFixFPS FPS固定更新
		void UpdateFixFPS();

		///--------------------------------------------------------------
		///						 入出力関数
	public:
		//========================================
		/// @brief  GetWinApp WindowsAPI取得
		/// @return WinApp
		WinApp GetWinApp() {
			return *winApp_;
		}

		//========================================
		/// @brief SetHr HRESULT型の変数を設定するセッター関数
		/// \param sHr 設定するHRESULT値
		/// \return
		void SetHr(HRESULT sHr) {
			this->hr_ = sHr;
		}

		//========================================
		/// @brief GetHr HRESULT型の変数を取得するゲッター関数
		/// \return HRESULT値
		HRESULT GetHr() const {
			return hr_;
		}

		//========================================
		/// @brief SetDevice デバイスの設定
		/// @param sDevice デバイスポインタ
		/// \return
		void SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> sDevice) {
			this->device_ = sDevice;
		}

		//========================================
		/// @brief GetDevice デバイスの取得
		/// \return デバイスポインタ
		Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() {
			return device_;
		}

		//========================================
		/// @brief SetCommandList コマンドリストの設定
		/// @param sCommandList コマンドリストポインタ
		void SetCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> sCommandList) {
			this->commandList_ = sCommandList;
		}

		//========================================
		/// @brief GetCommandList コマンドリストの取得
		/// \return コマンドリストポインタ
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() {
			return commandList_.Get();
		}

		//========================================
		/// @brief GetSwapChainDesc スワップチェーンの設定の取得
		/// \return スワップチェーンの設定
		DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const {
			return swapChainDesc_;
		}

		//========================================
		/// @brief GetRtvDesc RTVディスクリプタの取得
		/// @return RTVディスクリプタ
		D3D12_RENDER_TARGET_VIEW_DESC GetRtvDesc() const {
			return rtvDesc_;
		}

		//========================================
		/// @brief GetRenderTextureRtvHandle レンダーテクスチャRTV Handleの取得
		/// @param index レンダーテクスチャIndex
		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTextureRtvHandle(uint32_t index) const {
			assert(index < 2);
			return renderTextureRtvHandles_[index].cpuHandle;
		}

		/// @brief RTV Allocatorの取得
		DescriptorAllocator &GetRtvAllocator() {
			return rtvAllocator_;
		}

		/// @brief DSV Allocatorの取得
		DescriptorAllocator &GetDsvAllocator() {
			return dsvAllocator_;
		}

		/// @brief CBV/SRV/UAV Allocatorの取得
		DescriptorAllocator &GetResourceAllocator() {
			return resourceAllocator_;
		}

		/// @brief Sampler Allocatorの取得
		DescriptorAllocator &GetSamplerAllocator() {
			return samplerAllocator_;
		}

		//========================================
		/// @brief GetRenderTextureResources レンダーテクスチャリソースの取得
		/// @return
		Microsoft::WRL::ComPtr<ID3D12Resource> GetRenderTextureResource(uint32_t index) {
			return renderTextureResources_[index];
		}

		//========================================
		/// @brief GetRenderResourceIndex レンダーリソースインデックスの取得
		/// @return
		uint32_t GetRenderResourceIndex() const {
			return renderResourceIndex_;
		}

		//========================================
		/// @brief SetRenderResourceIndex レンダーリソースインデックスの設定
		/// @param index インデックス
		void SetRenderResourceIndex(uint32_t index) {
			renderResourceIndex_ = index;
		}

		//========================================
		/// @brief GetRenderTextureRootSignature レンダーテクスチャのルートシグネチャ取得
		/// @return
		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRenderTextureRootSignature() {
			return renderTextureRootSignature_;
		}

		//========================================
		/// @brief GetRenderTexturePipelineState レンダーテクスチャのパイプラインステート取得
		/// @return
		Microsoft::WRL::ComPtr<ID3D12PipelineState> GetRenderTexturePipelineState() {
			return renderTextureGraphicsPipelineState_;
		}

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
		// デバックレイヤーの生成
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;

		//========================================
		// DXGIファクトリーの生成
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
		HRESULT hr_ = S_OK; // Initialize HRESULT to S_OK

		//========================================
		// 使用するアダプタ用変数
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;

		//========================================
		// D3D12Deviceの作成
		Microsoft::WRL::ComPtr<ID3D12Device> device_;

		//========================================
		// エラー・警告の場合即停止(初期化完了のあとに行う)
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
		// アダプターの情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc_{};

		//========================================
		// コマンドキューを作成する
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

		//========================================
		// フレームバッファリング（3フレーム先読み）
		static constexpr uint32_t FRAME_BUFFER_COUNT = 3;

		//========================================
		// コマンドアロケータを生成する
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[FRAME_BUFFER_COUNT];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

		//========================================
		// スワップチェーンを生成する
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};

		//========================================
		// Fenceの生成
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr; // Initialize to nullptr

		uint64_t frameFenceValues_[FRAME_BUFFER_COUNT] = {};
		uint32_t currentFrameIndex_ = 0;

		//========================================
		// 深度バッファ
		DescriptorHandle dsvHandle_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

		//========================================
		// DescriptorAllocator
		DescriptorAllocator rtvAllocator_;
		DescriptorAllocator dsvAllocator_;
		DescriptorAllocator resourceAllocator_;
		DescriptorAllocator samplerAllocator_;

		//========================================
		// RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
		DescriptorHandle swapChainRtvHandles_[2]{};
		DescriptorHandle renderTextureRtvHandles_[2]{};
		// これから書き込むバックバッファのインデックスを取得
		UINT backBufferIndex_ = 0;
		// TransitionBarrierの設定
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
		// レンダーリソース
		uint32_t renderResourceIndex_ = 0;
		// レンダーターゲットインデックス
		uint32_t renderTargetIndex_ = 1;
		/// @brief rootSignature_ レンダーテクスチャのルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> renderTextureRootSignature_;
		/// @brief graphicsPipelineState_ レンダーテクスチャのパイプラインステート
		Microsoft::WRL::ComPtr<ID3D12PipelineState> renderTextureGraphicsPipelineState_;

		//========================================
		// グレースケール
		GrayscaleEffect grayscaleEffect_;
	};
}

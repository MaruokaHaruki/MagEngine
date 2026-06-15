/*********************************************************************
 * \file   SrvSetup.cpp
 * \brief  サーバリソースビューセットアップクラスの実装
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   SRVディスクリプタテーブル初期化・管理処理を実装
 *********************************************************************/
#include "SrvSetup.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						初期化
	void SrvSetup::Initialize(DirectXCore *dxCore) {
		// NOTE:SRVヒープはエンジン全体で共有するため、未初期化のまま進むと後段のGPU実行時に原因追跡が難しくなる
		assert(dxCore);
		//========================================
		// DXCoreの設定
		this->dxCore_ = dxCore;
		//========================================
		// ディスクリプタヒープの生成
		descriptorHeap_ = dxCore_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount_, true);
		// ディスクリプタ1個分のサイズを取得して記録
		descriptorSizeSRV_ = dxCore_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	///=============================================================================
	///						ループ前処理
	void SrvSetup::PreDraw() {
		// NOTE:描画前に必ず初期化済みヒープをバインドする前提
		assert(dxCore_);
		assert(descriptorHeap_);
		ID3D12DescriptorHeap *descriptorHeaps[] = {descriptorHeap_.Get()};
		dxCore_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
	}

	///=============================================================================
	///						指定数のSRVを追加で確保できるか
	bool SrvSetup::CanAllocate(uint32_t count) const {
		return count <= ( kMaxSRVCount_ - useIndex_ );
	}

	///=============================================================================
	///						メモリ確保
	uint32_t SrvSetup::Allocate() {
		// NOTE:SRV範囲外アクセスはGPU側の不定動作になりやすいため、確保時点で止める
		if(IsFull()) {
			assert(false);
			Logger::Log("SRV descriptor heap is full.", Logger::LogLevel::Error);
			return kInvalidSRVIndex_;
		}

		// returnする番号を一旦記録
		uint32_t index = useIndex_;
		// 次に使用するディスクリプタのインデックスを進める
		useIndex_++;
		// 上で記録した番号を返す
		return index;
	}

	///=============================================================================
	///						SRV生成(テクスチャ用)
	void SrvSetup::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource *pResource, DXGI_FORMAT format, UINT mipLevels) {
		// NOTE:無効なSRV設定はCreateShaderResourceViewでは失敗を返せないため、呼び出し前に検証する
		assert(dxCore_);
		assert(descriptorHeap_);
		assert(IsValidIndex(srvIndex));
		assert(pResource);
		assert(mipLevels > 0);
		//========================================
		// ディスクリプタハンドルの取得
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = GetSRVCPUDescriptorHandle(srvIndex);
		//========================================
		// テクスチャ用のSRVを生成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		dxCore_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, handleCPU);
	}

	///=============================================================================
	///						SRV生成(構造化バッファ用)
	void SrvSetup::CreateSRVStructuredBuffer(uint32_t srvIndex, ID3D12Resource *pResource, UINT elementQuantity, UINT structureByteStride) {
		// NOTE:構造化バッファの要素数とストライドはSRV解釈の前提なので0を許可しない
		assert(dxCore_);
		assert(IsValidIndex(srvIndex));
		assert(pResource);
		assert(elementQuantity > 0);
		assert(structureByteStride > 0);
		//========================================
		// 構造化バッファ用のSRVを生成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.StructureByteStride = structureByteStride;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = elementQuantity;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		dxCore_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetSRVCPUDescriptorHandle(srvIndex));
	}

	void SrvSetup::CreateRenderTextureSRV() {
		// NOTE:RenderTexture用SRVは予約スロット0を使う既存仕様に合わせる
		assert(dxCore_);
		assert(descriptorHeap_);
		//========================================
		// ディスクリプタハンドルの取得
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = GetSRVCPUDescriptorHandle(0);
		//========================================
		// テクスチャ用のSRVを生成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		dxCore_->GetDevice()->CreateShaderResourceView(dxCore_->GetRenderTextureResource(1).Get(), &srvDesc, handleCPU);
	}

	///=============================================================================
	///						SRVDescriptorHandleの取得を関数化
	///--------------------------------------------------------------
	///						 CPU
	D3D12_CPU_DESCRIPTOR_HANDLE SrvSetup::GetSRVCPUDescriptorHandle(uint32_t index) {
		// NOTE:ハンドル計算は範囲外でも数値上は成立するため、ここで境界を保証する
		assert(descriptorHeap_);
		assert(IsValidIndex(index));
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSizeSRV_ * index);
		return handleCPU;
	}

	///--------------------------------------------------------------
	///						 GPU
	D3D12_GPU_DESCRIPTOR_HANDLE SrvSetup::GetSRVGPUDescriptorHandle(uint32_t index) {
		// NOTE:GPUハンドルの範囲外指定はGPU実行時エラーになりやすいため、取得時点で検出する
		assert(descriptorHeap_);
		assert(IsValidIndex(index));
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
		handleGPU.ptr += (descriptorSizeSRV_ * index);
		return handleGPU;
	}

	///=============================================================================
	///						グラフィックスルートディスクリプタテーブルの設定
	void SrvSetup::SetGraphicsRootDescriptorTable(uint32_t rootParameterIndex, uint32_t srvIndex) {
		// NOTE:コマンドリストへの無効なSRVバインドは描画破綻の原因特定が難しいため、事前に検証する
		assert(dxCore_);
		assert(IsValidIndex(srvIndex));
		//========================================
		// ルートディスクリプタテーブルの設定
		dxCore_->GetCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex, GetSRVGPUDescriptorHandle(srvIndex));
	}

	///=============================================================================
	///						オフスクリーンレンダーテクスチャの生成
	void SrvSetup::CreateOffScreenTexture(uint32_t srvIndex, uint32_t rtvIndex) {
		// NOTE:レンダーテクスチャの数はDirectXCore側の固定配列に依存しているため、現状の2枚運用を明示する
		assert(dxCore_);
		assert(IsValidIndex(srvIndex));
		assert(rtvIndex < 2);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		dxCore_->GetDevice()->CreateShaderResourceView(dxCore_->GetRenderTextureResource(rtvIndex).Get(), &srvDesc, GetSRVCPUDescriptorHandle(srvIndex));
	}
}

/*********************************************************************
 * \file   TextureManager.cpp
 * \brief  テクスチャ管理
 *
 * \author Harukichimaru
 * \date   October 2024
 * \note
 *********************************************************************/
#include "TextureManager.h"
#include <algorithm>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///								初期化
	void TextureManager::Initialize(DirectXCore *dxCore, const std::string &textureDirectoryPath, SrvSetup *srvSetup) {
		//---------------------------------------
		// SRVの数と同期
		textureDatas_.reserve(SrvSetup::kMaxSRVCount_);
		//---------------------------------------
		// 引数でdxManagerを受取
		dxCore_ = dxCore;
		//---------------------------------------
		// ディレクトリパスの設定
		kTextureDirectoryPath = textureDirectoryPath;
		//---------------------------------------
		// SrvSetupの設定
		srvSetup_ = srvSetup;

		CreateRenderTextureMetaData();
	}

	///=============================================================================
	///						テクスチャファイルの読み込み
	void TextureManager::LoadTexture(const std::string &filePath) {
		// ディレクトリパスを追加
		std::string fullPath = kTextureDirectoryPath + filePath;

		//---------------------------------------
		// 読み込み済みテクスチャを検索
		if (textureDatas_.contains(fullPath)) {
			return;
		}

		//---------------------------------------
		// SRVの空きがあるかチェック
		assert(srvSetup_->IsFull() == false);

		//---------------------------------------
		// テクスチャファイルを読んでプログラムを扱えるようにする
		DirectX::ScratchImage image{};
		std::wstring filePathW = WstringUtility::ConvertString(fullPath);
		HRESULT hr;

		// ファイル拡張子をチェックして読み込み関数を選択
		// 小文字に変換してから比較
		std::string lowerPath = fullPath;
		std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

		if (lowerPath.find(".dds") != std::string::npos) {
			// DDSファイルの読み込み
			hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
			assert(SUCCEEDED(hr));

			// DDSファイルの場合、キューブマップかどうかを確認
			if (image.GetMetadata().IsCubemap()) {
				// キューブマップの場合はミップマップ生成をスキップ
			}
		} else {
			// WICファイルの読み込み
			hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
			assert(SUCCEEDED(hr));
		}

		//---------------------------------------
		// mipmapの作成
		DirectX::ScratchImage mipImages{};
		// 圧縮フォーマットまたはキューブマップの場合はミップマップ生成をスキップ
		if (DirectX::IsCompressed(image.GetMetadata().format) || image.GetMetadata().IsCubemap()) {
			mipImages = std::move(image);
		} else {
			// mipmapの生成
			hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
			assert(SUCCEEDED(hr));
		}

		//---------------------------------------
		// 追加したテクスチャデータの参照を取得する
		TextureData &textureData = textureDatas_[fullPath];

		//---------------------------------------
		// テクスチャデータの書き込
		// テクスチャメタデータの取得
		textureData.metadata = mipImages.GetMetadata();
		// テクスチャリソースの作成
		textureData.resource = dxCore_->CreateTextureResource(textureData.metadata);
		// 中間リソース
		textureData.interMediateResource = dxCore_->UploadTextureData(textureData.resource, mipImages);
		// NOTE:Texture SRVは共通Resource DescriptorAllocatorから確保する
		textureData.srvHandle = dxCore_->GetResourceAllocator().Allocate();
		assert(textureData.srvHandle.IsValid());

		//---------------------------------------
		// SRVの生成
		// metaDataを元にSRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = textureData.metadata.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// Cubemapかどうかをチェック
		if (textureData.metadata.IsCubemap()) {
			// キューブマップテクスチャ
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;		// 最も詳細なミップレベル
			srvDesc.TextureCube.MipLevels = UINT_MAX;		// ミップレベルの数
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f; // 最小LODクランプ値
		} else {
			// 通常のテクスチャ
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
			srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);
		}
		// SRVの生成
		dxCore_->GetDevice().Get()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandle.cpuHandle);
	}

	///=============================================================================
	///								終了処理
	void TextureManager::Finalize() {
		// GPU同期後に呼ばれる前提で、SRV参照元のテクスチャを明示的に解放するため。
		textureDatas_.clear();
		dxCore_ = nullptr;
		srvSetup_ = nullptr;
	}

	///=============================================================================
	///					SRVテクスチャインデックスの開始番号
	uint32_t TextureManager::GetTextureIndex(const std::string &filePath) {
		// ディレクトリパスを追加
		std::string fullPath = kTextureDirectoryPath + filePath;
		if (filePath == "RenderTexture0" || filePath == "RenderTexture1") {
			fullPath = filePath;
		}

		if (textureDatas_.contains(fullPath)) {
			return textureDatas_.at(fullPath).srvHandle.index;
		}
		//---------------------------------------
		// 検索化ヒットしない場合は停止
		assert(0);
		return 0;
	}

	///=============================================================================
	///						GPUハンドルの取得
	D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string &filePath) {
		// ディレクトリパスを追加
		std::string fullPath = kTextureDirectoryPath + filePath;
		// レンダーテクスチャの場合は特別処理
		if (filePath == "RenderTexture0" || filePath == "RenderTexture1") {
			fullPath = filePath;
		} else {
			// 通常のテクスチャの場合はディレクトリパスを追加
			fullPath = kTextureDirectoryPath + filePath;
		}
		// 範囲外指定チェック
		assert(textureDatas_.contains(fullPath));
		// テクスチャデータの参照を取得
		TextureData &textureData = textureDatas_[fullPath];
		return textureData.srvHandle.gpuHandle;
	}

	///=============================================================================
	///						CPUハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleCPU(const std::string &filePath) {
		// ディレクトリパスを追加
		std::string fullPath = kTextureDirectoryPath + filePath;
		// レンダーテクスチャの場合は特別処理
		if (filePath == "RenderTexture0" || filePath == "RenderTexture1") {
			fullPath = filePath;
		} else {
			// 通常のテクスチャの場合はディレクトリパスを追加
			fullPath = kTextureDirectoryPath + filePath;
		}
		// 範囲外指定チェック
		assert(textureDatas_.contains(fullPath));
		// テクスチャデータの参照を取得
		TextureData &textureData = textureDatas_[fullPath];
		return textureData.srvHandle.cpuHandle;
	}

	///=============================================================================
	///						メタデータの取得
	const DirectX::TexMetadata &TextureManager::GetMetadata(const std::string &filePath) {
		// ディレクトリパスを追加
		std::string fullPath = kTextureDirectoryPath + filePath;
		// 範囲外指定チェック
		assert(textureDatas_.contains(fullPath));
		// テクスチャデータの参照を取得
		TextureData &textureData = textureDatas_[fullPath];
		return textureData.metadata;
	}

	///=============================================================================
	///				レンダーテクスチャ用メタデータの作成
	void TextureManager::CreateRenderTextureMetaData() {
		TextureData &textureData1 = textureDatas_["RenderTexture0"];

		textureData1.srvHandle = dxCore_->GetResourceAllocator().Allocate();
		assert(textureData1.srvHandle.IsValid());
		srvSetup_->CreateOffScreenTexture(textureData1.srvHandle.index, 0);

		TextureData &textureData2 = textureDatas_["RenderTexture1"];

		textureData2.srvHandle = dxCore_->GetResourceAllocator().Allocate();
		assert(textureData2.srvHandle.IsValid());
		srvSetup_->CreateOffScreenTexture(textureData2.srvHandle.index, 1);
	}
}

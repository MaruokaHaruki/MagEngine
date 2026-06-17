/*********************************************************************
 * \file   TextureManager.h
 * \brief  テクスチャ管理用クラス
 *
 * \author Harukichimaru
 * \date   October 2024
 *********************************************************************/
#pragma once
#include "DirectXCore.h"
#include "SrvSetup.h"
#include <string>
#include <unordered_map>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///--------------------------------------------------------------
	///							構造体
	/**
	 * \brief テクスチャデータのセット
	 * \brief filePath ファイルパス
	 * \brief metadata メタデータ
	 * \brief resource リソース
	 * \brief srvHandle SRV Descriptor
	 */
	struct TextureData {
		DirectX::TexMetadata metadata{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		Microsoft::WRL::ComPtr<ID3D12Resource> interMediateResource;
		DescriptorHandle srvHandle{};
	};

	///=============================================================================
	///								クラス
	class TextureManager {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		TextureManager() = default;
		~TextureManager() = default;
		TextureManager(const TextureManager &) = delete;
		TextureManager &operator=(const TextureManager &) = delete;

		/// @brief Initialize 初期化
		/// @param dxManager DirectXCoreポインタ
		/// @param textureDirectoryPath テクスチャディレクトリパス
		/// @param srvSetup SrvSetupポインタ
		void Initialize(DirectXCore *dxManager, const std::string &textureDirectoryPath, SrvSetup *srvSetup);

		/// @brief LoadTexture テクスチャファイルの読み込み
		/// @param filePath ファイルパス
		void LoadTexture(const std::string &filePath);

		/// @brief Finalize 終了処理
		void Finalize();

		/// @brief GetTextureIndex テクスチャのSRVインデックスを取得
		/// @param filePath ファイルパス
		/// @return SRVインデックス
		uint32_t GetTextureIndex(const std::string &filePath);

		/// @brief GetSrvHandleGPU GPUハンドルの取得
		/// @param filePath ファイルパス
		/// @return GPUハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string &filePath);

		/// @brief GetSrvHandleCPU CPUハンドルの取得
		/// @param filePath ファイルパス
		/// @return CPUハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandleCPU(const std::string &filePath);

		/// @brief GetMetadata テクスチャのメタデータを取得
		/// @param filePath ファイルパス
		/// @return
		const DirectX::TexMetadata &GetMetadata(const std::string &filePath);

		/// @brief CreateRenderTextureMetaData レンダーテクスチャのメタデータを生成
		void CreateRenderTextureMetaData();

	private:
		//========================================
		// DirectXCoreポインタ
		DirectXCore *dxCore_ = nullptr;
		//========================================
		// テクスチャデータ
		std::unordered_map<std::string, TextureData> textureDatas_;
		//========================================
		// SRVインデックスの開始番号
		// NOTE:ImGuiが使っている番号を開けてその後ろのSRVヒープ1番から使用する
		const uint32_t kSRVIndexTop = 1;
		// SrvSetupポインタ
		SrvSetup *srvSetup_ = nullptr;
		//========================================
		// ディレクトリパス
		std::string kTextureDirectoryPath = "resources/texture";
	};
}

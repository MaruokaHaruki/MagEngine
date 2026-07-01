/*********************************************************************
 * \file   SkyboxSetup.h
 * \brief  スカイボックス共通設定クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "Camera.h"
#include "DirectXCore.h"
#include "engine/render/PipelineRecipe.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	// 前方宣言
	class DirectXCore;
	class LightManager;
	class TextureManager;
	///=============================================================================
	///						クラス
	class SkyboxSetup {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/// @brief  デフォルトコンストラクタ
		/// @param dxCore DirectXCoreポインタ
		void Initialize(DirectXCore *dxCore, TextureManager &textureManager);

		/// @brief 共通描画設定
		void CommonDrawSetup();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/// @brief ルートシグネチャの作成
		void CreateRootSignature();

		/// @brief グラフィックスパイプラインの作成
		void CreateGraphicsPipeline();

		/// @brief 所有中のRootSignatureを使ってSkybox用Recipeを作成
		PipelineRecipe CreatePipelineRecipe() const;

	public:
		/// @brief Skybox用PSO設定をRecipeとして作成
		static PipelineRecipe CreateDefaultRecipe(ID3D12RootSignature *rootSignature);

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/// @brief GetDXManager DirectXCoreの取得
		/// @return DirectXCoreポインタ
		DirectXCore *GetDXManager() const {
			return dxCore_;
		}

		TextureManager &GetTextureManager() const {
			return *textureManager_;
		}

		/// @brief SetDefaultCamera デフォルトカメラの設定
		/// @param camera カメラポインタ
		void SetDefaultCamera(Camera *camera) {
			this->defaultCamera_ = camera;
		}

		/// @brief GetDefaultCamera デフォルトカメラの取得
		/// @return カメラポインタ
		Camera *GetDefaultCamera() {
			return defaultCamera_;
		}

		/// @brief SetLightManager ライトマネージャの設定
		/// @param lightManager ライトマネージャポインタ
		void SetLightManager(LightManager *lightManager) {
			this->lightManager_ = lightManager;
		}

		/// @brief GetLightManager ライトマネージャの取得
		/// @return ライトマネージャポインタ
		LightManager *GetLightManager() const {
			return lightManager_;
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//========================================
		// DirectXCoreポインタ
		DirectXCore *dxCore_ = nullptr;
		// キューブマップはTextureManagerが所有し、Skyboxは参照だけを使う
		TextureManager *textureManager_ = nullptr;

		//========================================
		// RootSignature
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		//========================================
		// グラフィックスパイプライン
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

		//========================================
		// デフォルトカメラ
		Camera *defaultCamera_ = nullptr;

		//========================================
		// LightManagerポインタ
		LightManager *lightManager_ = nullptr;
	};
}

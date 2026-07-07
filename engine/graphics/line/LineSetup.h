/*********************************************************************
* \file   LineSetup.h
* \brief
*
* \author Harukichimaru
* \date   January 2025
* \note
*********************************************************************/
#pragma once
#include "DirectXCore.h"
#include "Camera.h"
#include "LineRenderMode.h"
#include "SrvSetup.h"
#include "engine/render/pipeline/PipelineRecipe.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///                     ラインセットアップ
	class LineSetup {
		///--------------------------------------------------------------
		///                         メンバ関数
	public:
		/**----------------------------------------------------------------------------
		* \brief  Initialize 初期化
		* \param  dxManager ダイレクトXマネージャー
		*/
		void Initialize(DirectXCore *dxCore, SrvSetup *srvSetup);

		/**----------------------------------------------------------------------------
		* \brief  CommonDrawSetup 共通描画設定
		*/
		void CommonDrawSetup(LineRenderMode renderMode = LineRenderMode::World);

		///--------------------------------------------------------------
		///                         静的メンバ関数
	private:

		/**----------------------------------------------------------------------------
		* \brief  CreateRootSignature ルートシグネチャーの作成
		*/
		void CreateRootSignature();

		/**----------------------------------------------------------------------------
		* \brief  CreateGraphicsPipeline グラフィックスパイプラインの作成
		*/
		void CreateGraphicsPipeline();

		/// @brief 所有中のRootSignatureを使ってLine用Recipeを作成
		PipelineRecipe CreateRecipe() const;

		///--------------------------------------------------------------
		///                         入出力関数
	public:
		/**----------------------------------------------------------------------------
		* \brief  GetDXManager DirectXCore取得
		* \return
		*/
		DirectXCore *GetDXManager() const { return dxCore_; }

		/**----------------------------------------------------------------------------
		* \brief  GetSrvSetup SrvSetup取得
		* \return
		*/
		SrvSetup *GetSrvSetup() const { return srvSetup_; }

		/**----------------------------------------------------------------------------
		* \brief  SetCamera デフォルトカメラの設定
		* \param  camera
		*/
		void SetDefaultCamera(Camera *camera) { this->defaultCamera_ = camera; }
		/*
		* \brief  GetCamera デフォルトカメラの取得
		* \return
		*/
		Camera *GetDefaultCamera() { return defaultCamera_; }

		/// @brief World Line用PSO設定をRecipeとして作成
		static PipelineRecipe CreateWorldPipelineRecipe(ID3D12RootSignature *rootSignature);

		/// @brief HUD Line用PSO設定をRecipeとして作成
		static PipelineRecipe CreateHudPipelineRecipe(ID3D12RootSignature *rootSignature);

		/// @brief 互換維持用の既定Recipe
		static PipelineRecipe CreateDefaultRecipe(ID3D12RootSignature *rootSignature) {
			return CreateWorldPipelineRecipe(rootSignature);
		}


		///--------------------------------------------------------------
		///                         メンバ変数
	private:
		//========================================
		// DirectXCoreポインタ
		DirectXCore *dxCore_ = nullptr;
		//SrvSetupポインタ
		SrvSetup *srvSetup_ = nullptr;

		//========================================
		// RootSignature
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		//========================================
		// グラフィックスパイプライン
		Microsoft::WRL::ComPtr<ID3D12PipelineState> worldPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> hudPipelineState_;

		//========================================
		// デフォルトカメラ
		Camera *defaultCamera_ = nullptr;
	};

}

/*********************************************************************
 * \file   ParticleSetup.h
 * \brief
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#pragma once
#include "Camera.h"
#include "DirectXCore.h"
#include "SrvSetup.h"
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
	class ParticleSetup {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  Initialize 初期化
		 * \param  dxManager ダイレクトXマネージャー
		 */
		void Initialize(DirectXCore *dxCore, SrvSetup *srvSetup);

		/**----------------------------------------------------------------------------
		 * \brief  CommonDrawSetup 共通描画設定
		 */
		void CommonDrawSetup();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  CreateRootSignature ルートシグネチャーの作成
		 */
		void CreateRootSignature();

		/**----------------------------------------------------------------------------
		 * \brief  CreateGraphicsPipeline グラフィックスパイプラインの作成
		 */
		void CreateGraphicsPipeline();

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  GetDXManager DirectXCore取得
		 * \return
		 */
		DirectXCore *GetDXManager() const {
			return dxCore_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetSrvSetup SrvSetup取得
		 * \return
		 */
		SrvSetup *GetSrvSetup() const {
			return srvSetup_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetCamera デフォルトカメラの設定
		 * \param  camera
		 */
		void SetDefaultCamera(Camera *camera) {
			this->defaultCamera_ = camera;
		}
		/*
		 * \brief  GetCamera デフォルトカメラの取得
		 * \return
		 */
		Camera *GetDefaultCamera() {
			return defaultCamera_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetRootSignature ルートシグネチャの取得
		 * \return
		 */
		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const {
			return rootSignature_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetGraphicsPipelineState グラフィックスパイプラインステートの取得
		 * \return
		 */
		Microsoft::WRL::ComPtr<ID3D12PipelineState> GetGraphicsPipelineState() const {
			return graphicsPipelineState_;
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//========================================
		// DirectXCoreポインタ
		DirectXCore *dxCore_ = nullptr;
		// SrvSetupポインタ
		SrvSetup *srvSetup_ = nullptr;

		//========================================
		// RootSignature
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		//========================================
		// グラフィックスパイプライン
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

		//========================================
		// デフォルトカメラ
		Camera *defaultCamera_ = nullptr;
	};
}
/*********************************************************************
 * \file   Object3dSetup.h
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "Camera.h"
#include "DirectXCore.h"
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						クラス
	class LightManager;
	class Object3dSetup {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/// @brief  デフォルトコンストラクタ
		/// @param dxCore DirectXCoreポインタ
		void Initialize(DirectXCore *dxCore);

	/// @brief 共通描画設定
	/// COMMENT: GPU の描画状態切り替えを最小化。複数オブジェクトの連続描画効率を向上
	void CommonDrawSetup();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/// @brief ルートシグネチャの作成
		void CreateRootSignature();

		/// @brief グラフィックスパイプラインの作成
		void CreateGraphicsPipeline();

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/// @brief GetDXManager DirectXCoreの取得
		/// @return DirectXCoreポインタ
		DirectXCore *GetDXManager() const {
			return dxCore_;
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
		LightManager *GetLightManager() {
			return lightManager_;
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//========================================
		// DirectXCoreポインタ
		/// COMMENT: GPU リソースとコマンドリスト管理
		DirectXCore *dxCore_ = nullptr;

		//=======================================
		// LightManagerポインタ
		/// COMMENT: ライティング情報を複数オブジェクトで共有（バッチ処理最適化）
		LightManager *lightManager_ = nullptr;

		//========================================
		// RootSignature
		/// COMMENT: GPU パイプラインの設定。複数フレーム間で再利用
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		//========================================
		// グラフィックスパイプライン
		/// COMMENT: PSO キャッシング。ドロー呼び出し前の状態設定を削減
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

		//========================================
		// デフォルトカメラ
		/// COMMENT: カメラバッファを複数オブジェクト間で共有（メモリ最適化）
		Camera *defaultCamera_ = nullptr;
	};
}

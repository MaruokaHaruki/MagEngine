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

///=============================================================================
///						クラス
class SkyboxSetup {
	///--------------------------------------------------------------
	///						 メンバ関数
public:
	/// @brief  デフォルトコンストラクタ
	/// @param dxCore DirectXCoreポインタ
	void Initialize(DirectXCore *dxCore);

	/// @brief 共通描画設定
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

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// DirectXCoreポインタ
	DirectXCore *dxCore_ = nullptr;

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

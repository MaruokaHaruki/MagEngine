/*********************************************************************
 * \file   OffscreenRendering.h
 * \brief
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#pragma once
#include "DirectXCore.h"
#include "SrvSetup.h"

/// @brief オフスクリーンレンダリングクラス
class OffscreenRendering {

	///--------------------------------------------------------------
	///							メンバ関数
public:

	/// \brief 初期化
	void Initialize(DirectXCore *dxCore, SrvSetup *srvSetup);

	/// \brief 共通描画設定
	void CommonDrawSetup();

    /// \brief レンダーテクスチャをSwapChainに描画
    void DrawToSwapChain();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:

	/// @brief ルートシグネチャーの作成
	void CreateRootSignature();

	/// @brief グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

	///--------------------------------------------------------------
	///							入出力関数
public:


	///--------------------------------------------------------------
	///							メンバ変数
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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
};

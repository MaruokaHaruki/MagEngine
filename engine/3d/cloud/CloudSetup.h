/*********************************************************************
 * \file   CloudSetup.h
 * \brief  雲描画のセットアップクラス
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   雲レンダリング用のルートシグネチャとパイプライン設定を管理
 *********************************************************************/
#pragma once
#include <d3d12.h>
#include <wrl/client.h>

///=============================================================================
///						前方宣言
class DirectXCore;

///=============================================================================
///						クラス
class CloudSetup {
	///--------------------------------------------------------------
	///						 メンバ関数
public:
	/// @brief 初期化
	/// @param dxCore DirectXCoreポインタ
	void Initialize(DirectXCore *dxCore);

	/// @brief 共通描画設定
	void CommonDrawSetup();

	///--------------------------------------------------------------
	///						 入出力関数
public:
	/// @brief GetDXCore DirectXCoreの取得
	/// @return DirectXCoreポインタ
	DirectXCore *GetDXCore() const {
		return dxCore_;
	}

	///--------------------------------------------------------------
	///						 静的メンバ関数
private:
	/// @brief ルートシグネチャの作成
	void CreateRootSignature();

	/// @brief グラフィックスパイプラインの作成
	void CreateGraphicsPipeline();

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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};

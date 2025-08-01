/*********************************************************************
 * \file   FullscreenPassRendere.h
 * \brief  フルスクリーンエフェクトクラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#pragma once
//========================================
// 標準ライブラリ
#include <string>
//========================================
// DX12 include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class DirectXCore;
class FullscreenPassRendere {

	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(DirectXCore *dxCore);

	/// \brief 前描画処理
	void PreDraw();

	/// \brief 後描画処理
	void PostDraw();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	void CreatePipeline();

	void CreateRootSignature();

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
	// Shaderパス
	// 頂点シェーダーのパス
	std::wstring vertexShaderPath_;
	// ピクセルシェーダーのパス
	std::wstring pixelShaderPath_;
};

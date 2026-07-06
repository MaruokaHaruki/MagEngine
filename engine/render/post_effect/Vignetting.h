/*********************************************************************
 * \file   Vignetting.h
 * \brief  ビネットエフェクトクラス
 *
 * \author Harukichimaru
 * \date   January 2026
 *********************************************************************/
#pragma once
#include "PostEffectParameterSet.h"
#include "engine/render/pipeline/PipelineRecipe.h"
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
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	class DirectXCore;
	class Vignetting {

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

		/// \brief 所有中のRootSignatureとShader PathからRecipeを作成
		PipelineRecipe CreatePipelineRecipe() const;

		/// \brief Vignetting用PSO設定をRecipeとして作成
		static PipelineRecipe CreateDefaultRecipe(ID3D12RootSignature *rootSignature, const std::wstring &vertexShaderPath, const std::wstring &pixelShaderPath);

		/// \brief Vignetting用Root Parameter配置を取得
		static PostEffectBindingLayout CreateBindingLayout();

		/// \brief 所有RootSignatureと一致するRoot Parameter配置を取得
		PostEffectBindingLayout GetBindingLayout() const;

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
		std::wstring vertexShaderPath_;
		std::wstring pixelShaderPath_;
	};

	inline PostEffectBindingLayout Vignetting::CreateBindingLayout() {
		// NOTE: Grayscaleと同じSRV(t0)のみのRootSignatureを使うため、番号の意味だけEffect側に明示する。
		return PostEffectBindingLayout{0, PostEffectBindingLayout::kInvalidRootParameter};
	}

	inline PostEffectBindingLayout Vignetting::GetBindingLayout() const {
		return CreateBindingLayout();
	}
}

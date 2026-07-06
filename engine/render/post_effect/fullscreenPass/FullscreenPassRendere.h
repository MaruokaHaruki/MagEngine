/*********************************************************************
 * \file   FullscreenPassRendere.h
 * \brief  フルスクリーンエフェクトクラス
 *
 * \author Harukichimaru
 * \date   July 2025
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

		/// \brief 所有中のRootSignatureとShader PathからRecipeを作成
		PipelineRecipe CreatePipelineRecipe() const;

		/// \brief Fullscreen描画用PSO設定をRecipeとして作成
		static PipelineRecipe CreateDefaultRecipe(ID3D12RootSignature *rootSignature, const std::wstring &vertexShaderPath, const std::wstring &pixelShaderPath);

		/// \brief Fullscreen描画用Root Parameter配置を取得
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
		// 頂点シェーダーのパス
		std::wstring vertexShaderPath_;
		// ピクセルシェーダーのパス
		std::wstring pixelShaderPath_;
	};

	inline PostEffectBindingLayout FullscreenPassRendere::CreateBindingLayout() {
		// NOTE: Fullscreen描画の入力TextureもRootParameter 0のSRVとして扱う既存仕様を明示する。
		return PostEffectBindingLayout{0, PostEffectBindingLayout::kInvalidRootParameter};
	}

	inline PostEffectBindingLayout FullscreenPassRendere::GetBindingLayout() const {
		return CreateBindingLayout();
	}
}

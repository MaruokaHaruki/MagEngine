/*********************************************************************
 * \file   SpriteSetup.h
 * \brief  スプライト管理クラス - スプライト描画システムの共通設定
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note   全てのSpriteインスタンスで共有される描画設定を管理
 *         グラフィックスパイプライン・ルートシグネチャの生成を行う
 *********************************************************************/
#pragma once
///=============================================================================
///						インクルード
#include "DirectXCore.h"
#include "SpriteRenderMode.h"
#include "engine/render/pipeline/PipelineRecipe.h"
#include "engine/math/structure/graphics/Light.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	class TextureManager;

	struct SpriteRootParameterBinding {
		static constexpr uint32_t kMaterial = 0;
		static constexpr uint32_t kTransformation = 1;
		static constexpr uint32_t kTexture = 2;
		static constexpr uint32_t kDirectionalLight = 3;
	};

	struct SpriteDrawBinding {
		D3D12_GPU_VIRTUAL_ADDRESS material = 0;
		D3D12_GPU_VIRTUAL_ADDRESS transformation = 0;
		D3D12_GPU_DESCRIPTOR_HANDLE texture{};
		D3D12_GPU_VIRTUAL_ADDRESS directionalLight = 0;

		bool IsValid() const {
			return material != 0 &&
				   transformation != 0 &&
				   texture.ptr != 0 &&
				   directionalLight != 0;
		}
	};

	///=============================================================================
	///                            SpriteSetupクラス
	/**
	 * \class SpriteSetup
	 * \brief スプライト描画システムの共通設定管理クラス
	 * 
	 * \note  【役割】
	 *        - グラフィックスパイプラインの作成と管理
	 *        - ルートシグネチャの作成と管理
	 *        - 全スプライト共通の描画設定
	 * 
	 * \note  【使用方法】
	 *        1. Initialize()でDirectXCoreを登録
	 *        2. 描画前にCommonDrawSetup()を呼び出す
	 *        3. 各Spriteのdraw()を呼び出す
	 */
	class SpriteSetup {
		///--------------------------------------------------------------
		///						 公開メンバ関数
	public:
		///=============================================================================
		///                        初期化

		/**----------------------------------------------------------------------------
		 * \brief  Initialize 初期化
		 * \param  dxCore DirectXCoreポインタ
		 * \note   スプライト描画システムを初期化し、
		 *         グラフィックスパイプラインを作成する
		 */
		void Initialize(DirectXCore *dxCore, TextureManager &textureManager);

		///=============================================================================
		///                        描画設定

		/**----------------------------------------------------------------------------
		 * \brief  CommonDrawSetup 共通描画設定
		 * \note   全てのスプライトを描画する前に1度だけ呼び出す
		 *         ルートシグネチャとパイプラインステートを設定する
		 * 
		 * \code
		 *   spriteSetup->CommonDrawSetup();
		 *   sprite1->Draw();
		 *   sprite2->Draw();
		 *   // ...
		 * \endcode
		 */
		void CommonDrawSetup(SpriteRenderMode renderMode = SpriteRenderMode::Ui);

		///=============================================================================
		///                        アクセッサ

		/**----------------------------------------------------------------------------
		 * \brief  GetDXManager DirectXCoreの取得
		 * \return DirectXCoreポインタ
		 * \note   Spriteクラスから使用される
		 */
		DirectXCore *GetDXManager() const {
			return dxCore_;
		}

		TextureManager &GetTextureManager() const {
			return *textureManager_;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetDirectionalLightGPUVirtualAddress() const {
			return directionalLightBuffer_ ? directionalLightBuffer_->GetGPUVirtualAddress() : 0;
		}

		void SetDirectionalLight(const MagMath::DirectionalLight &directionalLight);

		/// @brief World Sprite用PSO設定をRecipeとして作成
		static PipelineRecipe CreateWorldPipelineRecipe(ID3D12RootSignature *rootSignature);

		/// @brief UI Sprite用PSO設定をRecipeとして作成
		static PipelineRecipe CreateUiPipelineRecipe(ID3D12RootSignature *rootSignature);

		/// @brief 既存テスト互換用。World Spriteの既存Depth契約を返す
		static PipelineRecipe CreateDefaultRecipe(ID3D12RootSignature *rootSignature);

		///--------------------------------------------------------------
		///						 プライベート関数
	private:
		///=============================================================================
		///                        内部処理

		/**----------------------------------------------------------------------------
		 * \brief  CreateRootSignature ルートシグネチャの作成
		 * \note   シェーダーリソースのバインド方法を定義
		 *         - Material (CBV)
		 *         - TransformationMatrix (CBV)
		 *         - Texture (SRV)
		 *         - Sampler
		 */
		void CreateRootSignature();

		/**----------------------------------------------------------------------------
		 * \brief  CreateGraphicsPipeline グラフィックスパイプラインの作成
		 * \note   描画パイプライン全体の設定を行う
		 *         - シェーダーのコンパイルと登録
		 *         - 入力レイアウトの設定
		 *         - ブレンドステートの設定（アルファブレンディング）
		 *         - ラスタライザーステートの設定
		 *         - デプスステンシルステートの設定
		 */
		void CreateGraphicsPipeline();
		void CreateDirectionalLightBuffer();

		/// @brief 所有中のRootSignatureを使ってSprite用Recipeを作成
		PipelineRecipe CreateWorldRecipe() const;
		PipelineRecipe CreateUiRecipe() const;

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		///---------------------------------------
		/// DirectXCore
		DirectXCore *dxCore_ = nullptr;	  // DirectXCoreへのポインタ

		/// COMMENT: Spriteはテクスチャの寿命を所有せず、Framework所有の管理器を参照する
		TextureManager *textureManager_ = nullptr;

		///---------------------------------------
		/// パイプラインステート
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;			  // ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> worldPipelineState_;	  // World Sprite用グラフィックスパイプライン
		Microsoft::WRL::ComPtr<ID3D12PipelineState> uiPipelineState_;		  // UI Sprite用グラフィックスパイプライン

		/// COMMENT: Sprite shaderはenableLightingに関係なくb1を参照可能な契約なので、有効なCBVを常時保持する。
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_;
		MagMath::DirectionalLight *directionalLightData_ = nullptr;
	};
}

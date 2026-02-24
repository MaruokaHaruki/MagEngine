/*********************************************************************
 * \file   Skybox.h
 * \brief  スカイボックス描画クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "Matrix4x4.h"
#include "TextureManager.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "Vector4.h"
//========================================
// DX12include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	// 頂点データ構造体
	struct SkyboxVertex {
		MagMath::Vector4 position;
	};

	// 前方宣言
	class SkyboxSetup;
	class Camera;
	class LightManager;
	///=============================================================================
	///								クラス
	class Skybox {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		/// \brief 初期化
		void Initialize(SkyboxSetup *skyboxSetup);

		/// \brief 更新
		void Update();

		/// \brief 描画
		void Draw();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/// \brief キューブの頂点データ作成
		void CreateCubeVertices();

		/// @brief トランスフォーメーションマトリックスバッファの作成
		void CreateTransformationMatrixBuffer();

		/// \brief 並行光源の作成
		void CreateDirectionalLight();

		/// \brief ポイントライトの作成
		void CreatePointLight();

		/// \brief スポットライトの作成
		void CreateSpotLight();

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/// \brief SetCamera カメラの設定
		void SetCamera(Camera *camera) {
			this->camera_ = camera;
		}

		/// \brief SetTexture テクスチャの設定
		void SetTexture(const std::string &texturePath) {
			texturePath_ = texturePath;
		}

		/// \brief GetTransform トランスフォーメーションのポインタを取得
		MagMath::Transform *GetTransform() {
			return &transform_;
		}

		/// \brief SetTransform トランスフォーメーションの設定
		void SetTransform(const MagMath::Transform &transform) {
			transform_ = transform;
		}

		/// \brief SetScale スケールの設定
		void SetScale(const MagMath::Vector3 &scale) {
			transform_.scale = scale;
		}

		/// \brief GetScale スケールの取得
		const MagMath::Vector3 &GetScale() const {
			return transform_.scale;
		}

		/// \brief SetRotation 回転の設定
		void SetRotation(const MagMath::Vector3 &rotate) {
			transform_.rotate = rotate;
		}

		/// \brief GetRotation 回転の取得
		const MagMath::Vector3 &GetRotation() const {
			return transform_.rotate;
		}

		/// \brief SetPosition 位置の設定
		void SetPosition(const MagMath::Vector3 &translate) {
			transform_.translate = translate;
		}

		/// \brief GetPosition 位置の取得
		const MagMath::Vector3 &GetPosition() const {
			return transform_.translate;
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//========================================
		// SkyboxSetupポインタ
		SkyboxSetup *skyboxSetup_ = nullptr;

		//========================================
		// トランスフォーメーションマトリックス
		Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixBuffer_;
		// 並行光源バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_;
		// ポイントライトバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> pointLightBuffer_;
		// スポットライトバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> spotLightBuffer_;
		// 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		// インデックスバッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;

		//========================================
		// バッファリソース内のデータを指すポインタ
		// トランスフォーメーションマトリックス
		MagMath::TransformationMatrix *transformationMatrixData_ = nullptr;
		// 並行光源データ
		MagMath::DirectionalLight *directionalLightData_ = nullptr;
		// ポイントライトデータ
		MagMath::PointLight *pointLightData_ = nullptr;
		// スポットライトデータ
		MagMath::SpotLight *spotLightData_ = nullptr;
		// 頂点バッファビュー
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		// インデックスバッファビュー
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

		//---------------------------------------
		// 頂点データとインデックスデータ
		std::vector<SkyboxVertex> vertices_;
		std::vector<uint32_t> indices_;

		//========================================
		// カメラ
		Camera *camera_ = nullptr;

		//========================================
		// テクスチャパス
		std::string texturePath_ = "";

		//========================================
		// Transform
		MagMath::Transform transform_ = {};
	};
}
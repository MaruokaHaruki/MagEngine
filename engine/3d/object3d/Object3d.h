/*********************************************************************
 * \file   Object3d.h
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "Model.h"
#include "ModelManager.h"
//========================================
// DX12include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	struct CameraForGpu {
		MagMath::Vector3 worldPosition;
	};

	class Object3dSetup;
	class Camera;
	class Object3d {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		/// \brief 初期化
		void Initialize(Object3dSetup *object3dSetup);

		/// \brief 更新
		void Update();

		/// \brief 描画
		void Draw();

		/// \brief ImGui描画
		void ChangeTexture(const std::string &texturePath);

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  トランスフォーメーションマトリックスバッファの作成
		 */
		void CreateTransformationMatrixBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  並行光源の作成
		 */
		void CreateDirectionalLight();

		/**----------------------------------------------------------------------------
		 * \brief  CreatePointLight ポイントライトの作成
		 */
		void CreatePointLight();

		/**----------------------------------------------------------------------------
		 * \brief  CreateSpotLight スポットライトの作成
		 */
		void CreateSpotLight();

		/**----------------------------------------------------------------------------
		 * \brief  カメラバッファの作成
		 */
		void CreateCameraBuffer();

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  SetModel モデルの設定
		 * \param  filePath ファイルパス
		 */
		void SetModel(const std::string &filePath) {
			model_ = ModelManager::GetInstance()->FindModel(filePath);
		}

		MagMath::Transform *GetTransform() {
			return &transform_;
		} // Transformのポインタを取得する関数
		/**----------------------------------------------------------------------------
		 * \brief  SetTransform トランスフォーメーションの設定
		 * \param  transform トランスフォーメーション
		 */
		void SetTransform(const MagMath::Transform &transform) {
			transform_ = transform;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetModel モデルの設定
		 * \param  model モデル
		 */
		void SetScale(const MagMath::Vector3 &scale) {
			transform_.scale = scale;
		}
		/*
		 * \brief  GetScale スケールの取得
		 * \return MagMath::Vector3 スケール
		 */
		const MagMath::Vector3 &GetScale() const {
			return transform_.scale;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetRotate 回転の設定
		 * \param  rotate 回転
		 */
		void SetRotation(const MagMath::Vector3 &rotate) {
			transform_.rotate = rotate;
		}
		/*
		 * \brief  GetRotate 回転の取得
		 * \return MagMath::Vector3 回転
		 */
		const MagMath::Vector3 &GetRotation() const {
			return transform_.rotate;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetTranslate 移動の設定
		 * \param  translate 移動
		 */
		void SetPosition(const MagMath::Vector3 &translate) {
			transform_.translate = translate;
		}
		/*
		 * \brief  GetTranslate 移動の取得
		 * \return MagMath::Vector3 移動
		 */
		const MagMath::Vector3 &GetPosition() const {
			return transform_.translate;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetCamera カメラの設定
		 * \param  camera
		 */
		void SetCamera(Camera *camera) {
			this->camera_ = camera;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetDirectionalLight 並行光源の設定
		 * \param  color
		 * \param  direction
		 * \param  intensity
		 */
		void SetDirectionalLight(const MagMath::Vector4 &color, const MagMath::Vector3 &direction, float intensity) {
			directionalLightData_->color = color;
			directionalLightData_->direction = direction;
			directionalLightData_->intensity = intensity;
		}
		/*
		 * \brief  GetDirectionalLight 並行光源の取得
		 */
		const MagMath::DirectionalLight &GetDirectionalLight() const {
			return *directionalLightData_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetPointLight ポイントライトの設定（拡張版）
		 * \param  color ライトの色
		 * \param  position ライトの位置
		 * \param  intensity 光の強度
		 * \param  radius ライトの影響範囲
		 * \param  decay 減衰の度合い（1.0が線形、2.0が二乗減衰）
		 */
		void SetPointLight(const MagMath::Vector4 &color, const MagMath::Vector3 &position, float intensity,
			float radius = 10.0f, float decay = 2.0f) {
			pointLightData_->color = color;
			pointLightData_->position = position;
			pointLightData_->intensity = intensity;
			pointLightData_->radius = radius;
			pointLightData_->decay = decay;
		}
		/*
		 * \brief  GetPointLight ポイントライトの取得
		 * \return 現在設定されているポイントライト情報
		 */
		const MagMath::PointLight &GetPointLight() const {
			return *pointLightData_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetSpotLight スポットライトの設定
		 * \param  color ライトの色
		 * \param  position ライトの位置
		 * \param  direction ライトの方向
		 * \param  intensity 光の強度
		 * \param  distance 光の届く距離
		 * \param  decay 減衰の度合い
		 * \param  angle スポットライトの角度（ラジアン）
		 */
		void SetSpotLight(const MagMath::Vector4 &color, const MagMath::Vector3 &position,
			const MagMath::Vector3 &direction, float intensity,
			float distance = 15.0f, float decay = 2.0f,
			float angle = 0.5f) {
			spotLightData_->color = color;
			spotLightData_->position = position;
			spotLightData_->direction = direction;
			spotLightData_->intensity = intensity;
			spotLightData_->distance = distance;
			spotLightData_->decay = decay;
			spotLightData_->cosAngle = cosf(angle);
		}
		/*
		 * \brief  GetSpotLight スポットライトの取得
		 * \return 現在設定されているスポットライト情報
		 */
		const MagMath::SpotLight &GetSpotLight() const {
			return *spotLightData_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetMaterialColor マテリアルカラーの設定
		 * \param  color
		 */
		void SetMaterialColor(const MagMath::Vector4 &color) {
			model_->SetMaterialColor(color);
		}
		/*
		 * \brief  GetMaterialColor マテリアルカラーの取得
		 */
		MagMath::Vector4 GetMaterialColor() const {
			return model_->GetMaterialColor();
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetShininess 光沢度の設定
		 * \param  shininess
		 */
		void SetShininess(float shininess) {
			model_->SetShininess(shininess);
		}
		/*
		 * \brief  GetShininess 光沢度の取得
		 */
		float GetShininess() const {
			return model_->GetShininess();
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetEnvironmentMapEnabled 環境マップの有効/無効設定
		 * \param  enabled 有効フラグ
		 */
		void SetEnvironmentMapEnabled(bool enabled) {
			model_->SetEnvironmentMapEnabled(enabled);
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetEnvironmentMapEnabled 環境マップの有効/無効取得
		 * \return
		 */
		bool GetEnvironmentMapEnabled() const {
			return model_->GetEnvironmentMapEnabled();
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetEnvironmentMapStrength 環境マップの強度設定
		 * \param  strength 強度
		 */
		void SetEnvironmentMapStrength(float strength) {
			model_->SetEnvironmentMapStrength(strength);
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetEnvironmentMapStrength 環境マップの強度取得
		 * \return
		 */
		float GetEnvironmentMapStrength() const {
			return model_->GetEnvironmentMapStrength();
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//========================================
		// オブジェクト3Dセットアップポインタ
		Object3dSetup *object3dSetup_ = nullptr;
		//========================================
		// モデルデータ
		Model *model_ = nullptr;
		//========================================
		// トランスフォーメーションマトリックス
		Microsoft::WRL::ComPtr<ID3D12Resource> transfomationMatrixBuffer_;
		// カメラ
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraBuffer_;
		// 並行光源
		Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightBuffer_;
		// 点光源
		Microsoft::WRL::ComPtr<ID3D12Resource> pointLightBuffer_;
		// TODO:スポットライト
		Microsoft::WRL::ComPtr<ID3D12Resource> spotLightBuffer_;
		//========================================
		// バッファリソース内のデータを指すポインタ
		// トランスフォーメーションマトリックス
		MagMath::TransformationMatrix *transformationMatrixData_ = nullptr;
		// カメラ
		CameraForGpu *cameraData_ = nullptr;
		// 並行光源
		MagMath::DirectionalLight *directionalLightData_ = nullptr;
		// 点光源
		MagMath::PointLight *pointLightData_ = nullptr;
		// TODO:スポットライト
		MagMath::SpotLight *spotLightData_ = nullptr;
		//========================================
		// Transform
		MagMath::Transform transform_ = {};
		//========================================
		// カメラ
		Camera *camera_ = nullptr;
	};
}
/*********************************************************************
 * \file   Cloud.h
 * \brief  雲描画クラス
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note   ボリュメトリック雲のレンダリングを管理
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						前方宣言
	class Camera;
	class CloudSetup;

	///=============================================================================
	///						構造体

	/**----------------------------------------------------------------------------
	 * \brief  CloudCameraConstant カメラ定数バッファ（GPU用）
	 * \note   シェーダーに渡すカメラ情報
	 */
	struct alignas( 16 ) CloudCameraConstant {
		MagMath::Matrix4x4 invViewProj;	   // 逆ビュープロジェクション行列（レイ方向計算用）
		MagMath::Vector3 cameraPosition;	   // カメラのワールド座標
		float padding = 0.0f;	   // パディング
		float nearPlane = 0.1f;	   // ニアプレーン
		float farPlane = 10000.0f; // ファープレーン
		float padding2 = 0.0f;	   // パディング
		float padding3 = 0.0f;	   // パディング
		MagMath::Matrix4x4 viewProj;		   // ビュープロジェクション行列（深度値計算用）
	};

	/**----------------------------------------------------------------------------
	 * \brief  CloudRenderParams 雲レンダリングパラメータ（GPU用）
	 * \note   雲の見た目を制御するパラメータ群
	 */
	struct alignas( 16 ) CloudRenderParams {
		//========================================
		// 雲の位置とサイズ
		MagMath::Vector3 cloudCenter{ 0.0f, 150.0f, 0.0f }; // 雲の中心座標
		float cloudSizeX = 300.0f;				 // 未使用（構造体パディング用）

		MagMath::Vector3 cloudSize{ 300.0f, 100.0f, 300.0f }; // 雲のXYZサイズ
		float padding0 = 0.0f;					   // パディング

		//========================================
		// ライティング
		MagMath::Vector3 sunDirection{ 0.3f, 0.8f, 0.5f }; // 太陽光の方向
		float sunIntensity = 1.2f;				// 太陽光の強度

		MagMath::Vector3 sunColor{ 1.0f, 0.96f, 0.88f }; // 太陽光の色
		float ambient = 0.3f;				  // 環境光の強度

		//========================================
		// 雲の密度とノイズ
		float density = 1.0f;			 // 雲の密度
		float coverage = 0.5f;			 // 雲のカバレッジ（雲量）
		float baseNoiseScale = 0.003f;	 // ベースノイズのスケール
		float detailNoiseScale = 0.015f; // ディテールノイズのスケール

		//========================================
		// レイマーチング設定
		float stepSize = 3.0f;				  // レイマーチングのステップサイズ
		float maxDistance = 2000.0f;		  // 最大レイマーチング距離
		float lightStepSize = 15.0f;		  // ライトサンプリングのステップサイズ
		float shadowDensityMultiplier = 1.2f; // 影の密度倍率

		//========================================
		// アニメーション
		float time = 0.0f;		   // 経過時間（アニメーション用）
		float noiseSpeed = 0.05f;  // ノイズのアニメーション速度
		float detailWeight = 0.4f; // ディテールノイズの重み
		float anisotropy = 0.6f;   // 異方性パラメータ

		//========================================
		// デバッグ
		float debugFlag = 0.0f; // デバッグフラグ
		float padding1 = 0.0f;	// パディング
		float padding2 = 0.0f;	// パディング
		float padding3 = 0.0f;	// パディング
	};

	///=============================================================================
	///						クラス
	class Cloud {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		/// @brief 初期化
		/// @param setup CloudSetupポインタ
		void Initialize(CloudSetup *setup);

		/// @brief 更新
		/// @param camera カメラ参照
		/// @param deltaTime デルタタイム
		void Update(const Camera &camera, float deltaTime);

		/// @brief 描画
		void Draw();

		/// @brief ImGui描画
		void DrawImGui();

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  フルスクリーン頂点バッファの作成
		 * \note   画面全体を覆う三角形の頂点データを作成
		 */
		void CreateFullscreenVertexBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  定数バッファの作成
		 * \note   カメラとパラメータ用の定数バッファを作成
		 */
		void CreateConstantBuffers();

		/**----------------------------------------------------------------------------
		 * \brief  雲パラメータの更新
		 * \note   Transformから雲の位置情報を更新
		 */
		void UpdateCloudParams();

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  GetTransform Transformの取得
		 * \return Transform参照
		 */
		MagMath::Transform &GetTransform() {
			return transform_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetTransform Transformの取得（const版）
		 * \return Transform参照
		 */
		const MagMath::Transform &GetTransform() const {
			return transform_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetPosition 位置の設定
		 * \param  pos 位置
		 */
		void SetPosition(const MagMath::Vector3 &pos);

		/**----------------------------------------------------------------------------
		 * \brief  SetScale スケールの設定
		 * \param  scale スケール
		 */
		void SetScale(const MagMath::Vector3 &scale);

		/**----------------------------------------------------------------------------
		 * \brief  SetSize サイズの設定
		 * \param  size サイズ
		 */
		void SetSize(const MagMath::Vector3 &size) {
			paramsCPU_.cloudSize = size;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetMutableParams パラメータの取得（可変）
		 * \return CloudRenderParams参照
		 */
		CloudRenderParams &GetMutableParams() {
			return paramsCPU_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetParams パラメータの取得（const）
		 * \return CloudRenderParams参照
		 */
		const CloudRenderParams &GetParams() const {
			return paramsCPU_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetWeatherMap ウェザーマップの設定
		 * \param  srv SRVハンドル
		 */
		void SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv);

		/**----------------------------------------------------------------------------
		 * \brief  SetEnabled 有効/無効の設定
		 * \param  enabled 有効フラグ
		 */
		void SetEnabled(bool enabled) {
			enabled_ = enabled;
		}

		/**----------------------------------------------------------------------------
		 * \brief  IsEnabled 有効/無効の取得
		 * \return 有効フラグ
		 */
		bool IsEnabled() const {
			return enabled_;
		}

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  FullscreenVertex フルスクリーン頂点構造体
		 * \note   画面全体を覆う三角形用の頂点データ
		 */
		struct FullscreenVertex {
			float position[3]; // 頂点座標
			float uv[2];	   // テクスチャ座標
		};

		//========================================
		// CloudSetupポインタ
		CloudSetup *setup_ = nullptr;

		//========================================
		// Transform（雲の位置・回転・スケール）
		MagMath::Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 150.0f, 0.0f} };

		//========================================
		// 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		//========================================
		// 定数バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraCB_; // カメラ用
		Microsoft::WRL::ComPtr<ID3D12Resource> paramsCB_; // パラメータ用

		//========================================
		// バッファリソース内のデータを指すポインタ
		CloudCameraConstant *cameraData_ = nullptr; // カメラデータ
		CloudRenderParams *paramsData_ = nullptr;	// パラメータデータ
		CloudRenderParams paramsCPU_;				// CPU側パラメータ

		//========================================
		// ウェザーマップ
		D3D12_GPU_DESCRIPTOR_HANDLE weatherMapSrv_{}; // ウェザーマップSRV
		bool hasWeatherMapSrv_ = false;				  // ウェザーマップ有効フラグ

		//========================================
		// その他
		bool enabled_ = true;		   // 描画有効フラグ
		float accumulatedTime_ = 0.0f; // 累積時間
	};
}
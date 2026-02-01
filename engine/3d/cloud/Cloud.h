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
	struct alignas(16) CloudCameraConstant {
		MagMath::Matrix4x4 invViewProj;	 // 逆ビュープロジェクション行列（レイ方向計算用）
		MagMath::Vector3 cameraPosition; // カメラのワールド座標
		float padding = 0.0f;			 // パディング
		float nearPlane = 0.1f;			 // ニアプレーン
		float farPlane = 10000.0f;		 // ファープレーン
		float padding2 = 0.0f;			 // パディング
		float padding3 = 0.0f;			 // パディング
		MagMath::Matrix4x4 viewProj;	 // ビュープロジェクション行列（深度値計算用）
	};

	/**----------------------------------------------------------------------------	 * \brief  BulletHoleGPU 弾痕データ(GPU用)
	 * \note   レイマーチング時にSDFで弾痕を表現するためのデータ
	 */
	struct alignas(16) BulletHoleGPU {
		MagMath::Vector3 origin{0.0f, 0.0f, 0.0f};	  ///< 弾の開始位置
		float startRadius = 0.5f;					  ///< 弾痕の開始半径（入口）
		MagMath::Vector3 direction{0.0f, 1.0f, 0.0f}; ///< 弾の正規化方向ベクトル
		float endRadius = 0.2f;						  ///< 弾痕の終了半径（出口）
		float lifeTime = 1.0f;						  ///< 残存時間（0.0～1.0、1.0=完全、0.0=消滅）
		float coneLength = 10.0f;					  ///< 円錐の長さ
		float padding1 = 0.0f;						  ///< パディング
		float padding2 = 0.0f;						  ///< パディング
	};

	/**----------------------------------------------------------------------------	 * \brief  CloudRenderParams 雲レンダリングパラメータ（GPU用）
	 * \note   雲の見た目を制御するパラメータ群
	 */
	struct alignas(16) CloudRenderParams {
		//========================================
		// 雲の位置とサイズ
		MagMath::Vector3 cloudCenter{0.0f, 150.0f, 0.0f}; // 雲の中心座標
		float cloudSizeX = 300.0f;						  // 未使用（構造体パディング用）

		MagMath::Vector3 cloudSize{300.0f, 100.0f, 300.0f}; // 雲のXYZサイズ
		float padding0 = 0.0f;								// パディング

		//========================================
		// ライティング
		MagMath::Vector3 sunDirection{0.3f, 0.8f, 0.5f}; // 太陽光の方向
		float sunIntensity = 1.2f;						 // 太陽光の強度

		MagMath::Vector3 sunColor{1.0f, 0.96f, 0.88f}; // 太陽光の色
		float ambient = 0.3f;						   // 環境光の強度

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
		float debugFlag = 0.0f;			  // デバッグフラグ
		int bulletHoleCount = 0;		  // 有効な弾痕の数
		float bulletHoleFadeStart = 0.0f; // 弾痕フェード開始距離
		float bulletHoleFadeEnd = 2.0f;	  // 弾痕フェード終了距離
	};

	/**----------------------------------------------------------------------------
	 * \brief  BulletHoleBuffer 弾痕配列定数バッファ(GPU用)
	 * \note   最大8個の弾痕を管理（高速化優先）
	 */
	struct alignas(16) BulletHoleBuffer {
		static constexpr int kMaxBulletHoles = 8;	// 最大弾痕数（高速化のため削減）
		BulletHoleGPU bulletHoles[kMaxBulletHoles]; // 弾痕配列
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
		/**----------------------------------------------------------------------------
		 * \brief  弾痕を追加する（円錐形状）
		 * \param  origin 弾の開始位置
		 * \param  direction 弾の方向（正規化済み）
		 * \param  startRadius 弾痕の開始半径（入口）
		 * \param  endRadius 弾痕の終了半径（出口）
		 * \param  coneLength 円錐の長さ
		 * \param  lifeTime 残存時間（秒単位）
		 * \note   Counter-Strike風の動的スモークで、弾が通過した軌跡を作成する
		 */
		void AddBulletHole(const MagMath::Vector3 &origin,
						   const MagMath::Vector3 &direction,
						   float startRadius = 1.5f,
						   float endRadius = 0.3f,
						   float coneLength = 10.0f,
						   float lifeTime = 15.0f);

		/**----------------------------------------------------------------------------
		 * \brief  すべての弾痕をクリアする
		 * \note   シーン切り替え時などに使用
		 */
		void ClearBulletHoles();
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
		 * \note   カメラ、パラメータ、弾痕用の定数バッファを作成
		 */
		void CreateConstantBuffers();

		/**----------------------------------------------------------------------------
		 * \brief  雲パラメータの更新
		 * \note   Transformから雲の位置情報を更新
		 */
		void UpdateCloudParams();

		/**----------------------------------------------------------------------------
		 * \brief  弾痕の更新処理
		 * \param  deltaTime 前フレームからの経過時間（秒）
		 * \note   lifeTimeを減少させ、0以下になった弾痕を削除する
		 */
		void UpdateBulletHoles(float deltaTime);

		/**----------------------------------------------------------------------------
		 * \brief  弾痕データをGPUバッファに転送する
		 * \note   CPU側の弾痕配列をGPU用フォーマットに変換して定数バッファに書き込む
		 */
		void TransferBulletHolesToGPU();

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
		/**----------------------------------------------------------------------------		 * \brief  BulletHole 弾痕データ(CPU側)
		 * \note   CPU側で管理する弾痕情報
		 */
		struct BulletHole {
			MagMath::Vector3 origin;	// 弾の開始位置
			MagMath::Vector3 direction; // 弾の正規化方向ベクトル
			float startRadius;			// 弾痕の開始半径（入口）
			float endRadius;			// 弾痕の終了半径（出口）
			float coneLength;			// 円錐の長さ
			float lifeTime;				// 残存時間（秒単位）
			float maxLifeTime;			// 最大残存時間（秒単位）
		};

		/**----------------------------------------------------------------------------		 * \brief  FullscreenVertex フルスクリーン頂点構造体
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
		MagMath::Transform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 150.0f, 0.0f}};

		//========================================
		// 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		//========================================
		// 定数バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraCB_;	  // カメラ用
		Microsoft::WRL::ComPtr<ID3D12Resource> paramsCB_;	  // パラメータ用
		Microsoft::WRL::ComPtr<ID3D12Resource> bulletHoleCB_; // 弾痕用

		//========================================
		// バッファリソース内のデータを指すポインタ
		CloudCameraConstant *cameraData_ = nullptr;	 // カメラデータ
		CloudRenderParams *paramsData_ = nullptr;	 // パラメータデータ
		CloudRenderParams paramsCPU_;				 // CPU側パラメータ
		BulletHoleBuffer *bulletHoleData_ = nullptr; // 弾痕データ

		//========================================
		// 弾痕管理
		std::vector<BulletHole> bulletHoles_;	 // 弾痕配列(CPU側)
		BulletHoleBuffer bulletHoleBufferCPU_{}; // 弾痕バッファ(CPU側)

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
/*********************************************************************
 * \file   TrailEmitter.h
 * \brief  トレイルパーティクル生成・管理クラス
 *
 * \author MagEngine
 * \date   March 2026
 * \note   軌跡パーティクルのバッファ管理と更新を担当
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "TrailEffectPreset.h"
#include <d3d12.h>
#include <stdexcept>
#include <vector>
#include <wrl/client.h>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						前方宣言
	class TrailEffectSetup;

	///=============================================================================
	///						構造体

	/**----------------------------------------------------------------------------
	 * \brief  TrailVertex トレイル頂点構造体
	 * \note   GPU用の頂点データ
	 */
	struct alignas(16) TrailVertex {
		MagMath::Vector3 position; ///< 頂点位置
		float age;				   ///< 経過時間率 (0.0-1.0)
		MagMath::Vector3 normal;   ///< 法線方向
		float padding;			   ///< パディング
	};

	/**----------------------------------------------------------------------------
	 * \brief  TrailRenderParams トレイルレンダリングパラメータ（GPU用）
	 * \note   トレイルの見た目を制御するパラメータ群
	 */
	struct alignas(16) TrailRenderParams {
		MagMath::Vector3 color{0.5f, 0.5f, 0.5f}; ///< エフェクト色
		float opacity = 0.8f;					  ///< 透明度 (0.0-1.0)
		float width = 2.0f;						  ///< 軌跡幅
		float lifeTime = 3.0f;					  ///< ライフタイム（秒）
		float time = 0.0f;						  ///< 経過時間
		float velocityDamping = 0.95f;			  ///< 速度減衰
		float gravityInfluence = 0.2f;			  ///< 重力影響度
		float padding = 0.0f;					  ///< パディング
	};

	/**----------------------------------------------------------------------------
	 * \brief  CameraConstant カメラ定数バッファ構造体（GPU用）
	 * \note   トレイル描画に必要なカメラ情報
	 */
	struct alignas(16) CameraConstant {
		MagMath::Matrix4x4 viewProj;	///< ビュープロジェクション行列
		MagMath::Vector3 worldPosition; ///< カメラのワールド座標
		float time = 0.0f;				///< 経過時間
	};

	///=============================================================================
	///						クラス
	class TrailEmitter {
		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  初期化
		 * \param  setup TrailEffectSetupポインタ
		 */
		void Initialize(TrailEffectSetup *setup);

		/**----------------------------------------------------------------------------
		 * \brief  更新処理
		 * \param  deltaTime フレーム間の経過時間
		 */
		void Update(float deltaTime);

		/**----------------------------------------------------------------------------
		 * \brief  描画
		 */
		void Draw();

		/**----------------------------------------------------------------------------
		 * \brief  軌跡の生成
		 * \param  position 生成位置
		 * \param  velocity 速度
		 */
		void EmitTrail(const MagMath::Vector3 &position, const MagMath::Vector3 &velocity);

		/**----------------------------------------------------------------------------
		 * \brief  すべての軌跡をクリア
		 */
		void ClearTrails();

		///--------------------------------------------------------------
		///						 入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  色の設定
		 * \param  color 色
		 */
		void SetColor(const MagMath::Vector3 &color) {
			paramsCPU_.color = color;
		}

		/**----------------------------------------------------------------------------
		 * \brief  透明度の設定
		 * \param  opacity 透明度 (0.0-1.0)
		 */
		void SetOpacity(float opacity) {
			paramsCPU_.opacity = opacity;
		}

		/**----------------------------------------------------------------------------
		 * \brief  幅の設定
		 * \param  width 幅
		 */
		void SetWidth(float width) {
			paramsCPU_.width = width;
		}

		/**----------------------------------------------------------------------------
		 * \brief  ライフタイムの設定
		 * \param  lifeTime ライフタイム（秒）
		 */
		void SetLifeTime(float lifeTime) {
			paramsCPU_.lifeTime = lifeTime;
		}

		/**----------------------------------------------------------------------------
		 * \brief  速度減衰の設定
		 * \param  damping 速度減衰 (0.0-1.0)
		 */
		void SetVelocityDamping(float damping) {
			paramsCPU_.velocityDamping = damping;
		}

		/**----------------------------------------------------------------------------
		 * \brief  重力影響度の設定
		 * \param  influence 重力影響度
		 */
		void SetGravityInfluence(float influence) {
			paramsCPU_.gravityInfluence = influence;
		}

		/**----------------------------------------------------------------------------
		 * \brief  パーティクル数の取得
		 * \return パーティクル数
		 */
		size_t GetParticleCount() const {
			return particles_.size();
		}

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  頂点バッファの作成
		 */
		void CreateVertexBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  定数バッファの作成
		 */
		void CreateConstantBuffers();

		/**----------------------------------------------------------------------------
		 * \brief  パーティクルデータをGPUバッファに転送
		 */
		void TransferParticlesToGPU();

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  TrailParticle パーティクルデータ(CPU側)
		 * \note   CPU側で管理するパーティクル情報
		 */
		struct TrailParticle {
			MagMath::Vector3 position; ///< 現在位置
			MagMath::Vector3 velocity; ///< 速度
			float age;				   ///< 経過時間（秒）
			float maxLifeTime;		   ///< 最大ライフタイム
		};

		//========================================
		// TrailEffectSetupポインタ
		TrailEffectSetup *setup_ = nullptr;

		//========================================
		// パーティクルデータ
		std::vector<TrailParticle> particles_;

		//========================================
		// 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		size_t maxVertices_ = 10000;

		//========================================
		// 定数バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> paramsCB_;
		TrailRenderParams *paramsData_ = nullptr;
		TrailRenderParams paramsCPU_;

		//========================================
		// カメラ定数バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraCB_;
		CameraConstant *cameraData_ = nullptr;
		CameraConstant cameraCPU_;

		//========================================
		// その他
		float accumulatedTime_ = 0.0f;
	};
}

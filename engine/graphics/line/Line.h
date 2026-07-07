/*********************************************************************
 * \file   Line.h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "Light.h"
#include "LineStyle.h"
#include "MagMath.h"
#include <span>
#include <vector>
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
	struct LineVertex {
		MagMath::Vector3 position;
		MagMath::Vector4 color;
		float thickness = 1.0f;
		float padding[3]{0.0f, 0.0f, 0.0f}; // 16バイト境界にアライン
	};

	class Camera;
	class LineSetup;
	class Line {
		///--------------------------------------------------------------
		///							メンバ関数
	public:
		~Line();

		/// \brief 初期化
		void Initialize(LineSetup *lineSetup);

		/// @brief GPUリソースを解放する
		void Finalize();

		/// \brief 更新
		void Update();

		/// \brief 描画
		void Draw();

		/// @brief 現在蓄積している頂点数を取得
		size_t GetVertexCount() const {
			return vertices_.size();
		}

		/// @brief 確保済み頂点バッファのバイト数を取得
		UINT GetVertexBufferSizeInBytes() const {
			return vertexBufferView_.SizeInBytes;
		}

		/**----------------------------------------------------------------------------
		 * \brief  ClearLines
		 */
		void ClearLines();

		/**----------------------------------------------------------------------------
		 * \brief  DrawLine ライン描画
		 * \param  start 始点
		 * \param  end 終点
		 * \param  color 色
		 * \param  thickness 線の太さ
		 */
		void DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color, float thickness = 1.0f);
		void DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style, LineRenderMode batchMode);
		void AddPolyline(std::span<const MagMath::Vector3> points, const LineStyle &style, LineRenderMode batchMode);
		void AddRect(const MagMath::Vector3 &min, const MagMath::Vector3 &max, const LineStyle &style, LineRenderMode batchMode);
		void AddCornerBracket(const MagMath::Vector3 &center, const MagMath::Vector3 &halfSize, float cornerLength, const LineStyle &style, LineRenderMode batchMode);
		void AddArrow(const MagMath::Vector3 &position, const MagMath::Vector3 &direction, float size, const LineStyle &style, LineRenderMode batchMode);

		///--------------------------------------------------------------
		///						 静的メンバ関数
	private:
		/**----------------------------------------------------------------------------
		 * \brief  CreateVertexBuffer 頂点バッファの作成
		 */
		void CreateVertexBuffer();

		/**----------------------------------------------------------------------------
		 * \brief  CreateTransformationMatrixBuffer
		 * \return
		 */
		void CreateTransformationMatrixBuffer();
		void AppendHudLineQuad(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style);
		void AppendLineSegment(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style, LineRenderMode batchMode);
		static LineStyle SanitizeStyle(const LineStyle &style);

		///--------------------------------------------------------------
		///							入出力関数
	public:
		/**----------------------------------------------------------------------------
		 * \brief  SetTransform トランスフォーメーションの設定
		 * \param  transform トランスフォーメーション
		 * \note
		 */
		void SetTransform(const MagMath::Transform &transform) {
			transform_ = transform;
		}

		/**----------------------------------------------------------------------------
		 * \brief  GetTransform
		 * \return
		 */
		MagMath::Transform GetTransform() const {
			return transform_;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetModel モデルの設定
		 * \param  model モデル
		 * \note
		 */
		void SetScale(const MagMath::Vector3 &scale) {
			transform_.scale = scale;
		}
		/**----------------------------------------------------------------------------
		 * \brief  GetScale スケールの取得
		 * \return MagMath::Vector3 スケール
		 * \note
		 */
		const MagMath::Vector3 &GetScale() const {
			return transform_.scale;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetRotate 回転の設定
		 * \param  rotate 回転
		 * \note
		 */
		void SetRotation(const MagMath::Vector3 &rotate) {
			transform_.rotate = rotate;
		}
		/**----------------------------------------------------------------------------
		 * \brief  GetRotate 回転の取得
		 * \return MagMath::Vector3 回転
		 * \note
		 */
		const MagMath::Vector3 &GetRotation() const {
			return transform_.rotate;
		}

		/**----------------------------------------------------------------------------
		 * \brief  SetTranslate 移動の設定
		 * \param  translate 移動
		 * \note
		 */
		void SetPosition(const MagMath::Vector3 &translate) {
			transform_.translate = translate;
		}
		/**----------------------------------------------------------------------------
		 * \brief  GetTranslate 移動の取得
		 * \return MagMath::Vector3 移動
		 * \note
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

		///--------------------------------------------------------------
		///							メンバ変数
	private:
		//---------------------------------------
		// オブジェクト3Dセットアップポインタ
		LineSetup *lineSetup_ = nullptr;

		//---------------------------------------
		// 頂点データサイズ管理
		size_t maxVertexCount_ = 100000; // バッファの最大頂点数

		//---------------------------------------
		// 頂点データ
		std::vector<LineVertex> vertices_;

		//---------------------------------------
		// 頂点バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
		LineVertex *mappedVertexData_ = nullptr;
		// バッファリソースの使い道を指すポインタ
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};

		//---------------------------------------
		// トランスフォーメーションマトリックス
		Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixBuffer_;

		//---------------------------------------
		// バッファリソース内のデータを指すポインタ
		// トランスフォーメーションマトリックス
		MagMath::TransformationMatrix *transformationMatrixData_ = nullptr;

		//--------------------------------------
		// Transform
		MagMath::Transform transform_ = {};

		//--------------------------------------
		// カメラ
		Camera *camera_ = nullptr;
	};

}

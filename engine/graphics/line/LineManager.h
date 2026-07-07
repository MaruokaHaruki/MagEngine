/*********************************************************************
 * \file   LineManager.h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "DirectXCore.h"
#include "Line.h"
#include "LineRenderMode.h"
#include "LineSetup.h"
#include "SrvSetup.h"
#include <memory>
#include <string>
 ///=============================================================================
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						ラインマネージャ
	class LineManager {
	public:
		struct Diagnostics {
			bool hudUpdateCalled = false;
			bool lockOnHudUpdateCalled = false;
			uint32_t hudLineAddCount = 0;
			uint32_t lockOnHudLineAddCount = 0;
			size_t worldVertexCountBeforeDraw = 0;
			size_t hudVertexCountBeforeDraw = 0;
			size_t hudVertexBufferSizeInBytes = 0;
			uint32_t renderWorldLineItemCount = 0;
			uint32_t lineRenderPassExecuteCount = 0;
			uint32_t worldDrawCallCount = 0;
			uint32_t hudDrawCallCount = 0;
			bool hudPsoBound = false;
			bool hudCommandListValid = false;
		};

		///--------------------------------------------------------------
		///						 メンバ関数
	public:
		LineManager() = default;
		~LineManager() = default;
		LineManager(const LineManager &) = delete;
		LineManager &operator=(const LineManager &) = delete;

		/// @brief 初期化
		/// @param dxCore DirectXCoreポインタ
		/// @param srvSetup SrvSetupポインタ
		void Initialize(DirectXCore *dxCore, SrvSetup *srvSetup);

		/// @brief 終了処理
		void Finalize();

		/// @brief 更新処理
		void Update();

		/// @brief 描画処理
		void Draw(LineRenderMode renderMode);

		/// @brief 現在の描画モードで描画する
		void Draw();

		/// @brief ImGuiの描画
		void DrawImGui();

		/// @brief 要求時だけLine診断を出力する
		void ReportDiagnostics() const;

		/// @brief ラインのクリア
		void ClearLines();

		/// @brief 以降のDrawLineをどのバッチへ積むかを切り替える
		void SetRenderMode(LineRenderMode renderMode) {
			renderMode_ = renderMode;
		}

		/// @brief 現在のバッチを取得する
		LineRenderMode GetRenderMode() const {
			return renderMode_;
		}

		/// @brief ラインの描画
		/// @param start 始点
		/// @param end 終点
		/// @param color 色
		/// @param thickness 線の太さ
		void DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color, float thickness = 1.0f);

		/// @brief HUD更新が実行されたことを記録する
		void NotifyHudUpdate() {
			diagnostics_.hudUpdateCalled = true;
		}

		/// @brief LockOnHUD更新が実行されたことを記録する
		void NotifyLockOnHudUpdate() {
			diagnostics_.lockOnHudUpdateCalled = true;
		}

		/// @brief HUD生成中のLine追加数を記録する
		void BeginHudLineSource(bool lockOnHud) {
			activeHudLineSourceIsLockOn_ = lockOnHud;
		}

		/// @brief HUD生成元の記録を解除する
		void EndHudLineSource() {
			activeHudLineSourceIsLockOn_ = false;
		}

		/// @brief RenderWorldへ登録されたLine item数を記録する
		void SetRenderWorldLineItemCount(uint32_t count) {
			diagnostics_.renderWorldLineItemCount = count;
		}

		/// @brief LineRenderPassが実行されたことを記録する
		void NotifyLineRenderPassExecuted() {
			++diagnostics_.lineRenderPassExecuteCount;
		}

		/// @brief 現在の頂点数を取得する
		size_t GetVertexCount(LineRenderMode renderMode) const;

		/// @brief HUD頂点バッファサイズを取得する
		size_t GetHudVertexBufferSizeInBytes() const;

		/// @brief グリッドの描画
		/// @param gridSize グリッドサイズ
		/// @param divisions 分割数
		/// @param color 色
		/// @param thickness 線の太さ
		void DrawGrid(float gridSize, int divisions, const MagMath::Vector4 &color, float thickness = 1.0f);

		/// @brief 円の描画
		/// @param center 中心
		/// @param radius 半径
		/// @param color 色
		/// @param thickness 線の太さ
		/// @param normal 円の法線方向（デフォルトはY軸方向）
		/// @param divisions 分割数
		void DrawCircle(const MagMath::Vector3 &center, float radius, const MagMath::Vector4 &color,
			float thickness = 1.0f, const MagMath::Vector3 &normal = { 0.0f, 1.0f, 0.0f }, int divisions = 2);

/// @brief 球体の描画
/// @param center 中心
/// @param radius 半径
/// @param color 色
/// @param divisions 分割数
/// @param thickness 線の太さ
		void DrawSphere(const MagMath::Vector3 &center, float radius, const MagMath::Vector4 &color,
			int divisions = 8, float thickness = 1.0f);

/// @brief 3D空間にテキストを描画
/// @param position 位置
/// @param text テキスト
/// @param color 色
/// @note 未実装：実際のテキスト描画にはDirectX Fontなどの機能が必要
		void DrawText3D(const MagMath::Vector3 &position, const std::string &text, const MagMath::Vector4 &color);

		/// @brief 矢印の先端を描画
		/// @param tip 先端の位置
		/// @param direction 方向ベクトル（正規化済みであること）
		/// @param size 矢じりのサイズ
		/// @param color 色
		/// @param thickness 線の太さ
		void DrawArrowhead(const MagMath::Vector3 &tip, const MagMath::Vector3 &direction, float size,
			const MagMath::Vector4 &color, float thickness = 1.0f);

/// @brief 矢印を描画（線+矢じり）
/// @param start 始点
/// @param end 終点
/// @param color 色
/// @param headSize 矢じりのサイズ（全長に対する比率）
/// @param thickness 線の太さ
		void DrawArrow(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color,
			float headSize = 0.2f, float thickness = 1.0f);

/// @brief 座標軸を描画
/// @param origin 原点
/// @param size サイズ
/// @param thickness 線の太さ
/// @note X軸は赤、Y軸は緑、Z軸は青で描画
		void DrawCoordinateAxes(const MagMath::Vector3 &origin, float size, float thickness = 1.0f);

		/// @brief 立方体を描画
		/// @param center 中心点
		/// @param size サイズ
		/// @param color 色
		/// @param thickness 線の太さ
		void DrawCube(const MagMath::Vector3 &center, float size, const MagMath::Vector4 &color, float thickness = 1.0f);

		/// @brief 直方体を描画
		/// @param center 中心点
		/// @param size サイズ (幅, 高さ, 奥行き)
		/// @param color 色
		/// @param thickness 線の太さ
		void DrawBox(const MagMath::Vector3 &center, const MagMath::Vector3 &size, const MagMath::Vector4 &color, float thickness = 1.0f);

		/// @brief 円錐を描画
		/// @param apex 頂点
		/// @param direction 方向ベクトル
		/// @param height 高さ
		/// @param radius 底面の半径
		/// @param color 色
		/// @param divisions 分割数
		/// @param thickness 線の太さ
		void DrawCone(const MagMath::Vector3 &apex, const MagMath::Vector3 &direction, float height, float radius,
			const MagMath::Vector4 &color, int divisions = 24, float thickness = 1.0f);

/// @brief 円柱を描画
/// @param center 中心点
/// @param direction 方向ベクトル
/// @param height 高さ
/// @param radius 半径
/// @param color 色
/// @param divisions 分割数
/// @param thickness 線の太さ
		void DrawCylinder(const MagMath::Vector3 &center, const MagMath::Vector3 &direction, float height, float radius,
			const MagMath::Vector4 &color, int divisions = 24, float thickness = 1.0f);

/// @brief 太陽シンボルを描画
/// @param center 中心
/// @param size サイズ
/// @param color 色
/// @param thickness 線の太さ
		void DrawSunSymbol(const MagMath::Vector3 &center, float size, const MagMath::Vector4 &color, float thickness = 1.0f);

		/// @brief 光線パターンを描画
		/// @param center 中心
		/// @param maxLength 最大長
		/// @param color 色
		/// @param rayCount 光線数
		/// @param decay 減衰率
		/// @param thickness 線の太さ
		void DrawLightRays(const MagMath::Vector3 &center, float maxLength, const MagMath::Vector4 &color,
			int rayCount, float decay, float thickness = 1.0f);

/// @brief 直交ベクトルの計算
/// @param direction 基準となる方向ベクトル
/// @param perpVector1 [out] 垂直ベクトル1
/// @param perpVector2 [out] 垂直ベクトル2
		void CalculatePerpendicularVectors(const MagMath::Vector3 &direction, MagMath::Vector3 &perpVector1, MagMath::Vector3 &perpVector2);

		/// @brief グリッドオフセットの設定
		/// @param offset オフセット値
		void SetGridOffset(const MagMath::Vector3 &offset) {
			gridOffset_ = offset;
		}

		/// @brief グリッドオフセットの取得
		/// @return オフセット値
		const MagMath::Vector3 &GetGridOffset() const {
			return gridOffset_;
		}

		/// @brief グリッドアニメーションの有効/無効設定
		/// @param enable アニメーション有効フラグ
		void SetGridAnimation(bool enable) {
			isGridAnimationEnabled_ = enable;
		}

		/// @brief グリッドアニメーション速度の設定
		/// @param speed 移動速度
		void SetGridAnimationSpeed(float speed) {
			gridAnimationSpeed_ = speed;
		}

		///--------------------------------------------------------------
		///						 静的メンバ関数
	public:
		/// @brief デフォルトカメラの取得
		/// @return カメラのポインタ
		Camera *GetDefaultCamera() {
			if (!lineSetup_) {
				return nullptr;
			}
			return lineSetup_->GetDefaultCamera();
		}

		/// @brief デフォルトカメラの設定
		/// @param camera カメラのポインタ
		void SetDefaultCamera(Camera *camera) {
			if (!lineSetup_) {
				return;
			}
			lineSetup_->SetDefaultCamera(camera);
		}

		///--------------------------------------------------------------
		///						 メンバ変数
	private:
		Line *GetLine(LineRenderMode renderMode) const;

		// DirectXCore への参照
		DirectXCore *dxCore_ = nullptr;
		// SrvSetup への参照
		SrvSetup *srvSetup_ = nullptr;

		//========================================
		// ライン
		std::unique_ptr<Line> worldLine_;
		std::unique_ptr<Line> hudLine_;

		//========================================
		// LineSetup インスタンス
		std::unique_ptr<LineSetup> lineSetup_;

		//========================================
		// 描画設定
		// ラインを描画するか
		bool isDrawLine_ = true;
		// グリッドを描画するか
		bool isDrawGrid_ = false;
		float gridSize_ = 64.0f;
		int gridDivisions_ = 8;
		MagMath::Vector4 gridColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		MagMath::Vector3 gridOffset_ = { 0.0f, 0.0f, 0.0f }; // グリッドオフセット

		// グリッドアニメーション設定
		bool isGridAnimationEnabled_ = false;
		float gridAnimationSpeed_ = 5.0f;
		float gridAnimationTime_ = 0.0f;

		// 球を描画するか
		bool isDrawSphere_ = true;

		// 現在の線バッチ
		LineRenderMode renderMode_ = LineRenderMode::World;

		// NOTE: Report Render Diagnosticsで直近フレームのHUD Line経路を追えるよう、軽量なカウンタだけ保持する。
		Diagnostics diagnostics_{};
		bool activeHudLineSourceIsLockOn_ = false;
	};
}

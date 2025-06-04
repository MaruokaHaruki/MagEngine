/*********************************************************************
 * \file   LineManager.h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
#include "DirectXCore.h"
#include "Line.h"
#include "LineSetup.h"
#include "SrvSetup.h"
#include <memory>
#include <string>
#include <vector>

///=============================================================================
///						ラインマネージャ
class LineManager {
	///--------------------------------------------------------------
	///						 メンバ関数
public:
	/// @brief インスタンスの取得
	/// @return LineManagerのインスタンス
	static LineManager *GetInstance();

	/// @brief 初期化
	/// @param dxCore DirectXCoreポインタ
	/// @param srvSetup SrvSetupポインタ
	void Initialize(DirectXCore *dxCore, SrvSetup *srvSetup);

	/// @brief 終了処理
	void Finalize();

	/// @brief 更新処理
	void Update();

	/// @brief 描画処理
	void Draw();

	/// @brief ImGuiの描画
	void DrawImGui();

	/// @brief ラインのクリア
	void ClearLines();

	/// @brief ラインの描画
	/// @param start 始点
	/// @param end 終点
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawLine(const Vector3 &start, const Vector3 &end, const Vector4 &color, float thickness = 1.0f);

	/// @brief グリッドの描画
	/// @param gridSize グリッドサイズ
	/// @param divisions 分割数
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawGrid(float gridSize, int divisions, const Vector4 &color, float thickness = 1.0f);

	/// @brief 円の描画
	/// @param center 中心
	/// @param radius 半径
	/// @param color 色
	/// @param thickness 線の太さ
	/// @param normal 円の法線方向（デフォルトはY軸方向）
	/// @param divisions 分割数
	void DrawCircle(const Vector3 &center, float radius, const Vector4 &color,
					float thickness = 1.0f, const Vector3 &normal = {0.0f, 1.0f, 0.0f}, int divisions = 2);

	/// @brief 球体の描画
	/// @param center 中心
	/// @param radius 半径
	/// @param color 色
	/// @param divisions 分割数
	/// @param thickness 線の太さ
	void DrawSphere(const Vector3 &center, float radius, const Vector4 &color,
					int divisions = 8, float thickness = 1.0f);

	/// @brief 3D空間にテキストを描画
	/// @param position 位置
	/// @param text テキスト
	/// @param color 色
	/// @note 未実装：実際のテキスト描画にはDirectX Fontなどの機能が必要
	void DrawText3D(const Vector3 &position, const std::string &text, const Vector4 &color);

	/// @brief 矢印の先端を描画
	/// @param tip 先端の位置
	/// @param direction 方向ベクトル（正規化済みであること）
	/// @param size 矢じりのサイズ
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawArrowhead(const Vector3 &tip, const Vector3 &direction, float size,
					   const Vector4 &color, float thickness = 1.0f);

	/// @brief 矢印を描画（線+矢じり）
	/// @param start 始点
	/// @param end 終点
	/// @param color 色
	/// @param headSize 矢じりのサイズ（全長に対する比率）
	/// @param thickness 線の太さ
	void DrawArrow(const Vector3 &start, const Vector3 &end, const Vector4 &color,
				   float headSize = 0.2f, float thickness = 1.0f);

	/// @brief 座標軸を描画
	/// @param origin 原点
	/// @param size サイズ
	/// @param thickness 線の太さ
	/// @note X軸は赤、Y軸は緑、Z軸は青で描画
	void DrawCoordinateAxes(const Vector3 &origin, float size, float thickness = 1.0f);

	/// @brief 立方体を描画
	/// @param center 中心点
	/// @param size サイズ
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawCube(const Vector3 &center, float size, const Vector4 &color, float thickness = 1.0f);

	/// @brief 直方体を描画
	/// @param center 中心点
	/// @param size サイズ (幅, 高さ, 奥行き)
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawBox(const Vector3 &center, const Vector3 &size, const Vector4 &color, float thickness = 1.0f);

	/// @brief 円錐を描画
	/// @param apex 頂点
	/// @param direction 方向ベクトル
	/// @param height 高さ
	/// @param radius 底面の半径
	/// @param color 色
	/// @param divisions 分割数
	/// @param thickness 線の太さ
	void DrawCone(const Vector3 &apex, const Vector3 &direction, float height, float radius,
				  const Vector4 &color, int divisions = 24, float thickness = 1.0f);

	/// @brief 円柱を描画
	/// @param center 中心点
	/// @param direction 方向ベクトル
	/// @param height 高さ
	/// @param radius 半径
	/// @param color 色
	/// @param divisions 分割数
	/// @param thickness 線の太さ
	void DrawCylinder(const Vector3 &center, const Vector3 &direction, float height, float radius,
					  const Vector4 &color, int divisions = 24, float thickness = 1.0f);

	/// @brief 太陽シンボルを描画
	/// @param center 中心
	/// @param size サイズ
	/// @param color 色
	/// @param thickness 線の太さ
	void DrawSunSymbol(const Vector3 &center, float size, const Vector4 &color, float thickness = 1.0f);

	/// @brief 光線パターンを描画
	/// @param center 中心
	/// @param maxLength 最大長
	/// @param color 色
	/// @param rayCount 光線数
	/// @param decay 減衰率
	/// @param thickness 線の太さ
	void DrawLightRays(const Vector3 &center, float maxLength, const Vector4 &color,
					   int rayCount, float decay, float thickness = 1.0f);

	/// @brief 直交ベクトルの計算
	/// @param direction 基準となる方向ベクトル
	/// @param perpVector1 [out] 垂直ベクトル1
	/// @param perpVector2 [out] 垂直ベクトル2
	void CalculatePerpendicularVectors(const Vector3 &direction, Vector3 &perpVector1, Vector3 &perpVector2);

	///--------------------------------------------------------------
	///						 静的メンバ関数
public:
	/// @brief デフォルトカメラの取得
	/// @return カメラのポインタ
	Camera *GetDefaultCamera() {
		return lineSetup_->GetDefaultCamera();
	}

	/// @brief デフォルトカメラの設定
	/// @param camera カメラのポインタ
	void SetDefaultCamera(Camera *camera) {
		lineSetup_->SetDefaultCamera(camera);
	}

	///--------------------------------------------------------------
	///						 メンバ変数
private:
	//========================================
	// コンストラクタを非公開に
	static LineManager *instance_;

	// コンストラクタ
	LineManager() = default;
	// デストラクタ
	~LineManager() = default;
	// コピーコンストラクタ
	LineManager(const LineManager &) = delete;
	// 代入演算子
	LineManager &operator=(const LineManager &) = delete;

	//========================================
	// DirectXCore への参照
	DirectXCore *dxCore_ = nullptr;
	// SrvSetup への参照
	SrvSetup *srvSetup_ = nullptr;

	//========================================
	// ライン
	std::unique_ptr<Line> line_;

	//========================================
	// LineSetup インスタンス
	std::unique_ptr<LineSetup> lineSetup_;

	//========================================
	// 描画設定
	// ラインを描画するか
	bool isDrawLine_ = true;
	// グリッドを描画するか
	bool isDrawGrid_ = true;
	float gridSize_ = 64.0f;
	int gridDivisions_ = 8;
	Vector4 gridColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector3 gridOffset_ = {0.0f, 0.0f, 0.0f}; // グリッドオフセット

	// 球を描画するか
	bool isDrawSphere_ = true;
};
/*********************************************************************
 * \file   LineManager.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "LineManager.h"
#include "ImguiSetup.h"
//========================================
// 数学関数のインクルード
#define _USE_MATH_DEFINES
#include <math.h>

///=============================================================================
///						インスタンス
LineManager *LineManager::instance_ = nullptr;

///=============================================================================
///						インスタンス設定
LineManager *LineManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new LineManager();
	}
	return instance_;
}

///=============================================================================
///						初期化
void LineManager::Initialize(DirectXCore *dxCore, SrvSetup *srvSetup) {
	//========================================
	// 引数でdxManagerを受取
	dxCore_ = dxCore;
	// 引数でsrvSetupを受取
	srvSetup_ = srvSetup;
	//========================================
	// ラインセットアップの生成
	lineSetup_ = std::make_unique<LineSetup>();
	// ラインセットアップの初期化
	lineSetup_->Initialize(dxCore_, nullptr);
	//========================================
	// ラインの初期化
	line_ = std::make_unique<Line>();
	// ラインの初期化
	line_->Initialize(lineSetup_.get());
}

///=============================================================================
///						終了処理
void LineManager::Finalize() {
	// インスタンスの削除
	delete instance_;
	instance_ = nullptr;
}

///=============================================================================
///						更新処理
void LineManager::Update() {
	// グリッドアニメーション
	if (isGridAnimationEnabled_) {
		// アニメーション時間を更新
		gridAnimationTime_ += gridAnimationSpeed_ * (1.0f / 60.0f); // 60FPSを想定

		// グリッドサイズに基づいてループさせる
		float stepSize = gridSize_ / gridDivisions_;
		float loopTime = stepSize / gridAnimationSpeed_;

		// 時間をループさせる
		if (gridAnimationTime_ >= loopTime) {
			gridAnimationTime_ -= loopTime;
		}

		// Z方向にマイナス方向へ移動するオフセットを計算
		float animationOffset = -gridAnimationTime_ * gridAnimationSpeed_;
		gridOffset_.z = animationOffset;
	}

	// Gridの描画
	if (isDrawGrid_) {
		DrawGrid(gridSize_, gridDivisions_, gridColor_);
	}
	// ラインの更新
	line_->Update();
}

///=============================================================================
///						ラインの描画
void LineManager::Draw() {
	//========================================
	// 共通描画設定
	lineSetup_->CommonDrawSetup();
	// ラインの描画
	line_->Draw();

	//========================================
	// ラインのクリア
	line_->ClearLines();
}

///=============================================================================
///						Imguiの描画
void LineManager::DrawImGui() {
	//========================================
	// 描画設定
	ImGui::Begin("LineManager");
	//========================================
	// Lineを描画するか
	ImGui::Checkbox("Line", &isDrawLine_);
	ImGui::Separator();
	//========================================
	// Gridの描画
	ImGui::Checkbox("Grid", &isDrawGrid_);
	// Gridの設定
	ImGui::SliderFloat("GridSize", &gridSize_, 1.0f, 10000.0f);
	ImGui::SliderInt("Divisions", &gridDivisions_, 1, 512);
	// 色
	ImGui::ColorEdit4("Color", &gridColor_.x);
	// グリッドアニメーション設定
	ImGui::Checkbox("Grid Animation", &isGridAnimationEnabled_);
	ImGui::SliderFloat("Animation Speed", &gridAnimationSpeed_, 0.1f, 20.0f);
	// セパレーター
	ImGui::Separator();
	//========================================
	// Sphereの描画
	ImGui::Checkbox("Sphere", &isDrawSphere_);
	ImGui::End();
}

///=============================================================================
///						ラインのクリア
void LineManager::ClearLines() {
	// ラインのクリア
	line_->ClearLines();
}

///=============================================================================
///						ラインの追加
void LineManager::DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color, float thickness) {
	if (!isDrawLine_) {
		return;
	}
	// TODO: 問題あり
	line_->DrawLine(start, end, color);
#ifdef _DEBUG
	// ラインの追加
	line_->DrawLine(start, end, color);
	// 注：Line.cppを修正して線の太さのサポートを追加する必要があります
	// ここでは既存の関数を使用していますが、実際には線の太さを設定する機能を追加すべきです
#endif // _DEBUG
}

///=============================================================================
///						グリッドの描画
void LineManager::DrawGrid(float gridSize, int divisions, const MagMath::Vector4 &color, float thickness) {
	if (!isDrawGrid_ || divisions <= 0) {
		return;
	}
	float halfSize = gridSize * 0.5f;
	float step = gridSize / divisions;

	// オフセットを適用してグリッドを移動
	for (int i = 0; i <= divisions; ++i) {
		float offset = -halfSize + (i * step);

		// X軸に平行な線（Z方向のオフセットを適用）
		DrawLine({-halfSize + gridOffset_.x, 0.0f, offset + gridOffset_.z},
				 {halfSize + gridOffset_.x, 0.0f, offset + gridOffset_.z}, color, thickness);

		// Z軸に平行な線（X方向のオフセットを適用）
		DrawLine({offset + gridOffset_.x, 0.0f, -halfSize + gridOffset_.z},
				 {offset + gridOffset_.x, 0.0f, halfSize + gridOffset_.z}, color, thickness);
	}
}

///=============================================================================
///						円の描画
void LineManager::DrawCircle(const MagMath::Vector3 &center, float radius, const MagMath::Vector4 &color,
							 float thickness, const MagMath::Vector3 &normal, int divisions) {
	if (!isDrawLine_ || divisions <= 0) {
		return;
	}

	// 法線ベクトルから垂直な2つのベクトルを計算
	MagMath::Vector3 perpVector1, perpVector2;
	CalculatePerpendicularVectors(normal, perpVector1, perpVector2);

	const float angleStep = 2.0f * static_cast<float>(M_PI) / divisions;

	// 円を描画
	for (int i = 0; i < divisions; ++i) {
		float angle1 = angleStep * i;
		float angle2 = angleStep * (i + 1);

		MagMath::Vector3 point1 = {
			center.x + (perpVector1.x * cosf(angle1) + perpVector2.x * sinf(angle1)) * radius,
			center.y + (perpVector1.y * cosf(angle1) + perpVector2.y * sinf(angle1)) * radius,
			center.z + (perpVector1.z * cosf(angle1) + perpVector2.z * sinf(angle1)) * radius};

		MagMath::Vector3 point2 = {
			center.x + (perpVector1.x * cosf(angle2) + perpVector2.x * sinf(angle2)) * radius,
			center.y + (perpVector1.y * cosf(angle2) + perpVector2.y * sinf(angle2)) * radius,
			center.z + (perpVector1.z * cosf(angle2) + perpVector2.z * sinf(angle2)) * radius};

		// 円周上の点同士を結ぶ
		DrawLine(point1, point2, color, thickness);
	}
}

///=============================================================================
///						球体の描画
void LineManager::DrawSphere(const MagMath::Vector3 &center, float radius, const MagMath::Vector4 &color,
							 int divisions, float thickness) {
	if (!isDrawSphere_ || divisions <= 0) {
		return;
	}

	// 緯度・経度方向の分割を計算
	float latStep = static_cast<float>(M_PI) / divisions;		 // 緯度のステップ
	float lonStep = 2.0f * static_cast<float>(M_PI) / divisions; // 経度のステップ

	// 緯度方向のループ
	for (int lat = 0; lat <= divisions; ++lat) {
		float theta = lat * latStep; // 緯度角
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);

		// 経度方向のループ
		for (int lon = 0; lon < divisions; ++lon) {
			float phi = lon * lonStep; // 経度角
			float nextPhi = (lon + 1) * lonStep;

			// 現在の点
			MagMath::Vector3 point1 = {
				center.x + radius * sinTheta * cosf(phi),
				center.y + radius * cosTheta,
				center.z + radius * sinTheta * sinf(phi)};

			// 次の経度の点
			MagMath::Vector3 point2 = {
				center.x + radius * sinTheta * cosf(nextPhi),
				center.y + radius * cosTheta,
				center.z + radius * sinTheta * sinf(nextPhi)};

			// 次の緯度の点
			if (lat < divisions) {
				float nextTheta = (lat + 1) * latStep;
				float sinNextTheta = sinf(nextTheta);
				float cosNextTheta = cosf(nextTheta);

				MagMath::Vector3 point3 = {
					center.x + radius * sinNextTheta * cosf(phi),
					center.y + radius * cosNextTheta,
					center.z + radius * sinNextTheta * sinf(phi)};

				// 緯度方向の線を描画
				DrawLine(point1, point3, color, thickness);
			}

			// 経度方向の線を描画
			DrawLine(point1, point2, color, thickness);
		}
	}
}

///=============================================================================
///						3Dテキスト描画（プレースホルダー）
void LineManager::DrawText3D(const MagMath::Vector3 &position, const std::string &text, const MagMath::Vector4 &color) {
	// 実装は別途DirectXのフォント機能やベクターフォント描画などで実装する必要があります
	// ここではダミーの実装として、テキストの位置に小さなマーカーを描画します
	DrawLine(position, {position.x + 0.1f, position.y + 0.1f, position.z}, color);
	DrawLine(position, {position.x - 0.1f, position.y + 0.1f, position.z}, color);
}

///=============================================================================
///						矢じりの描画
void LineManager::DrawArrowhead(const MagMath::Vector3 &tip, const MagMath::Vector3 &direction, float size,
								const MagMath::Vector4 &color, float thickness) {
	// 方向ベクトルに垂直な2つのベクトルを計算
	MagMath::Vector3 perpVector1, perpVector2;
	CalculatePerpendicularVectors(direction, perpVector1, perpVector2);

	// 矢じりの基部
	MagMath::Vector3 arrowBase = {
		tip.x - direction.x * size * 2.0f,
		tip.y - direction.y * size * 2.0f,
		tip.z - direction.z * size * 2.0f};

	// 矢じりの4つの角へのラインを描画
	DrawLine(tip, {arrowBase.x + perpVector1.x * size, arrowBase.y + perpVector1.y * size, arrowBase.z + perpVector1.z * size}, color, thickness);

	DrawLine(tip, {arrowBase.x - perpVector1.x * size, arrowBase.y - perpVector1.y * size, arrowBase.z - perpVector1.z * size}, color, thickness);

	DrawLine(tip, {arrowBase.x + perpVector2.x * size, arrowBase.y + perpVector2.y * size, arrowBase.z + perpVector2.z * size}, color, thickness);

	DrawLine(tip, {arrowBase.x - perpVector2.x * size, arrowBase.y - perpVector2.y * size, arrowBase.z - perpVector2.z * size}, color, thickness);
}

///=============================================================================
///						矢印の描画（線+矢じり）
void LineManager::DrawArrow(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color,
							float headSize, float thickness) {
	// 線分を描画
	DrawLine(start, end, color, thickness);

	// 方向ベクトル計算
	MagMath::Vector3 direction = {
		end.x - start.x,
		end.y - start.y,
		end.z - start.z};

	// 方向ベクトルの長さを計算
	float length = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	// 長さが0より大きい場合のみ矢じりを描画
	if (length > 0.0001f) {
		// 方向ベクトルを正規化
		direction.x /= length;
		direction.y /= length;
		direction.z /= length;

		// 矢じりのサイズを計算（全長の一定割合）
		float arrowheadSize = length * headSize;

		// 矢じりを描画
		DrawArrowhead(end, direction, arrowheadSize, color, thickness);
	}
}

///=============================================================================
///						座標軸の描画
void LineManager::DrawCoordinateAxes(const MagMath::Vector3 &origin, float size, float thickness) {
	// X軸（赤）
	DrawArrow(origin, {origin.x + size, origin.y, origin.z}, {1.0f, 0.0f, 0.0f, 1.0f}, 0.1f, thickness);

	// Y軸（緑）
	DrawArrow(origin, {origin.x, origin.y + size, origin.z}, {0.0f, 1.0f, 0.0f, 1.0f}, 0.1f, thickness);

	// Z軸（青）
	DrawArrow(origin, {origin.x, origin.y, origin.z + size}, {0.0f, 0.0f, 1.0f, 1.0f}, 0.1f, thickness);
}

///=============================================================================
///						立方体の描画
void LineManager::DrawCube(const MagMath::Vector3 &center, float size, const MagMath::Vector4 &color, float thickness) {
	// halfSize
	float halfSize = size * 0.5f;

	// 8つの頂点を計算
	MagMath::Vector3 p1 = {center.x - halfSize, center.y - halfSize, center.z - halfSize};
	MagMath::Vector3 p2 = {center.x + halfSize, center.y - halfSize, center.z - halfSize};
	MagMath::Vector3 p3 = {center.x + halfSize, center.y - halfSize, center.z + halfSize};
	MagMath::Vector3 p4 = {center.x - halfSize, center.y - halfSize, center.z + halfSize};
	MagMath::Vector3 p5 = {center.x - halfSize, center.y + halfSize, center.z - halfSize};
	MagMath::Vector3 p6 = {center.x + halfSize, center.y + halfSize, center.z - halfSize};
	MagMath::Vector3 p7 = {center.x + halfSize, center.y + halfSize, center.z + halfSize};
	MagMath::Vector3 p8 = {center.x - halfSize, center.y + halfSize, center.z + halfSize};

	// 下面
	DrawLine(p1, p2, color, thickness);
	DrawLine(p2, p3, color, thickness);
	DrawLine(p3, p4, color, thickness);
	DrawLine(p4, p1, color, thickness);

	// 上面
	DrawLine(p5, p6, color, thickness);
	DrawLine(p6, p7, color, thickness);
	DrawLine(p7, p8, color, thickness);
	DrawLine(p8, p5, color, thickness);

	// 垂直な辺
	DrawLine(p1, p5, color, thickness);
	DrawLine(p2, p6, color, thickness);
	DrawLine(p3, p7, color, thickness);
	DrawLine(p4, p8, color, thickness);
}

///=============================================================================
///						直方体の描画
void LineManager::DrawBox(const MagMath::Vector3 &center, const MagMath::Vector3 &size, const MagMath::Vector4 &color, float thickness) {
	// halfSize
	float halfSizeX = size.x * 0.5f;
	float halfSizeY = size.y * 0.5f;
	float halfSizeZ = size.z * 0.5f;

	// 8つの頂点を計算
	MagMath::Vector3 p1 = {center.x - halfSizeX, center.y - halfSizeY, center.z - halfSizeZ};
	MagMath::Vector3 p2 = {center.x + halfSizeX, center.y - halfSizeY, center.z - halfSizeZ};
	MagMath::Vector3 p3 = {center.x + halfSizeX, center.y - halfSizeY, center.z + halfSizeZ};
	MagMath::Vector3 p4 = {center.x - halfSizeX, center.y - halfSizeY, center.z + halfSizeZ};
	MagMath::Vector3 p5 = {center.x - halfSizeX, center.y + halfSizeY, center.z - halfSizeZ};
	MagMath::Vector3 p6 = {center.x + halfSizeX, center.y + halfSizeY, center.z - halfSizeZ};
	MagMath::Vector3 p7 = {center.x + halfSizeX, center.y + halfSizeY, center.z + halfSizeZ};
	MagMath::Vector3 p8 = {center.x - halfSizeX, center.y + halfSizeY, center.z + halfSizeZ};

	// 下面
	DrawLine(p1, p2, color, thickness);
	DrawLine(p2, p3, color, thickness);
	DrawLine(p3, p4, color, thickness);
	DrawLine(p4, p1, color, thickness);

	// 上面
	DrawLine(p5, p6, color, thickness);
	DrawLine(p6, p7, color, thickness);
	DrawLine(p7, p8, color, thickness);
	DrawLine(p8, p5, color, thickness);

	// 垂直な辺
	DrawLine(p1, p5, color, thickness);
	DrawLine(p2, p6, color, thickness);
	DrawLine(p3, p7, color, thickness);
	DrawLine(p4, p8, color, thickness);
}

///=============================================================================
///						円錐の描画
void LineManager::DrawCone(const MagMath::Vector3 &apex, const MagMath::Vector3 &direction, float height, float radius,
						   const MagMath::Vector4 &color, int divisions, float thickness) {
	// 方向ベクトルを正規化
	MagMath::Vector3 normalizedDir;
	float dirLength = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	if (dirLength > 0.0001f) {
		normalizedDir = {
			direction.x / dirLength,
			direction.y / dirLength,
			direction.z / dirLength};
	} else {
		normalizedDir = {0.0f, -1.0f, 0.0f}; // デフォルト方向
	}

	// 底面の中心を計算
	MagMath::Vector3 baseCenter = {
		apex.x + normalizedDir.x * height,
		apex.y + normalizedDir.y * height,
		apex.z + normalizedDir.z * height};

	// 方向ベクトルに垂直な2つのベクトルを計算
	MagMath::Vector3 perpVector1, perpVector2;
	CalculatePerpendicularVectors(normalizedDir, perpVector1, perpVector2);

	// 底面の円を描画
	DrawCircle(baseCenter, radius, color, thickness, normalizedDir, divisions);

	// 頂点から底面の円周上の点への線を描画
	const float angleStep = 2.0f * static_cast<float>(M_PI) / divisions;

	for (int i = 0; i < divisions; ++i) {
		float angle = angleStep * i;

		MagMath::Vector3 point = {
			baseCenter.x + (perpVector1.x * cosf(angle) + perpVector2.x * sinf(angle)) * radius,
			baseCenter.y + (perpVector1.y * cosf(angle) + perpVector2.y * sinf(angle)) * radius,
			baseCenter.z + (perpVector1.z * cosf(angle) + perpVector2.z * sinf(angle)) * radius};

		DrawLine(apex, point, color, thickness);
	}
}

///=============================================================================
///						円柱の描画
void LineManager::DrawCylinder(const MagMath::Vector3 &center, const MagMath::Vector3 &direction, float height, float radius,
							   const MagMath::Vector4 &color, int divisions, float thickness) {
	// 方向ベクトルを正規化
	MagMath::Vector3 normalizedDir;
	float dirLength = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	if (dirLength > 0.0001f) {
		normalizedDir = {
			direction.x / dirLength,
			direction.y / dirLength,
			direction.z / dirLength};
	} else {
		normalizedDir = {0.0f, 1.0f, 0.0f}; // デフォルト方向
	}

	// 上面と下面の中心を計算
	float halfHeight = height * 0.5f;
	MagMath::Vector3 topCenter = {
		center.x + normalizedDir.x * halfHeight,
		center.y + normalizedDir.y * halfHeight,
		center.z + normalizedDir.z * halfHeight};

	MagMath::Vector3 bottomCenter = {
		center.x - normalizedDir.x * halfHeight,
		center.y - normalizedDir.y * halfHeight,
		center.z - normalizedDir.z * halfHeight};

	// 上面と下面の円を描画
	DrawCircle(topCenter, radius, color, thickness, normalizedDir, divisions);
	DrawCircle(bottomCenter, radius, color, thickness, normalizedDir, divisions);

	// 上面と下面を結ぶ線を描画
	const float angleStep = 2.0f * static_cast<float>(M_PI) / divisions;
	MagMath::Vector3 perpVector1, perpVector2;
	CalculatePerpendicularVectors(normalizedDir, perpVector1, perpVector2);

	for (int i = 0; i < divisions; ++i) {
		float angle = angleStep * i;

		MagMath::Vector3 topPoint = {
			topCenter.x + (perpVector1.x * cosf(angle) + perpVector2.x * sinf(angle)) * radius,
			topCenter.y + (perpVector1.y * cosf(angle) + perpVector2.y * sinf(angle)) * radius,
			topCenter.z + (perpVector1.z * cosf(angle) + perpVector2.z * sinf(angle)) * radius};

		MagMath::Vector3 bottomPoint = {
			bottomCenter.x + (perpVector1.x * cosf(angle) + perpVector2.x * sinf(angle)) * radius,
			bottomCenter.y + (perpVector1.y * cosf(angle) + perpVector2.y * sinf(angle)) * radius,
			bottomCenter.z + (perpVector1.z * cosf(angle) + perpVector2.z * sinf(angle)) * radius};

		DrawLine(topPoint, bottomPoint, color, thickness);
	}
}

///=============================================================================
///						太陽シンボルの描画
void LineManager::DrawSunSymbol(const MagMath::Vector3 &center, float size, const MagMath::Vector4 &color, float thickness) {
	// 中心の円
	DrawCircle(center, size * 0.4f, {1.0f, 1.0f, 0.5f, 1.0f}, thickness);

	// 放射状の光線
	for (int i = 0; i < 8; i++) {
		float angle = i * static_cast<float>(M_PI) / 4.0f;
		MagMath::Vector3 inner = {
			center.x + cosf(angle) * size * 0.5f,
			center.y + sinf(angle) * size * 0.5f,
			center.z};
		MagMath::Vector3 outer = {
			center.x + cosf(angle) * size,
			center.y + sinf(angle) * size,
			center.z};
		DrawLine(inner, outer, color, thickness);
	}
}

///=============================================================================
///						光線パターンの描画
void LineManager::DrawLightRays(const MagMath::Vector3 &center, float maxLength, const MagMath::Vector4 &color,
								int rayCount, float decay, float thickness) {
	// 球面上にランダムな方向の光線を描画
	for (int i = 0; i < rayCount; i++) {
		// 球面上の均等分布点を生成
		float phi = static_cast<float>(M_PI * 2.0f) * (float)i / rayCount;
		float theta = static_cast<float>(M_PI) * (float)i / rayCount;

		MagMath::Vector3 direction = {
			sinf(theta) * cosf(phi),
			sinf(theta) * sinf(phi),
			cosf(theta)};

		// 減衰を反映した長さのグラデーション線
		int segments = 5;
		for (int j = 0; j < segments; j++) {
			float t1 = (float)j / segments;
			float t2 = (float)(j + 1) / segments;

			// 減衰関数に基づく長さの調整
			float len1 = maxLength * t1;
			float len2 = maxLength * t2;

			// 距離による減衰効果の反映（減衰係数に応じた透明度）
			float alpha1 = 1.0f / (1.0f + powf(t1 * 5.0f, decay));

			MagMath::Vector3 point1 = {
				center.x + direction.x * len1,
				center.y + direction.y * len1,
				center.z + direction.z * len1};

			MagMath::Vector3 point2 = {
				center.x + direction.x * len2,
				center.y + direction.y * len2,
				center.z + direction.z * len2};

			MagMath::Vector4 segmentColor = {
				color.x,
				color.y,
				color.z,
				color.w * alpha1};

			DrawLine(point1, point2, segmentColor, thickness * alpha1);
		}
	}
}

///=============================================================================
///						垂直ベクトルの計算
void LineManager::CalculatePerpendicularVectors(const MagMath::Vector3 &direction, MagMath::Vector3 &perpVector1, MagMath::Vector3 &perpVector2) {
	// 正規化された方向ベクトルが必要
	MagMath::Vector3 normalizedDir = direction;
	float len = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	if (len > 0.0001f) {
		normalizedDir.x /= len;
		normalizedDir.y /= len;
		normalizedDir.z /= len;
	} else {
		normalizedDir = {0.0f, 1.0f, 0.0f}; // デフォルト方向
	}

	// 方向ベクトルに垂直な最初のベクトルを作成
	if (fabs(normalizedDir.y) < 0.99f) {
		// Y軸との外積で一つ目の垂直ベクトルを作成
		perpVector1 = {
			normalizedDir.z,
			0.0f,
			-normalizedDir.x};
	} else {
		// X軸との外積で一つ目の垂直ベクトルを作成
		perpVector1 = {
			0.0f,
			-normalizedDir.z,
			normalizedDir.y};
	}

	// 正規化
	float perpLength = sqrtf(perpVector1.x * perpVector1.x +
							 perpVector1.y * perpVector1.y +
							 perpVector1.z * perpVector1.z);

	if (perpLength > 0.0001f) {
		perpVector1.x /= perpLength;
		perpVector1.y /= perpLength;
		perpVector1.z /= perpLength;
	}

	// 2つ目の垂直ベクトルを計算 (法線ベクトルと1つ目の垂直ベクトルの外積)
	perpVector2 = {
		normalizedDir.y * perpVector1.z - normalizedDir.z * perpVector1.y,
		normalizedDir.z * perpVector1.x - normalizedDir.x * perpVector1.z,
		normalizedDir.x * perpVector1.y - normalizedDir.y * perpVector1.x};

	// perpVector2は自動的に正規化されています（2つの正規化されたベクトルの外積結果）
}
/*********************************************************************
 * \file EditorViewportMath.h
 * \brief DPI適用後の画像座標とワールド座標の選択計算
 *********************************************************************/
#pragma once
#include "EditorDocument.h"
#include "engine/math/structure/common/Vector2.h"
#include <optional>
namespace MagEngine {
	struct ViewportRect { float x = 0; float y = 0; float width = 0; float height = 0; };
	struct EditorViewportInputState {
		ViewportRect imageRect;
		MagMath::Vector2 renderSize{1280, 720};
		bool focused = false;
		bool allowInput = false;
	};
	/// @brief 画像外・入力遮断・無効サイズでは中立値。座標はレンダーピクセル単位
	MagMath::Vector2 MapViewportPosition(MagMath::Vector2 point, const EditorViewportInputState &state);
	MagMath::Vector2 MapViewportDelta(MagMath::Vector2 delta, const EditorViewportInputState &state);
	struct EditorRay { MagMath::Vector3 origin; MagMath::Vector3 direction; };
	struct EditorBounds { MagMath::Vector3 min{-0.5f, -0.5f, -0.5f}; MagMath::Vector3 max{0.5f, 0.5f, 0.5f}; };
	/// @brief シーン画像を縦横比を保って収めた領域を返す
	ViewportRect FitViewport(ViewportRect available, float aspect);
	/// @brief 画像外・無効な領域はnullopt。camera回転とfovはラジアン
	std::optional<EditorRay> ScreenToViewportRay(MagMath::Vector2 point, const ViewportRect &rect, const MagMath::Transform &camera, float fov);
	/// @brief ローカル境界をTransformして交差する正の距離。非交差はnullopt
	std::optional<float> IntersectEditorBounds(const EditorRay &ray, const EditorBounds &bounds, const MagMath::Transform &transform);
}

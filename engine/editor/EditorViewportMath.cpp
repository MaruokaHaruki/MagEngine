/*********************************************************************
 * \file EditorViewportMath.cpp
 * \brief 画像の余白を除いた選択と逆変換による境界判定
 *********************************************************************/
#include "EditorViewportMath.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <limits>
namespace MagEngine {
	namespace {
		DirectX::XMMATRIX Rotation(const MagMath::Vector3 &v) { return DirectX::XMMatrixRotationX(v.x) * DirectX::XMMatrixRotationY(v.y) * DirectX::XMMatrixRotationZ(v.z); }
	}
	ViewportRect FitViewport(ViewportRect r, float aspect) {
		if (r.width <= 0 || r.height <= 0 || !std::isfinite(aspect) || aspect <= 0) { return {}; }
		const float width = (std::min)(r.width, r.height * aspect); const float height = width / aspect;
		return {r.x + (r.width - width) * 0.5f, r.y + (r.height - height) * 0.5f, width, height};
	}
	std::optional<EditorRay> ScreenToViewportRay(MagMath::Vector2 point, const ViewportRect &r, const MagMath::Transform &camera, float fov) {
		if (r.width <= 0 || r.height <= 0 || point.x < r.x || point.y < r.y || point.x >= r.x + r.width || point.y >= r.y + r.height || !std::isfinite(fov) || fov <= 0 || fov >= 3.14159265f) { return {}; }
		using namespace DirectX;
		const float y = (1.0f - 2.0f * (point.y - r.y) / r.height) * std::tan(fov * 0.5f);
		const float x = (2.0f * (point.x - r.x) / r.width - 1.0f) * std::tan(fov * 0.5f) * r.width / r.height;
		XMFLOAT3 direction;
		XMStoreFloat3(&direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(x, y, 1, 0), Rotation(camera.rotate))));
		return EditorRay{camera.translate, {direction.x, direction.y, direction.z}};
	}
	std::optional<float> IntersectEditorBounds(const EditorRay &ray, const EditorBounds &bounds, const MagMath::Transform &t) {
		if (t.scale.x <= 0 || t.scale.y <= 0 || t.scale.z <= 0) { return {}; }
		using namespace DirectX;
		const auto world = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z) * Rotation(t.rotate) * XMMatrixTranslation(t.translate.x, t.translate.y, t.translate.z);
		const auto inverse = XMMatrixInverse(nullptr, world);
		XMFLOAT3 origin, direction;
		XMStoreFloat3(&origin, XMVector3TransformCoord(XMVectorSet(ray.origin.x, ray.origin.y, ray.origin.z, 1), inverse));
		XMStoreFloat3(&direction, XMVector3TransformNormal(XMVectorSet(ray.direction.x, ray.direction.y, ray.direction.z, 0), inverse));
		const float o[]{origin.x, origin.y, origin.z}, d[]{direction.x, direction.y, direction.z};
		const float low[]{bounds.min.x, bounds.min.y, bounds.min.z}, high[]{bounds.max.x, bounds.max.y, bounds.max.z};
		float enter = 0, exit = std::numeric_limits<float>::max();
		for (size_t axis = 0; axis < 3; ++axis) {
			if (std::abs(d[axis]) < 1e-7f) { if (o[axis] < low[axis] || o[axis] > high[axis]) { return {}; } continue; }
			float a = (low[axis] - o[axis]) / d[axis], b = (high[axis] - o[axis]) / d[axis]; if (a > b) { std::swap(a, b); }
			enter = (std::max)(enter, a); exit = (std::min)(exit, b); if (enter > exit) { return {}; }
		}
		return enter;
	}
}

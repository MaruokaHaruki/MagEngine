/*********************************************************************
 * \file   CloudHoleTypes.h
 * \brief  Cloud弾痕に使うSDF穴形状のCPU側共通定義
 *
 * \note   HLSLのCloudHoleShape IDと順序を一致させる前提で管理する。
 *********************************************************************/
#pragma once

#include "MagMath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace MagEngine {
	enum class CloudHoleShape : uint32_t {
		Circle = 0,
		RoundedBox,
		ChamferBox,
		Box,
		OrientedBox,
		Segment,
		Rhombus,
		Trapezoid,
		Parallelogram,
		EquilateralTriangle,
		IsoscelesTriangle,
		Triangle,
		UnevenCapsule,
		Pentagon,
		Hexagon,
		Octagon,
		Hexagram,
		Pentagram,
		RegularStar,
		Pie,
		CutDisk,
		Arc,
		Ring,
		Horseshoe,
		Vesica,
		OrientedVesica,
		Moon,
		RoundedCross,
		Egg,
		Heart,
		Cross,
		RoundedX,
		Polygon,
		Ellipse,
		Parabola,
		ParabolaSegment,
		QuadraticBezier,
		BlobbyCross,
		Tunnel,
		Stairs,
		QuadraticCircle,
		Hyperbola,
		CoolS,
		CircleWave,
		Count
	};

	using CloudBulletHoleShape = CloudHoleShape;

	enum class CloudHoleShapeCategory : uint32_t {
		Basic = 0,
		Polygon,
		Circular,
		Organic,
		Curve,
		Experimental,
		Count
	};

	enum CloudHoleFlags : uint32_t {
		CloudHoleFlag_None = 0,
		CloudHoleFlag_Rounded = 1u << 0,
		CloudHoleFlag_Onion = 1u << 1,
	};

	struct CloudHoleData {
		MagMath::Vector3 position{0.0f, 0.0f, 0.0f};
		float startRadius = 1.5f;
		MagMath::Vector3 direction{0.0f, 1.0f, 0.0f};
		float endRadius = 0.3f;
		float lifetime = 15.0f;
		float maxLifetime = 15.0f;
		float coneLength = 10.0f;
		float rotation = 0.0f;
		float aspectRatio = 1.0f;
		CloudHoleShape shape = CloudHoleShape::Circle;
		uint32_t flags = CloudHoleFlag_None;
		uint32_t polygonPointCount = 0;
		uint32_t padding0 = 0;
		MagMath::Vector4 shapeParams0{0.0f, 0.0f, 0.0f, 0.0f};
		MagMath::Vector4 shapeParams1{0.0f, 0.0f, 0.0f, 0.0f};
	};

	inline bool IsCloudHoleShapeValid(uint32_t shapeId) {
		return shapeId < static_cast<uint32_t>(CloudHoleShape::Count);
	}

	inline bool IsCloudHoleShapeValid(CloudHoleShape shape) {
		return IsCloudHoleShapeValid(static_cast<uint32_t>(shape));
	}

	inline bool IsFinite(float value) {
		return std::isfinite(value);
	}

	inline float ClampFiniteFloat(float value, float fallback, float minValue, float maxValue) {
		if(!IsFinite(value)) {
			value = fallback;
		}
		return std::clamp(value, minValue, maxValue);
	}

	inline MagMath::Vector4 ClampFiniteVector4(const MagMath::Vector4 &value, float minValue, float maxValue) {
		return {
			ClampFiniteFloat(value.x, 0.0f, minValue, maxValue),
			ClampFiniteFloat(value.y, 0.0f, minValue, maxValue),
			ClampFiniteFloat(value.z, 0.0f, minValue, maxValue),
			ClampFiniteFloat(value.w, 0.0f, minValue, maxValue),
		};
	}

	inline MagMath::Vector3 NormalizeOrDefault(const MagMath::Vector3 &value, const MagMath::Vector3 &fallback) {
		const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
		if(length <= 0.0001f || !IsFinite(length)) {
			return fallback;
		}
		return {value.x / length, value.y / length, value.z / length};
	}

	inline std::string_view ToString(CloudHoleShape shape) {
		switch(shape) {
		case CloudHoleShape::Circle: return "Circle";
		case CloudHoleShape::RoundedBox: return "RoundedBox";
		case CloudHoleShape::ChamferBox: return "ChamferBox";
		case CloudHoleShape::Box: return "Box";
		case CloudHoleShape::OrientedBox: return "OrientedBox";
		case CloudHoleShape::Segment: return "Segment";
		case CloudHoleShape::Rhombus: return "Rhombus";
		case CloudHoleShape::Trapezoid: return "Trapezoid";
		case CloudHoleShape::Parallelogram: return "Parallelogram";
		case CloudHoleShape::EquilateralTriangle: return "EquilateralTriangle";
		case CloudHoleShape::IsoscelesTriangle: return "IsoscelesTriangle";
		case CloudHoleShape::Triangle: return "Triangle";
		case CloudHoleShape::UnevenCapsule: return "UnevenCapsule";
		case CloudHoleShape::Pentagon: return "Pentagon";
		case CloudHoleShape::Hexagon: return "Hexagon";
		case CloudHoleShape::Octagon: return "Octagon";
		case CloudHoleShape::Hexagram: return "Hexagram";
		case CloudHoleShape::Pentagram: return "Pentagram";
		case CloudHoleShape::RegularStar: return "RegularStar";
		case CloudHoleShape::Pie: return "Pie";
		case CloudHoleShape::CutDisk: return "CutDisk";
		case CloudHoleShape::Arc: return "Arc";
		case CloudHoleShape::Ring: return "Ring";
		case CloudHoleShape::Horseshoe: return "Horseshoe";
		case CloudHoleShape::Vesica: return "Vesica";
		case CloudHoleShape::OrientedVesica: return "OrientedVesica";
		case CloudHoleShape::Moon: return "Moon";
		case CloudHoleShape::RoundedCross: return "RoundedCross";
		case CloudHoleShape::Egg: return "Egg";
		case CloudHoleShape::Heart: return "Heart";
		case CloudHoleShape::Cross: return "Cross";
		case CloudHoleShape::RoundedX: return "RoundedX";
		case CloudHoleShape::Polygon: return "Polygon";
		case CloudHoleShape::Ellipse: return "Ellipse";
		case CloudHoleShape::Parabola: return "Parabola";
		case CloudHoleShape::ParabolaSegment: return "ParabolaSegment";
		case CloudHoleShape::QuadraticBezier: return "QuadraticBezier";
		case CloudHoleShape::BlobbyCross: return "BlobbyCross";
		case CloudHoleShape::Tunnel: return "Tunnel";
		case CloudHoleShape::Stairs: return "Stairs";
		case CloudHoleShape::QuadraticCircle: return "QuadraticCircle";
		case CloudHoleShape::Hyperbola: return "Hyperbola";
		case CloudHoleShape::CoolS: return "CoolS";
		case CloudHoleShape::CircleWave: return "CircleWave";
		default: return "Invalid";
		}
	}

	inline std::string_view ToString(CloudHoleShapeCategory category) {
		switch(category) {
		case CloudHoleShapeCategory::Basic: return "Basic";
		case CloudHoleShapeCategory::Polygon: return "Polygon";
		case CloudHoleShapeCategory::Circular: return "Circular";
		case CloudHoleShapeCategory::Organic: return "Organic";
		case CloudHoleShapeCategory::Curve: return "Curve";
		case CloudHoleShapeCategory::Experimental: return "Experimental";
		default: return "Invalid";
		}
	}

	inline bool IsCloudHoleExperimentalShape(CloudHoleShape shape) {
		switch(shape) {
		case CloudHoleShape::QuadraticBezier:
		case CloudHoleShape::Hyperbola:
		case CloudHoleShape::CoolS:
		case CloudHoleShape::CircleWave:
		case CloudHoleShape::Tunnel:
		case CloudHoleShape::Stairs:
		case CloudHoleShape::BlobbyCross:
		case CloudHoleShape::Polygon:
			return true;
		default:
			return false;
		}
	}

	inline CloudHoleShapeCategory GetCloudHoleShapeCategory(CloudHoleShape shape) {
		const uint32_t id = static_cast<uint32_t>(shape);
		if(id <= static_cast<uint32_t>(CloudHoleShape::Parallelogram)) {
			return CloudHoleShapeCategory::Basic;
		}
		if(id <= static_cast<uint32_t>(CloudHoleShape::RegularStar)) {
			return CloudHoleShapeCategory::Polygon;
		}
		if(id <= static_cast<uint32_t>(CloudHoleShape::Moon)) {
			return CloudHoleShapeCategory::Circular;
		}
		if(id <= static_cast<uint32_t>(CloudHoleShape::Ellipse)) {
			return CloudHoleShapeCategory::Organic;
		}
		return CloudHoleShapeCategory::Curve;
	}

	inline const std::array<CloudHoleShape, 9> &GetBasicCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 9> shapes = {
			CloudHoleShape::Circle, CloudHoleShape::RoundedBox, CloudHoleShape::ChamferBox,
			CloudHoleShape::Box, CloudHoleShape::OrientedBox, CloudHoleShape::Segment,
			CloudHoleShape::Rhombus, CloudHoleShape::Trapezoid, CloudHoleShape::Parallelogram};
		return shapes;
	}

	inline const std::array<CloudHoleShape, 10> &GetPolygonCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 10> shapes = {
			CloudHoleShape::EquilateralTriangle, CloudHoleShape::IsoscelesTriangle, CloudHoleShape::Triangle,
			CloudHoleShape::UnevenCapsule, CloudHoleShape::Pentagon, CloudHoleShape::Hexagon,
			CloudHoleShape::Octagon, CloudHoleShape::Hexagram, CloudHoleShape::Pentagram,
			CloudHoleShape::RegularStar};
		return shapes;
	}

	inline const std::array<CloudHoleShape, 8> &GetCircularCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 8> shapes = {
			CloudHoleShape::Pie, CloudHoleShape::CutDisk, CloudHoleShape::Arc, CloudHoleShape::Ring,
			CloudHoleShape::Horseshoe, CloudHoleShape::Vesica, CloudHoleShape::OrientedVesica, CloudHoleShape::Moon};
		return shapes;
	}

	inline const std::array<CloudHoleShape, 7> &GetOrganicCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 7> shapes = {
			CloudHoleShape::RoundedCross, CloudHoleShape::Egg, CloudHoleShape::Heart, CloudHoleShape::Cross,
			CloudHoleShape::RoundedX, CloudHoleShape::Polygon, CloudHoleShape::Ellipse};
		return shapes;
	}

	inline const std::array<CloudHoleShape, 10> &GetCurveCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 10> shapes = {
			CloudHoleShape::Parabola, CloudHoleShape::ParabolaSegment, CloudHoleShape::QuadraticBezier,
			CloudHoleShape::BlobbyCross, CloudHoleShape::Tunnel, CloudHoleShape::Stairs,
			CloudHoleShape::QuadraticCircle, CloudHoleShape::Hyperbola, CloudHoleShape::CoolS,
			CloudHoleShape::CircleWave};
		return shapes;
	}

	inline const std::array<CloudHoleShape, 8> &GetExperimentalCloudHoleShapes() {
		static constexpr std::array<CloudHoleShape, 8> shapes = {
			CloudHoleShape::QuadraticBezier, CloudHoleShape::Hyperbola, CloudHoleShape::CoolS,
			CloudHoleShape::CircleWave, CloudHoleShape::Tunnel, CloudHoleShape::Stairs,
			CloudHoleShape::BlobbyCross, CloudHoleShape::Polygon};
		return shapes;
	}

	inline CloudHoleShape GetFirstShapeInCategory(CloudHoleShapeCategory category) {
		switch(category) {
		case CloudHoleShapeCategory::Basic: return GetBasicCloudHoleShapes().front();
		case CloudHoleShapeCategory::Polygon: return GetPolygonCloudHoleShapes().front();
		case CloudHoleShapeCategory::Circular: return GetCircularCloudHoleShapes().front();
		case CloudHoleShapeCategory::Organic: return GetOrganicCloudHoleShapes().front();
		case CloudHoleShapeCategory::Curve: return GetCurveCloudHoleShapes().front();
		case CloudHoleShapeCategory::Experimental: return GetExperimentalCloudHoleShapes().front();
		default: return CloudHoleShape::Circle;
		}
	}

	inline CloudHoleShape AdvanceCloudHoleShapeInCategory(CloudHoleShape shape, CloudHoleShapeCategory category, int direction) {
		auto advanceIn = [shape, direction](const auto &shapes) {
			auto it = std::find(shapes.begin(), shapes.end(), shape);
			if(it == shapes.end()) {
				return shapes.front();
			}
			const int count = static_cast<int>(shapes.size());
			const int index = static_cast<int>(it - shapes.begin());
			return shapes[static_cast<size_t>((index + direction + count) % count)];
		};
		switch(category) {
		case CloudHoleShapeCategory::Basic: return advanceIn(GetBasicCloudHoleShapes());
		case CloudHoleShapeCategory::Polygon: return advanceIn(GetPolygonCloudHoleShapes());
		case CloudHoleShapeCategory::Circular: return advanceIn(GetCircularCloudHoleShapes());
		case CloudHoleShapeCategory::Organic: return advanceIn(GetOrganicCloudHoleShapes());
		case CloudHoleShapeCategory::Curve: return advanceIn(GetCurveCloudHoleShapes());
		case CloudHoleShapeCategory::Experimental: return advanceIn(GetExperimentalCloudHoleShapes());
		default: return CloudHoleShape::Circle;
		}
	}

	inline CloudHoleShapeCategory AdvanceCloudHoleCategory(CloudHoleShapeCategory category, int direction) {
		const int count = static_cast<int>(CloudHoleShapeCategory::Count);
		const int index = static_cast<int>(category);
		return static_cast<CloudHoleShapeCategory>((index + direction + count) % count);
	}

	inline CloudHoleData SanitizeCloudHoleData(CloudHoleData data) {
		if(!IsCloudHoleShapeValid(data.shape)) {
			data.shape = CloudHoleShape::Circle;
		}
		data.startRadius = ClampFiniteFloat(data.startRadius, 1.5f, 0.001f, 10000.0f);
		data.endRadius = ClampFiniteFloat(data.endRadius, 0.3f, 0.001f, 10000.0f);
		data.lifetime = ClampFiniteFloat(data.lifetime, 15.0f, 0.0f, 3600.0f);
		data.maxLifetime = ClampFiniteFloat(data.maxLifetime, data.lifetime > 0.0f ? data.lifetime : 15.0f, 0.001f, 3600.0f);
		data.coneLength = ClampFiniteFloat(data.coneLength, 10.0f, 0.001f, 100000.0f);
		data.rotation = ClampFiniteFloat(data.rotation, 0.0f, -100000.0f, 100000.0f);
		data.aspectRatio = ClampFiniteFloat(data.aspectRatio, 1.0f, 0.05f, 20.0f);
		data.direction = NormalizeOrDefault(data.direction, {0.0f, 1.0f, 0.0f});
		data.shapeParams0 = ClampFiniteVector4(data.shapeParams0, -100.0f, 100.0f);
		data.shapeParams1 = ClampFiniteVector4(data.shapeParams1, -100.0f, 100.0f);
		data.flags &= CloudHoleFlag_Rounded | CloudHoleFlag_Onion;
		data.polygonPointCount = std::clamp(data.polygonPointCount, 0u, 8u);
		if(data.shape == CloudHoleShape::Polygon && data.polygonPointCount < 3u) {
			data.polygonPointCount = 6u;
		}
		return data;
	}

	inline CloudHoleData MakeCloudHolePreset(CloudHoleShape shape) {
		CloudHoleData data{};
		data.shape = IsCloudHoleShapeValid(shape) ? shape : CloudHoleShape::Circle;
		data.shapeParams0 = {0.0f, 0.0f, 0.0f, 0.0f};
		data.shapeParams1 = {0.0f, 0.0f, 0.0f, 0.0f};

		switch(data.shape) {
		case CloudHoleShape::RoundedBox: data.shapeParams0.x = 0.2f; break;
		case CloudHoleShape::ChamferBox: data.shapeParams0.x = 0.15f; break;
		case CloudHoleShape::Trapezoid: data.shapeParams0.x = 0.6f; break;
		case CloudHoleShape::Parallelogram: data.shapeParams0.x = 0.3f; break;
		case CloudHoleShape::UnevenCapsule: data.shapeParams0 = {0.35f, 0.2f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Pentagon: data.polygonPointCount = 5u; break;
		case CloudHoleShape::Hexagon: data.polygonPointCount = 6u; break;
		case CloudHoleShape::Octagon: data.polygonPointCount = 8u; break;
		case CloudHoleShape::Hexagram: data.shapeParams0 = {6.0f, 0.45f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Pentagram:
		case CloudHoleShape::RegularStar: data.shapeParams0 = {5.0f, 0.45f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Pie: data.shapeParams0.x = 0.65f; break;
		case CloudHoleShape::CutDisk: data.shapeParams0.x = 0.35f; break;
		case CloudHoleShape::Arc: data.shapeParams0 = {0.18f, 0.75f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Ring: data.shapeParams0.x = 0.2f; break;
		case CloudHoleShape::Horseshoe: data.shapeParams0 = {0.2f, 0.7f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Moon: data.shapeParams0.x = 0.45f; break;
		case CloudHoleShape::RoundedCross: data.shapeParams0.x = 0.2f; break;
		case CloudHoleShape::Cross: data.shapeParams0.x = 0.28f; break;
		case CloudHoleShape::RoundedX: data.shapeParams0.x = 0.22f; break;
		case CloudHoleShape::Polygon: data.polygonPointCount = 6u; break;
		case CloudHoleShape::Ellipse: data.aspectRatio = 1.5f; break;
		case CloudHoleShape::Parabola: data.shapeParams0.x = 1.0f; break;
		case CloudHoleShape::ParabolaSegment: data.shapeParams0 = {1.0f, 0.7f, 0.0f, 0.0f}; break;
		case CloudHoleShape::QuadraticBezier: data.shapeParams0 = {-0.7f, -0.4f, 0.0f, 0.8f}; data.shapeParams1 = {0.7f, -0.4f, 0.0f, 0.0f}; break;
		case CloudHoleShape::Stairs: data.shapeParams0.x = 5.0f; break;
		case CloudHoleShape::CircleWave: data.shapeParams0 = {0.5f, 6.0f, 0.0f, 0.0f}; break;
		default: break;
		}

		data.maxLifetime = data.lifetime;
		return SanitizeCloudHoleData(data);
	}
}

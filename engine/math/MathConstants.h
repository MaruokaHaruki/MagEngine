/*********************************************************************
 * \file   MathConstants.h
 * \brief  MagMath名前空間と数学定数・基本関数の定義
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   すべての数学機能をMagMath名前空間で管理
 *********************************************************************/
#pragma once
#include <algorithm>
#include <cmath>

namespace MagMath {
	//========================================
	// 数学定数
	//========================================
	constexpr float PI = 3.14159265358979323846f;
	constexpr float TWO_PI = PI * 2.0f;
	constexpr float HALF_PI = PI * 0.5f;
	constexpr float EPSILON = 1e-6f;
	constexpr float DEG_TO_RAD = PI / 180.0f;
	constexpr float RAD_TO_DEG = 180.0f / PI;

	//========================================
	// 基本的な数学関数（std::の代替）
	//========================================

	/// <summary>最小値を返す</summary>
	template <typename T>
	constexpr T Min(T a, T b) {
		return (a < b) ? a : b;
	}

	/// <summary>最大値を返す</summary>
	template <typename T>
	constexpr T Max(T a, T b) {
		return (a > b) ? a : b;
	}

	/// <summary>絶対値を返す</summary>
	template <typename T>
	constexpr T Abs(T value) {
		return (value < 0) ? -value : value;
	}

	/// <summary>値を範囲内にクランプする</summary>
	template <typename T>
	constexpr T Clamp(T value, T min, T max) {
		return (value < min) ? min : (value > max) ? max
												   : value;
	}

	//========================================
	// std::のラッパー関数
	//========================================

	inline float Sin(float radian) {
		return std::sin(radian);
	}
	inline float Cos(float radian) {
		return std::cos(radian);
	}
	inline float Tan(float radian) {
		return std::tan(radian);
	}
	inline float Sqrt(float value) {
		return std::sqrt(value);
	}
	inline float Pow(float base, float exponent) {
		return std::pow(base, exponent);
	}
	inline float Atan2(float y, float x) {
		return std::atan2(y, x);
	}
	inline float Asin(float value) {
		return std::asin(value);
	}
	inline float Acos(float value) {
		return std::acos(value);
	}

	//========================================
	// 補間関数
	//========================================

	/// <summary>線形補間（テンプレート版）</summary>
	template <typename T>
	inline T Lerp(T a, T b, float t) {
		return a + (b - a) * t;
	}

	/// <summary>スムーズステップ補間（3次エルミート補間）</summary>
	inline float SmoothStep(float edge0, float edge1, float x) {
		float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	/// <summary>度からラジアンへ変換</summary>
	constexpr float DegreesToRadians(float degrees) {
		return degrees * DEG_TO_RAD;
	}

	/// <summary>ラジアンから度へ変換</summary>
	constexpr float RadiansToDegrees(float radians) {
		return radians * RAD_TO_DEG;
	}
}

/*********************************************************************
 * \file   MathUtility.h
 * \brief  プレイヤーシステムで使用する数学ユーティリティ関数
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   MagEngine側（engine/math）の数学ユーティリティを使用し、重複コードを削減
 *         プレイヤー特有の拡張関数のみをここで定義
 *
 * \usage  基本的な数学関数（Min, Max, Clamp, Abs, Lerp, Normalize等）は
 *         MagMath名前空間から直接使用してください。
 *         このファイルはフォールバック機能付き正規化やプレイヤー固有の
 *         イージング関数など、拡張機能のみを提供します。
 *********************************************************************/
#pragma once
#include "Vector3.h"
#include "MathConstants.h"

using namespace MagMath;

///=============================================================================
///						数学ユーティリティ名前空間
namespace MathUtility {

	//========================================
	//          ベクトル演算
	//========================================
	// ベクトル演算はMagMath::の関数を使用
	// - Normalize(const Vector3&)    : ベクトル正規化
	// - Dot(const Vector3&, const Vector3&)    : 内積
	// - Cross(const Vector3&, const Vector3&)  : 外積
	// - Length(const Vector3&)  : ベクトルの長さ
	// - Distance(const Vector3&, const Vector3&)  : 2点間の距離

	/// @brief ベクトルの正規化（フォールバック値付き）
	/// @param v 正規化するベクトル
	/// @param fallback ゼロベクトルの場合に返すデフォルト値
	/// @return 正規化されたベクトル（長さが0の場合はfallbackを返す）
	/// @note MagMath::Normalizeをラップし、ゼロベクトル時の処理を追加
	inline Vector3 NormalizeWithFallback(const Vector3 &v, const Vector3 &fallback = {0.0f, 0.0f, 1.0f}) {
		float length = MagMath::Length(v);
		if (length < MagMath::EPSILON) {
			return fallback;
		}
		return MagMath::Normalize(v);
	}

	/// @brief ベクトルの長さの2乗を計算（平方根計算を省略）
	/// @param v ベクトル
	/// @return ベクトルの長さの2乗
	/// @note 距離比較など、実際の長さが不要な場合に高速
	inline float LengthSquared(const Vector3 &v) {
		return MagMath::Dot(v, v);
	}

	/// @brief 2つのベクトル間の距離の2乗を計算
	/// @param a 位置ベクトルA
	/// @param b 位置ベクトルB
	/// @return 距離の2乗
	/// @note 距離比較に使用すると高速
	inline float DistanceSquared(const Vector3 &a, const Vector3 &b) {
		Vector3 diff = a - b;
		return MagMath::Dot(diff, diff);
	}

	//========================================
	//          補間（Interpolation）
	//========================================
	// 補間関数はMagMath::の実装を使用
	// - Lerp<T>(T a, T b, float t)    : 線形補間
	// - SmoothStep(float edge0, float edge1, float x) : スムーズステップ補間

	/// @brief ベクトルの線形補間（MagMath::Lerpのベクトル版）
	/// @param a 開始ベクトル
	/// @param b 終了ベクトル
	/// @param t 補間係数（0.0 〜 1.0）
	/// @return 補間されたベクトル
	inline Vector3 LerpVector(const Vector3 &a, const Vector3 &b, float t) {
		return a + (b - a) * t;
	}

	//========================================
	//          イージング関数
	//========================================

	/// @brief EaseOutQuad イージング（減速）
	/// @param t 進行度（0.0 〜 1.0）
	/// @return イージング適用後の値（0.0 〜 1.0）
	/// @note 開始時は速く、終了時は遅くなる
	inline float EaseOutQuad(float t) {
		return t * (2.0f - t);
	}

	/// @brief EaseInQuad イージング（加速）
	/// @param t 進行度（0.0 〜 1.0）
	/// @return イージング適用後の値（0.0 〜 1.0）
	/// @note 開始時は遅く、終了時は速くなる
	inline float EaseInQuad(float t) {
		return t * t;
	}

	/// @brief EaseInOutQuad イージング（加速→減速）
	/// @param t 進行度（0.0 〜 1.0）
	/// @return イージング適用後の値（0.0 〜 1.0）
	/// @note 開始・終了時は遅く、中間は速い
	inline float EaseInOutQuad(float t) {
		return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
	}

	/// @brief EaseOutCubic イージング（より強い減速）
	/// @param t 進行度（0.0 〜 1.0）
	/// @return イージング適用後の値（0.0 〜 1.0）
	inline float EaseOutCubic(float t) {
		float f = t - 1.0f;
		return f * f * f + 1.0f;
	}

	//========================================
	//          角度関連
	//========================================

	/// @brief 度数法をラジアンに変換（MagMath::DegreesToRadiansの別名）
	/// @param degrees 角度（度）
	/// @return 角度（ラジアン）
	inline float DegreesToRadians(float degrees) {
		return MagMath::DegreesToRadians(degrees);
	}

	/// @brief ラジアンを度数法に変換（MagMath::RadiansToDegreesの別名）
	/// @param radians 角度（ラジアン）
	/// @return 角度（度）
	inline float RadiansToDegrees(float radians) {
		return MagMath::RadiansToDegrees(radians);
	}

	/// @brief 角度を-180〜180度の範囲に正規化
	/// @param degrees 角度（度）
	/// @return 正規化された角度（-180 〜 180）
	inline float NormalizeAngleDegrees(float degrees) {
		while (degrees > 180.0f)
			degrees -= 360.0f;
		while (degrees < -180.0f)
			degrees += 360.0f;
		return degrees;
	}

	/// @brief 角度を-π〜πの範囲に正規化
	/// @param radians 角度（ラジアン）
	/// @return 正規化された角度（-π 〜 π）
	inline float NormalizeAngleRadians(float radians) {
		while (radians > MagMath::PI)
			radians -= MagMath::TWO_PI;
		while (radians < -MagMath::PI)
			radians += MagMath::TWO_PI;
		return radians;
	}

	//========================================
	//          その他のユーティリティ
	//========================================
	// 基本関数はMagMath::の実装を使用
	// - Min<T>(T a, T b)   : 最小値
	// - Max<T>(T a, T b)   : 最大値
	// - Clamp<T>(T value, T min, T max)  : クランプ
	// - Abs<T>(T value)    : 絶対値

	/// @brief 値の符号を取得
	/// @param value 値
	/// @return -1, 0, 1 のいずれか
	inline float Sign(float value) {
		if (value > 0.0f)
			return 1.0f;
		if (value < 0.0f)
			return -1.0f;
		return 0.0f;
	}

} // namespace MathUtility

#pragma once
#include "Matrix4x4.h"
#include "Vector4.h"
#include <cstdint>

namespace MagMath {

	/// @brief シェーダーへ転送するマテリアル定数
	/// @note paddingを含むメンバー順はHLSL側の定数バッファレイアウトに依存するため、変更時は対応シェーダーを同時に更新する。
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3]; // float4境界を保ち、後続行列のHLSLレイアウトずれを防ぐ。
		Matrix4x4 uvTransform;
		/// 光沢度
		float shininess;
		/// 環境マップ
		int32_t enableEnvironmentMap;
		float environmentMapStrength;
	};

} // namespace MagMath

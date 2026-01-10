#pragma once
#include "Matrix4x4.h"
#include "Vector4.h"
#include <cstdint>

namespace MagMath {

	/// <summary>
	/// マテリアル
	/// </summary>
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		/// 光沢度
		float shininess;
		/// 環境マップ
		int32_t enableEnvironmentMap;
		float environmentMapStrength;
	};

} // namespace MagMath
#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace MagMath {

	/// @brief 頂点バッファと入力レイアウトで共有する頂点属性
	/// @note メンバー順と各型はInputLayoutおよび頂点シェーダーの入力契約に依存する。
	struct VertexData {
		Vector4 position;
		Vector2 texCoord;
		Vector3 normal;
	};

} // namespace MagMath

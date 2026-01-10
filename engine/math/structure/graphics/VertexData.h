#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

namespace MagMath {

	/// <summary>
	/// VertexData
	/// </summary>
	struct VertexData {
		Vector4 position;
		Vector2 texCoord;
		Vector3 normal;
	};

} // namespace MagMath
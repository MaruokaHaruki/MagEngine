#pragma once
#include "Matrix4x4.h"

namespace MagMath {

	/// <summary>
	/// トランスレートマトリックス
	/// </summary>
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInvTranspose;
	};

} // namespace MagMath
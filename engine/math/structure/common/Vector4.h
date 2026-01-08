#pragma once

namespace MagMath {

	/// <summary>
	/// 4次元ベクトル
	/// </summary>
	struct Vector4 {
		float x;
		float y;
		float z;
		float w;
	};

} // namespace MagMath

// 後方互換性のためのグローバルエイリアス
using Vector4 = MagMath::Vector4;
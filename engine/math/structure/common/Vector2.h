#pragma once

namespace MagMath {

	/// <summary>
	/// 2次元ベクトル
	/// </summary>
	struct Vector2 {
		float x;
		float y;
	};

} // namespace MagMath

// 後方互換性のためのグローバルエイリアス
using Vector2 = MagMath::Vector2;
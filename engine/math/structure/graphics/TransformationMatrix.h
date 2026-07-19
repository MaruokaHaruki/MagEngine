#pragma once
#include "Matrix4x4.h"

namespace MagMath {

	/// @brief 描画時にシェーダーへ渡す座標変換行列群
	/// @note メンバー順は定数バッファのレイアウト契約である。WorldInvTransposeは法線変換用に常に更新する。
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInvTranspose;
	};

} // namespace MagMath

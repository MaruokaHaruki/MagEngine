#pragma once
#include "Vector3.h"

namespace MagMath {

	/// @brief 3Dオブジェクトの拡縮・回転・平行移動をまとめる値型
	/// @note 各要素の座標系と回転単位は使用する行列生成関数に従う。未初期化のままGPUへ転送してはならない。
	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

} // namespace MagMath

#pragma once
#include "Vector3.h"
#include "Vector4.h"

namespace MagMath {

	/// @brief 並行光源
	struct DirectionalLight {
		Vector4 color;	   // ライトの色
		Vector3 direction; // ライトの向き
		float intensity;   // 光度
	};

	/// @brief 点光源
	struct PointLight {
		Vector4 color;	  // ライトの色(16 bytes)
		Vector3 position; // ライトの位置(12 bytes)
		float intensity;  // ライトの強度(4 bytes) = 16 bytes
		float radius;	  // 半径(4 bytes)
		float decay;	  // 減衰(4 bytes)
		float padding;	  // パディング(4 bytes) = 16 bytes
	};

	/// @brief スポットライト
	struct SpotLight {
		Vector4 color;		   // ライトの色
		Vector3 position;	   // ライトの位置
		float intensity;	   // 光度
		Vector3 direction;	   // ライトの向き
		float distance;		   // 距離
		float decay;		   // 減衰
		float cosFalloffStart; // フォールオフ開始
		float cosFalloffEnd;   // フォールオフ終了
		float padding;		   // パディング
	};

} // namespace MagMath

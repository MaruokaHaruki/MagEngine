/*********************************************************************
 * \file   LineRenderMode.h
 * \brief  Line描画の用途を分けるための最小モード定義
 *********************************************************************/
#pragma once

#include <cstdint>

namespace MagEngine {
	enum class LineRenderMode : uint8_t {
		World,
		Hud,
	};
}

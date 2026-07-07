/*********************************************************************
 * \file   LineStyle.h
 * \brief  Line描画の見た目と描画先をまとめる軽量設定
 *********************************************************************/
#pragma once

#include "LineRenderMode.h"
#include "MagMath.h"

namespace MagEngine {
	struct LineStyle {
		LineRenderMode mode = LineRenderMode::World;
		MagMath::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		float thickness = 0.01f;
		bool dashed = false;
		float dashLength = 8.0f;
		float gapLength = 4.0f;
	};
}

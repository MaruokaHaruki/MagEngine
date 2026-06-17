/*********************************************************************
 * \file   RenderContext.h
 * \brief  RenderPass実行に必要な描画基盤参照
 *********************************************************************/
#pragma once

#include <d3d12.h>

namespace MagEngine {
	struct RenderContext {
		ID3D12GraphicsCommandList &commandList;
	};
}

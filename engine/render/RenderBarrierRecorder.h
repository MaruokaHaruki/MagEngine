/*********************************************************************
 * \file   RenderBarrierRecorder.h
 * \brief  手動ResourceBarrier発行とRenderGraph記録を一本化するヘルパー
 *********************************************************************/
#pragma once

#include "RenderGraph.h"

#include <d3d12.h>

namespace MagEngine {
	class RenderBarrierRecorder {
	public:
		explicit RenderBarrierRecorder(RenderGraph &renderGraph);

		/// @brief 呼び出し側が決定したTransitionを発行し、同じ内容を検証用に記録する
		void Transition(
			ID3D12GraphicsCommandList &commandList,
			RenderResourceId resourceId,
			ID3D12Resource &resource,
			D3D12_RESOURCE_STATES beforeState,
			D3D12_RESOURCE_STATES afterState,
			RenderBarrierPoint point);

	private:
		RenderResourceState ConvertState(D3D12_RESOURCE_STATES state) const;
		uint32_t GetSequence(RenderBarrierPoint point) const;

		// NOTE: RenderGraphはGPU Resourceを所有しないため、記録専用の非所有参照に限定する。
		RenderGraph &renderGraph_;
	};
}

/*********************************************************************
 * \file   RenderBarrierRecorder.h
 * \brief  手動ResourceBarrier発行とRenderGraph記録を一本化するヘルパー
 *********************************************************************/
#pragma once

#include "RenderGraph.h"

#include <d3d12.h>

namespace MagEngine {
	class IResourceBarrierSink {
	public:
		virtual ~IResourceBarrierSink() = default;
		virtual void ApplyTransition(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) = 0;
	};

	enum class RenderBarrierRecorderError : uint8_t {
		None,
		SameState,
		UnknownState,
		ConflictingBarrierPoint,
	};

	struct RenderBarrierRecorderResult {
		bool isValid = true;
		RenderBarrierRecorderError error = RenderBarrierRecorderError::None;
	};

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

		/// @brief RenderGraph外の内部Resource向けに、発行だけを共通化する
		void Transition(
			ID3D12GraphicsCommandList &commandList,
			ID3D12Resource &resource,
			D3D12_RESOURCE_STATES beforeState,
			D3D12_RESOURCE_STATES afterState);

		/// @brief CPUテスト用に実CommandListなしで発行回数と記録内容を検証する
		RenderBarrierRecorderResult TransitionForTesting(
			IResourceBarrierSink &sink,
			RenderResourceId resourceId,
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

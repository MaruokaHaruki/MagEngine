/*********************************************************************
 * \file   RenderTransitionExecutor.h
 * \brief  RenderGraphのTransition Planを限定的に実Barrier発行へ接続するクラス
 *********************************************************************/
#pragma once

#include "RenderBarrierRecorder.h"
#include "RenderGraph.h"

#include <d3d12.h>
#include <vector>

namespace MagEngine {
	struct RenderPassEntry;

	class IRenderResourceResolver {
	public:
		virtual ~IRenderResourceResolver() = default;
		// RenderGraphはGPU Resourceを所有しないため、実体解決は描画基盤側へ委譲する。
		virtual ID3D12Resource *ResolveRenderResource(RenderResourceId resourceId) = 0;
	};

	enum class RenderTransitionExecutionError : uint8_t {
		None,
		MissingPlan,
		UnsupportedPlan,
		ResourceResolveFailed,
		DuplicateBoundaryExecution,
	};

	struct RenderTransitionExecutionResult {
		bool isValid = true;
		RenderTransitionExecutionError error = RenderTransitionExecutionError::None;
		RenderResourceTransitionPlan plan{};
	};

	class RenderTransitionExecutor {
	public:
		RenderTransitionExecutor(
			const RenderGraph &renderGraph,
			const std::vector<RenderPassEntry> &renderPasses,
			RenderBarrierRecorder &barrierRecorder,
			IRenderResourceResolver &resourceResolver);

		RenderTransitionExecutionResult ExecuteBoundary(
			ID3D12GraphicsCommandList &commandList,
			RenderTransitionBoundary boundary);

		// CPUテストでは実CommandListを持たないため、Sink経由で発行記録だけを検証する。
		RenderTransitionExecutionResult ExecuteBoundaryForTesting(
			IResourceBarrierSink &sink,
			RenderTransitionBoundary boundary);

		// 自動発行範囲を広げすぎないため、既存手動Barrierの置換対象だけを許可する。
		static bool IsAutoTransitionSupported(const RenderResourceTransitionPlan &plan);

	private:
		RenderTransitionExecutionResult FindPlan(RenderTransitionBoundary boundary) const;
		bool HasExecutedBoundary(const RenderResourceTransitionPlan &plan) const;
		RenderBarrierPoint ToBarrierPoint(RenderTransitionBoundary boundary) const;
		D3D12_RESOURCE_STATES ToD3D12State(RenderResourceState state) const;

		const RenderGraph &renderGraph_;
		const std::vector<RenderPassEntry> &renderPasses_;
		RenderBarrierRecorder &barrierRecorder_;
		IRenderResourceResolver &resourceResolver_;
	};
}

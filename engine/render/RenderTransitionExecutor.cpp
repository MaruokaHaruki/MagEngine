#include "RenderTransitionExecutor.h"

#include "Renderer.h"

#include <algorithm>
#include <cassert>

namespace MagEngine {
	RenderTransitionExecutor::RenderTransitionExecutor(
		const RenderGraph &renderGraph,
		const std::vector<RenderPassEntry> &renderPasses,
		RenderBarrierRecorder &barrierRecorder,
		IRenderResourceResolver &resourceResolver)
		: renderGraph_(renderGraph),
		  renderPasses_(renderPasses),
		  barrierRecorder_(barrierRecorder),
		  resourceResolver_(resourceResolver) {
	}

	bool RenderTransitionExecutor::IsAutoTransitionSupported(const RenderResourceTransitionPlan &plan) {
		return
			(plan.resource == RenderResourceId::SceneColor &&
			 plan.beforeState == RenderResourceState::PixelShaderResource &&
			 plan.afterState == RenderResourceState::RenderTarget) ||
			(plan.resource == RenderResourceId::SceneColor &&
			 plan.beforeState == RenderResourceState::RenderTarget &&
			 plan.afterState == RenderResourceState::PixelShaderResource) ||
			(plan.resource == RenderResourceId::PresentColor &&
			 plan.beforeState == RenderResourceState::Present &&
			 plan.afterState == RenderResourceState::RenderTarget) ||
			(plan.resource == RenderResourceId::PresentColor &&
			 plan.beforeState == RenderResourceState::RenderTarget &&
			 plan.afterState == RenderResourceState::Present);
	}

	RenderTransitionExecutionResult RenderTransitionExecutor::ExecuteBoundary(
		ID3D12GraphicsCommandList &commandList,
		RenderTransitionBoundary boundary) {
		RenderTransitionExecutionResult result = FindPlan(boundary);
		if(!result.isValid) {
			assert(false && "RenderTransitionExecutor could not find a transition plan for the boundary.");
			return result;
		}
		if(!IsAutoTransitionSupported(result.plan)) {
			assert(false && "RenderTransitionExecutor attempted to execute an unsupported transition plan.");
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::UnsupportedPlan, result.plan};
		}
		if(HasExecutedBoundary(result.plan)) {
			assert(false && "RenderTransitionExecutor detected duplicate boundary execution.");
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::DuplicateBoundaryExecution, result.plan};
		}

		ID3D12Resource *resource = resourceResolver_.ResolveRenderResource(result.plan.resource);
		if(!resource) {
			assert(false && "RenderTransitionExecutor could not resolve a D3D12 resource.");
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::ResourceResolveFailed, result.plan};
		}

		barrierRecorder_.Transition(
			commandList,
			result.plan.resource,
			*resource,
			ToD3D12State(result.plan.beforeState),
			ToD3D12State(result.plan.afterState),
			ToBarrierPoint(*result.plan.boundary));
		return result;
	}

	RenderTransitionExecutionResult RenderTransitionExecutor::ExecuteBoundaryForTesting(
		IResourceBarrierSink &sink,
		RenderTransitionBoundary boundary) {
		RenderTransitionExecutionResult result = FindPlan(boundary);
		if(!result.isValid) {
			return result;
		}
		if(!IsAutoTransitionSupported(result.plan)) {
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::UnsupportedPlan, result.plan};
		}
		if(HasExecutedBoundary(result.plan)) {
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::DuplicateBoundaryExecution, result.plan};
		}
		if(!resourceResolver_.ResolveRenderResource(result.plan.resource)) {
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::ResourceResolveFailed, result.plan};
		}

		const RenderBarrierRecorderResult recorderResult = barrierRecorder_.TransitionForTesting(
			sink,
			result.plan.resource,
			ToD3D12State(result.plan.beforeState),
			ToD3D12State(result.plan.afterState),
			ToBarrierPoint(*result.plan.boundary));
		if(!recorderResult.isValid) {
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::DuplicateBoundaryExecution, result.plan};
		}
		return result;
	}

	RenderTransitionExecutionResult RenderTransitionExecutor::FindPlan(RenderTransitionBoundary boundary) const {
		const std::vector<RenderResourceTransitionPlan> plans = renderGraph_.BuildTransitionPlan(renderPasses_);
		const auto it = std::find_if(plans.begin(), plans.end(), [&](const RenderResourceTransitionPlan &plan) {
			return plan.boundary.has_value() && *plan.boundary == boundary;
		});
		if(it == plans.end()) {
			return RenderTransitionExecutionResult{false, RenderTransitionExecutionError::MissingPlan, RenderResourceTransitionPlan{}};
		}
		return RenderTransitionExecutionResult{true, RenderTransitionExecutionError::None, *it};
	}

	bool RenderTransitionExecutor::HasExecutedBoundary(const RenderResourceTransitionPlan &plan) const {
		if(!plan.boundary.has_value()) {
			return false;
		}
		const RenderBarrierPoint point = ToBarrierPoint(*plan.boundary);
		const std::vector<RenderResourceBarrierRecord> &records = renderGraph_.GetManualBarriers();
		return std::any_of(records.begin(), records.end(), [&](const RenderResourceBarrierRecord &record) {
			return record.resource == plan.resource && record.point == point;
		});
	}

	RenderBarrierPoint RenderTransitionExecutor::ToBarrierPoint(RenderTransitionBoundary boundary) const {
		switch(boundary) {
		case RenderTransitionBoundary::RenderTexturePreDraw:
			return RenderBarrierPoint::RenderTexturePreDraw;
		case RenderTransitionBoundary::RenderTexturePostDraw:
			return RenderBarrierPoint::RenderTexturePostDraw;
		case RenderTransitionBoundary::BeginPresentRenderTarget:
			return RenderBarrierPoint::BeginPresentRenderTarget;
		case RenderTransitionBoundary::BeforePresent:
			return RenderBarrierPoint::BeforePresent;
		}
		return RenderBarrierPoint::RenderTexturePreDraw;
	}

	D3D12_RESOURCE_STATES RenderTransitionExecutor::ToD3D12State(RenderResourceState state) const {
		switch(state) {
		case RenderResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case RenderResourceState::PixelShaderResource:
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case RenderResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case RenderResourceState::DepthRead:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case RenderResourceState::Present:
			return D3D12_RESOURCE_STATE_PRESENT;
		case RenderResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case RenderResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case RenderResourceState::GenericRead:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case RenderResourceState::Unknown:
			break;
		}
		assert(false && "RenderTransitionExecutor received an unknown resource state.");
		return D3D12_RESOURCE_STATE_COMMON;
	}
}

#include "RenderBarrierRecorder.h"

#include <cassert>

namespace MagEngine {
	RenderBarrierRecorder::RenderBarrierRecorder(RenderGraph &renderGraph)
		: renderGraph_(renderGraph) {
	}

	void RenderBarrierRecorder::Transition(
		ID3D12GraphicsCommandList &commandList,
		RenderResourceId resourceId,
		ID3D12Resource &resource,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState,
		RenderBarrierPoint point) {
#ifdef _DEBUG
		assert(beforeState != afterState && "ResourceBarrier transition must change the resource state.");
		assert(ConvertState(beforeState) != RenderResourceState::Unknown &&
			   ConvertState(afterState) != RenderResourceState::Unknown &&
			   "ResourceBarrier transition must use states known by RenderGraph validation.");
		for(const RenderResourceBarrierRecord &record : renderGraph_.GetManualBarriers()) {
			const bool sameSlot = record.resource == resourceId && record.point == point;
			const bool sameTransition = record.beforeState == ConvertState(beforeState) && record.afterState == ConvertState(afterState);
			assert((!sameSlot || sameTransition) &&
				   "The same RenderBarrierPoint must not record conflicting transitions for one resource.");
		}
#endif

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = beforeState;
		barrier.Transition.StateAfter = afterState;
		commandList.ResourceBarrier(1, &barrier);

		renderGraph_.RecordManualBarrier(RenderResourceBarrierRecord{
			resourceId,
			ConvertState(beforeState),
			ConvertState(afterState),
			point,
			GetSequence(point)});
	}

	void RenderBarrierRecorder::Transition(
		ID3D12GraphicsCommandList &commandList,
		ID3D12Resource &resource,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState) {
#ifdef _DEBUG
		assert(beforeState != afterState && "ResourceBarrier transition must change the resource state.");
		assert(ConvertState(beforeState) != RenderResourceState::Unknown &&
			   ConvertState(afterState) != RenderResourceState::Unknown &&
			   "ResourceBarrier transition must use states known by RenderGraph validation.");
#endif

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = &resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = beforeState;
		barrier.Transition.StateAfter = afterState;
		commandList.ResourceBarrier(1, &barrier);
	}

	RenderBarrierRecorderResult RenderBarrierRecorder::TransitionForTesting(
		IResourceBarrierSink &sink,
		RenderResourceId resourceId,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState,
		RenderBarrierPoint point) {
		if(beforeState == afterState) {
			return RenderBarrierRecorderResult{false, RenderBarrierRecorderError::SameState};
		}
		const RenderResourceState beforeGraphState = ConvertState(beforeState);
		const RenderResourceState afterGraphState = ConvertState(afterState);
		if(beforeGraphState == RenderResourceState::Unknown || afterGraphState == RenderResourceState::Unknown) {
			return RenderBarrierRecorderResult{false, RenderBarrierRecorderError::UnknownState};
		}
		for(const RenderResourceBarrierRecord &record : renderGraph_.GetManualBarriers()) {
			const bool sameSlot = record.resource == resourceId && record.point == point;
			const bool sameTransition = record.beforeState == beforeGraphState && record.afterState == afterGraphState;
			if(sameSlot && !sameTransition) {
				return RenderBarrierRecorderResult{false, RenderBarrierRecorderError::ConflictingBarrierPoint};
			}
		}

		sink.ApplyTransition(beforeState, afterState);
		renderGraph_.RecordManualBarrier(RenderResourceBarrierRecord{
			resourceId,
			beforeGraphState,
			afterGraphState,
			point,
			GetSequence(point)});
		return RenderBarrierRecorderResult{};
	}

	RenderResourceState RenderBarrierRecorder::ConvertState(D3D12_RESOURCE_STATES state) const {
		switch(state) {
		case D3D12_RESOURCE_STATE_RENDER_TARGET:
			return RenderResourceState::RenderTarget;
		case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
			return RenderResourceState::PixelShaderResource;
		case D3D12_RESOURCE_STATE_DEPTH_WRITE:
			return RenderResourceState::DepthWrite;
		case D3D12_RESOURCE_STATE_DEPTH_READ:
			return RenderResourceState::DepthRead;
		case D3D12_RESOURCE_STATE_PRESENT:
			return RenderResourceState::Present;
		case D3D12_RESOURCE_STATE_COPY_SOURCE:
			return RenderResourceState::CopySource;
		case D3D12_RESOURCE_STATE_COPY_DEST:
			return RenderResourceState::CopyDest;
		case D3D12_RESOURCE_STATE_GENERIC_READ:
			return RenderResourceState::GenericRead;
		default:
			break;
		}
		return RenderResourceState::Unknown;
	}

	uint32_t RenderBarrierRecorder::GetSequence(RenderBarrierPoint point) const {
		switch(point) {
		case RenderBarrierPoint::RenderTexturePreDraw:
			return 10;
		case RenderBarrierPoint::RenderTexturePostDraw:
			// NOTE: PostOverlay（HUD Line / Particle）の完了後に実行される境界。
			return 185;
		case RenderBarrierPoint::BeginPresentRenderTarget:
			return 166;
		case RenderBarrierPoint::BeforePresent:
			return 1000;
		}
		return 0;
	}
}

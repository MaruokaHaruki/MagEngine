#include "RenderGraph.h"

#include "Renderer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <tuple>
#include <vector>

namespace MagEngine {
	namespace {
		bool IsReadAccess(RenderResourceAccess access) {
			return access == RenderResourceAccess::Read || access == RenderResourceAccess::ReadWrite;
		}

		bool IsWriteAccess(RenderResourceAccess access) {
			return access == RenderResourceAccess::Write || access == RenderResourceAccess::ReadWrite;
		}

		size_t FindPassIndex(const std::vector<RenderPassEntry> &passes, RenderPassId passId) {
			for(size_t i = 0; i < passes.size(); ++i) {
				if(passes[i].id == passId) {
					return i;
				}
			}
			assert(false && "RenderGraph dependency references an unknown pass.");
			return passes.size();
		}

		std::optional<RenderTransitionBoundary> ToTransitionBoundary(RenderBarrierPoint point) {
			switch(point) {
			case RenderBarrierPoint::RenderTexturePreDraw:
				return RenderTransitionBoundary::RenderTexturePreDraw;
			case RenderBarrierPoint::RenderTexturePostDraw:
				return RenderTransitionBoundary::RenderTexturePostDraw;
			case RenderBarrierPoint::BeginPresentRenderTarget:
				return RenderTransitionBoundary::BeginPresentRenderTarget;
			case RenderBarrierPoint::BeforePresent:
				return RenderTransitionBoundary::BeforePresent;
			}
			return std::nullopt;
		}

		std::optional<RenderTransitionBoundary> InferTransitionBoundary(
			RenderResourceId resource,
			RenderResourceState beforeState,
			RenderResourceState afterState,
			std::optional<RenderPassId> afterPass,
			bool isFinalTransition) {
			if(resource == RenderResourceId::SceneColor &&
			   beforeState == RenderResourceState::PixelShaderResource &&
			   afterState == RenderResourceState::RenderTarget) {
				return RenderTransitionBoundary::RenderTexturePreDraw;
			}
			if(resource == RenderResourceId::SceneColor &&
			   beforeState == RenderResourceState::RenderTarget &&
			   afterState == RenderResourceState::PixelShaderResource) {
				return RenderTransitionBoundary::RenderTexturePostDraw;
			}
			if(resource == RenderResourceId::PresentColor &&
			   beforeState == RenderResourceState::Present &&
			   afterState == RenderResourceState::RenderTarget &&
			   afterPass == RenderPassId::PostEffect) {
				return RenderTransitionBoundary::BeginPresentRenderTarget;
			}
			if(resource == RenderResourceId::PresentColor &&
			   beforeState == RenderResourceState::RenderTarget &&
			   afterState == RenderResourceState::Present &&
			   isFinalTransition) {
				return RenderTransitionBoundary::BeforePresent;
			}
			return std::nullopt;
		}

		bool IsSameTransition(const RenderResourceTransitionPlan &plan, const RenderResourceBarrierRecord &record) {
			return plan.resource == record.resource &&
				   plan.beforeState == record.beforeState &&
				   plan.afterState == record.afterState &&
				   plan.boundary == ToTransitionBoundary(record.point);
		}

		uint32_t GetBoundarySequence(RenderTransitionBoundary boundary) {
			switch(boundary) {
			case RenderTransitionBoundary::RenderTexturePreDraw:
				return 10u;
		case RenderTransitionBoundary::RenderTexturePostDraw:
			// NOTE: PostOverlay（HUD Line / Particle）の完了後にSceneColorをSRVへ戻す。
			return 185u;
			case RenderTransitionBoundary::BeginPresentRenderTarget:
				return 166u;
			case RenderTransitionBoundary::BeforePresent:
				return 1000u;
			}
			return 0u;
		}
	}

	bool RenderGraphValidationResult::HasError(RenderGraphValidationError error) const {
		return std::find(errors.begin(), errors.end(), error) != errors.end();
	}

	std::string_view ToString(RenderResourceId resourceId) {
		switch(resourceId) {
		case RenderResourceId::SceneColor:
			return "SceneColor";
		case RenderResourceId::SceneDepth:
			return "SceneDepth";
		case RenderResourceId::PresentColor:
			return "PresentColor";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderResourceAccess access) {
		switch(access) {
		case RenderResourceAccess::Read:
			return "Read";
		case RenderResourceAccess::Write:
			return "Write";
		case RenderResourceAccess::ReadWrite:
			return "ReadWrite";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderResourceState state) {
		switch(state) {
		case RenderResourceState::Unknown:
			return "Unknown";
		case RenderResourceState::RenderTarget:
			return "RenderTarget";
		case RenderResourceState::PixelShaderResource:
			return "PixelShaderResource";
		case RenderResourceState::DepthWrite:
			return "DepthWrite";
		case RenderResourceState::DepthRead:
			return "DepthRead";
		case RenderResourceState::Present:
			return "Present";
		case RenderResourceState::CopySource:
			return "CopySource";
		case RenderResourceState::CopyDest:
			return "CopyDest";
		case RenderResourceState::GenericRead:
			return "GenericRead";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderBarrierPoint point) {
		switch(point) {
		case RenderBarrierPoint::RenderTexturePreDraw:
			return "RenderTexturePreDraw";
		case RenderBarrierPoint::RenderTexturePostDraw:
			return "RenderTexturePostDraw";
		case RenderBarrierPoint::BeginPresentRenderTarget:
			return "BeginPresentRenderTarget";
		case RenderBarrierPoint::BeforePresent:
			return "BeforePresent";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderTransitionBoundary boundary) {
		switch(boundary) {
		case RenderTransitionBoundary::RenderTexturePreDraw:
			return "RenderTexturePreDraw";
		case RenderTransitionBoundary::RenderTexturePostDraw:
			return "RenderTexturePostDraw";
		case RenderTransitionBoundary::BeginPresentRenderTarget:
			return "BeginPresentRenderTarget";
		case RenderTransitionBoundary::BeforePresent:
			return "BeforePresent";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderGraphValidationError error) {
		switch(error) {
		case RenderGraphValidationError::MissingInitialState:
			return "MissingInitialState";
		case RenderGraphValidationError::MissingRequiredState:
			return "MissingRequiredState";
		case RenderGraphValidationError::MissingWriter:
			return "MissingWriter";
		case RenderGraphValidationError::DuplicateUsage:
			return "DuplicateUsage";
		case RenderGraphValidationError::InvalidBarrierState:
			return "InvalidBarrierState";
		case RenderGraphValidationError::InvalidBarrierBeforeState:
			return "InvalidBarrierBeforeState";
		case RenderGraphValidationError::MissingBarrier:
			return "MissingBarrier";
		case RenderGraphValidationError::FinalStateMismatch:
			return "FinalStateMismatch";
		case RenderGraphValidationError::DependencyOrderMismatch:
			return "DependencyOrderMismatch";
		case RenderGraphValidationError::CircularDependency:
			return "CircularDependency";
		case RenderGraphValidationError::WriteConflict:
			return "WriteConflict";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderTransitionMismatchType type) {
		switch(type) {
		case RenderTransitionMismatchType::Resource:
			return "Resource";
		case RenderTransitionMismatchType::BeforeState:
			return "BeforeState";
		case RenderTransitionMismatchType::AfterState:
			return "AfterState";
		case RenderTransitionMismatchType::Sequence:
			return "Sequence";
		case RenderTransitionMismatchType::Boundary:
			return "Boundary";
		}
		return "Unknown";
	}

	void RenderGraph::AddExternalResource(RenderResourceId resourceId) {
		if(IsExternalResource(resourceId)) {
			return;
		}
		externalResources_.push_back(resourceId);
	}

	void RenderGraph::SetInitialResourceState(RenderResourceId resourceId, RenderResourceState state) {
		for(RenderResourceStateRecord &record : initialResourceStates_) {
			if(record.resource == resourceId) {
				record.state = state;
				return;
			}
		}
		initialResourceStates_.push_back(RenderResourceStateRecord{resourceId, state});
	}

	void RenderGraph::SetFinalResourceState(RenderResourceId resourceId, RenderResourceState state) {
		for(RenderResourceStateRecord &record : finalResourceStates_) {
			if(record.resource == resourceId) {
				record.state = state;
				return;
			}
		}
		finalResourceStates_.push_back(RenderResourceStateRecord{resourceId, state});
	}

	void RenderGraph::RecordManualBarrier(const RenderResourceBarrierRecord &record) {
		manualBarriers_.push_back(record);
	}

	void RenderGraph::Build(const std::vector<RenderPassEntry> &passes) {
		dependencies_.clear();

		struct LastWriter {
			RenderResourceId resource;
			RenderPassId pass;
		};
		std::vector<LastWriter> lastWriters;

		for(const RenderPassEntry &pass : passes) {
			if(!pass.enabled) {
				continue;
			}

			ValidateDuplicateUsages(pass);

			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				const auto writer = std::find_if(lastWriters.begin(), lastWriters.end(), [&](const LastWriter &lastWriter) {
					return lastWriter.resource == usage.resource;
				});
				const bool hasWriter = writer != lastWriters.end();

				if(IsReadAccess(usage.access)) {
					// NOTE: 外部初期化も先行WriterもないReadは、将来のBarrier自動化で破綻するため登録時点で検出する。
					assert((hasWriter || IsExternalResource(usage.resource)) && "RenderGraph read requires a previous writer or an external resource.");
					if(hasWriter) {
						AddDependency(writer->pass, pass.id, usage.resource);
					}
				}

				if(IsWriteAccess(usage.access)) {
					if(hasWriter) {
						AddDependency(writer->pass, pass.id, usage.resource);
						writer->pass = pass.id;
					} else {
						lastWriters.push_back(LastWriter{usage.resource, pass.id});
					}
				}
			}
		}
	}

	void RenderGraph::Validate(const std::vector<RenderPassEntry> &passes) const {
		ValidateWriteConflict(passes);
		ValidateAcyclic(passes);
		ValidateExecutionOrder(passes);
		ValidateRequiredStates(passes);
	}

	RenderGraphValidationResult RenderGraph::ValidateForTesting(const std::vector<RenderPassEntry> &passes) const {
		RenderGraphValidationResult result;

		for(const RenderPassEntry &pass : passes) {
			if(!pass.enabled) {
				continue;
			}
			for(size_t i = 0; i < pass.resourceUsages.size(); ++i) {
				if(pass.resourceUsages[i].requiredState == RenderResourceState::Unknown) {
					result.AddError(RenderGraphValidationError::MissingRequiredState);
				}
				for(size_t j = i + 1; j < pass.resourceUsages.size(); ++j) {
					if(pass.resourceUsages[i].resource == pass.resourceUsages[j].resource) {
						result.AddError(RenderGraphValidationError::DuplicateUsage);
					}
				}
			}
		}

		for(size_t i = 0; i < passes.size(); ++i) {
			if(!passes[i].enabled) {
				continue;
			}
			for(size_t j = i + 1; j < passes.size(); ++j) {
				if(!passes[j].enabled || passes[i].phase != passes[j].phase || passes[i].order != passes[j].order) {
					continue;
				}
				for(const RenderPassResourceUsage &lhs : passes[i].resourceUsages) {
					if(!IsWriteAccess(lhs.access)) {
						continue;
					}
					for(const RenderPassResourceUsage &rhs : passes[j].resourceUsages) {
						if(lhs.resource == rhs.resource && IsWriteAccess(rhs.access)) {
							result.AddError(RenderGraphValidationError::WriteConflict);
						}
					}
				}
			}
		}

		struct LastWriter {
			RenderResourceId resource;
			RenderPassId pass;
		};
		std::vector<LastWriter> lastWriters;
		std::vector<RenderPassDependency> dependencies = dependencies_;

		for(const RenderPassEntry &pass : passes) {
			if(!pass.enabled) {
				continue;
			}
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				const auto writer = std::find_if(lastWriters.begin(), lastWriters.end(), [&](const LastWriter &lastWriter) {
					return lastWriter.resource == usage.resource;
				});
				const bool hasWriter = writer != lastWriters.end();

				if(IsReadAccess(usage.access) && !hasWriter && !IsExternalResource(usage.resource)) {
					result.AddError(RenderGraphValidationError::MissingWriter);
				}
				if(IsReadAccess(usage.access) && hasWriter) {
					dependencies.push_back(RenderPassDependency{writer->pass, pass.id, usage.resource});
				}
				if(IsWriteAccess(usage.access)) {
					if(hasWriter) {
						dependencies.push_back(RenderPassDependency{writer->pass, pass.id, usage.resource});
						writer->pass = pass.id;
					} else {
						lastWriters.push_back(LastWriter{usage.resource, pass.id});
					}
				}
			}
		}

		for(const RenderPassDependency &dependency : dependencies) {
			const size_t beforeIndex = FindPassIndex(passes, dependency.before);
			const size_t afterIndex = FindPassIndex(passes, dependency.after);
			if(beforeIndex >= afterIndex) {
				result.AddError(RenderGraphValidationError::DependencyOrderMismatch);
			}
		}

		std::vector<uint8_t> states(passes.size(), 0);
		auto visit = [&](auto &&self, size_t index) -> bool {
			if(states[index] == 1) {
				return false;
			}
			if(states[index] == 2) {
				return true;
			}
			states[index] = 1;
			for(const RenderPassDependency &dependency : dependencies) {
				if(dependency.before != passes[index].id) {
					continue;
				}
				const size_t nextIndex = FindPassIndex(passes, dependency.after);
				if(!self(self, nextIndex)) {
					return false;
				}
			}
			states[index] = 2;
			return true;
		};
		for(size_t i = 0; i < passes.size(); ++i) {
			if(!visit(visit, i)) {
				result.AddError(RenderGraphValidationError::CircularDependency);
				break;
			}
		}

		for(const RenderPassEntry &pass : passes) {
			if(!pass.enabled) {
				continue;
			}
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				if(!FindInitialState(usage.resource).has_value()) {
					result.AddError(RenderGraphValidationError::MissingInitialState);
				}
			}
		}
		for(const RenderResourceBarrierRecord &barrier : manualBarriers_) {
			if(barrier.beforeState == RenderResourceState::Unknown || barrier.afterState == RenderResourceState::Unknown) {
				result.AddError(RenderGraphValidationError::InvalidBarrierState);
			}
			if(!FindInitialState(barrier.resource).has_value()) {
				result.AddError(RenderGraphValidationError::MissingInitialState);
			}
		}

		return result;
	}

	void RenderGraph::ClearRecordedBarriers() {
		manualBarriers_.clear();
	}

	void RenderGraph::ValidateRecordedResourceStates(const std::vector<RenderPassEntry> &passes) const {
		ValidateResourceStates(passes);
	}

	RenderGraphValidationResult RenderGraph::ValidateRecordedResourceStatesForTesting(const std::vector<RenderPassEntry> &passes) const {
		struct StateEvent {
			enum class Kind : uint8_t {
				Barrier,
				Pass,
			};

			uint32_t sequence = 0;
			Kind kind = Kind::Pass;
			size_t index = 0;
		};

		RenderGraphValidationResult result = ValidateForTesting(passes);
		std::vector<RenderResourceStateRecord> currentStates = initialResourceStates_;
		std::vector<RenderResourceBarrierRecord> barriers = manualBarriers_;
		std::sort(barriers.begin(), barriers.end(), [](const RenderResourceBarrierRecord &a, const RenderResourceBarrierRecord &b) {
			return std::tuple{a.sequence, a.resource, a.beforeState, a.afterState} <
				   std::tuple{b.sequence, b.resource, b.beforeState, b.afterState};
		});

		std::vector<StateEvent> events;
		for(size_t i = 0; i < barriers.size(); ++i) {
			events.push_back(StateEvent{barriers[i].sequence, StateEvent::Kind::Barrier, i});
		}
		for(size_t i = 0; i < passes.size(); ++i) {
			if(passes[i].enabled) {
				events.push_back(StateEvent{100u + static_cast<uint32_t>(i) * 10u, StateEvent::Kind::Pass, i});
			}
		}
		std::sort(events.begin(), events.end(), [](const StateEvent &a, const StateEvent &b) {
			return std::tuple{a.sequence, a.kind, a.index} < std::tuple{b.sequence, b.kind, b.index};
		});

		for(const StateEvent &event : events) {
			if(event.kind == StateEvent::Kind::Barrier) {
				const RenderResourceBarrierRecord &barrier = barriers[event.index];
				auto stateIt = std::find_if(currentStates.begin(), currentStates.end(), [&](const RenderResourceStateRecord &record) {
					return record.resource == barrier.resource;
				});
				if(stateIt == currentStates.end()) {
					result.AddError(RenderGraphValidationError::MissingInitialState);
					continue;
				}
				if(stateIt->state != barrier.beforeState) {
					result.AddError(RenderGraphValidationError::InvalidBarrierBeforeState);
				}
				stateIt->state = barrier.afterState;
				continue;
			}

			const RenderPassEntry &pass = passes[event.index];
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				auto stateIt = std::find_if(currentStates.begin(), currentStates.end(), [&](const RenderResourceStateRecord &record) {
					return record.resource == usage.resource;
				});
				if(stateIt == currentStates.end()) {
					result.AddError(RenderGraphValidationError::MissingInitialState);
					continue;
				}
				if(stateIt->state != usage.requiredState) {
					result.AddError(RenderGraphValidationError::MissingBarrier);
				}
			}
		}

		for(const RenderResourceStateRecord &finalState : finalResourceStates_) {
			auto stateIt = std::find_if(currentStates.begin(), currentStates.end(), [&](const RenderResourceStateRecord &record) {
				return record.resource == finalState.resource;
			});
			if(stateIt == currentStates.end()) {
				result.AddError(RenderGraphValidationError::MissingInitialState);
				continue;
			}
			if(stateIt->state != finalState.state) {
				result.AddError(RenderGraphValidationError::FinalStateMismatch);
			}
		}

		return result;
	}

	std::vector<RenderResourceTransitionPlan> RenderGraph::BuildTransitionPlan(const std::vector<RenderPassEntry> &passes) const {
		std::vector<RenderResourceTransitionPlan> plans;
		std::vector<RenderResourceStateRecord> currentStates = initialResourceStates_;

		auto findState = [&](RenderResourceId resourceId) -> RenderResourceState * {
			const auto it = std::find_if(currentStates.begin(), currentStates.end(), [&](const RenderResourceStateRecord &record) {
				return record.resource == resourceId;
			});
			if(it == currentStates.end()) {
				return nullptr;
			}
			return &it->state;
		};

		auto appendPlan = [&](RenderResourceId resourceId,
							  RenderResourceState beforeState,
							  RenderResourceState afterState,
							  std::optional<RenderPassId> beforePass,
							  std::optional<RenderPassId> afterPass,
							  uint32_t sequenceIndex,
							  bool isFinalTransition) {
			if(beforeState == afterState) {
				return;
			}
			const std::optional<RenderTransitionBoundary> boundary = InferTransitionBoundary(resourceId, beforeState, afterState, afterPass, isFinalTransition);
			plans.push_back(RenderResourceTransitionPlan{
				resourceId,
				beforeState,
				afterState,
				beforePass,
				afterPass,
				boundary,
				boundary.has_value() ? GetBoundarySequence(*boundary) : sequenceIndex});
		};

		std::vector<size_t> sortedPassIndices;
		sortedPassIndices.reserve(passes.size());
		for(size_t i = 0; i < passes.size(); ++i) {
			sortedPassIndices.push_back(i);
		}
		std::sort(sortedPassIndices.begin(), sortedPassIndices.end(), [&](size_t lhs, size_t rhs) {
			const RenderPassEntry &a = passes[lhs];
			const RenderPassEntry &b = passes[rhs];
			return std::tuple{a.phase, a.order, a.id} < std::tuple{b.phase, b.order, b.id};
		});

		std::vector<RenderResourceStateRecord> lastWriterStates;
		struct LastWriterPass {
			RenderResourceId resource;
			RenderPassId pass;
		};
		std::vector<LastWriterPass> lastWriterPasses;

		for(size_t sortedIndex = 0; sortedIndex < sortedPassIndices.size(); ++sortedIndex) {
			const RenderPassEntry &pass = passes[sortedPassIndices[sortedIndex]];
			if(!pass.enabled) {
				continue;
			}

			const uint32_t passSequence = 100u + static_cast<uint32_t>(sortedIndex) * 10u;
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				RenderResourceState *state = findState(usage.resource);
				if(!state || usage.requiredState == RenderResourceState::Unknown) {
					continue;
				}

				std::optional<RenderPassId> beforePass;
				const auto lastWriter = std::find_if(lastWriterPasses.begin(), lastWriterPasses.end(), [&](const LastWriterPass &record) {
					return record.resource == usage.resource;
				});
				if(lastWriter != lastWriterPasses.end()) {
					beforePass = lastWriter->pass;
				}

				appendPlan(usage.resource, *state, usage.requiredState, beforePass, pass.id, passSequence - 1u, false);
				*state = usage.requiredState;

				if(IsWriteAccess(usage.access)) {
					if(lastWriter != lastWriterPasses.end()) {
						lastWriter->pass = pass.id;
					} else {
						lastWriterPasses.push_back(LastWriterPass{usage.resource, pass.id});
					}
				}
			}
		}

		for(const RenderResourceStateRecord &finalState : finalResourceStates_) {
			RenderResourceState *state = findState(finalState.resource);
			if(!state) {
				continue;
			}
			std::optional<RenderPassId> beforePass;
			const auto lastWriter = std::find_if(lastWriterPasses.begin(), lastWriterPasses.end(), [&](const LastWriterPass &record) {
				return record.resource == finalState.resource;
			});
			if(lastWriter != lastWriterPasses.end()) {
				beforePass = lastWriter->pass;
			}
			appendPlan(finalState.resource, *state, finalState.state, beforePass, std::nullopt, 300u, true);
			*state = finalState.state;
		}

		std::sort(plans.begin(), plans.end(), [](const RenderResourceTransitionPlan &a, const RenderResourceTransitionPlan &b) {
			return std::tuple{a.sequenceIndex, a.resource, a.beforeState, a.afterState} <
				   std::tuple{b.sequenceIndex, b.resource, b.beforeState, b.afterState};
		});
		return plans;
	}

	RenderTransitionPlanComparisonResult RenderGraph::CompareTransitionPlanWithManualBarriers(const std::vector<RenderPassEntry> &passes) const {
		RenderTransitionPlanComparisonResult result;
		const std::vector<RenderResourceTransitionPlan> plans = BuildTransitionPlan(passes);
		std::vector<RenderResourceBarrierRecord> records = manualBarriers_;
		std::sort(records.begin(), records.end(), [](const RenderResourceBarrierRecord &a, const RenderResourceBarrierRecord &b) {
			return std::tuple{a.sequence, a.resource, a.beforeState, a.afterState} <
				   std::tuple{b.sequence, b.resource, b.beforeState, b.afterState};
		});

		std::vector<bool> matchedRecords(records.size(), false);
		for(const RenderResourceTransitionPlan &plan : plans) {
			const auto exactIt = std::find_if(records.begin(), records.end(), [&](const RenderResourceBarrierRecord &record) {
				const size_t index = static_cast<size_t>(&record - records.data());
				return !matchedRecords[index] && IsSameTransition(plan, record);
			});
			if(exactIt != records.end()) {
				const size_t recordIndex = static_cast<size_t>(exactIt - records.begin());
				matchedRecords[recordIndex] = true;
				if(plan.sequenceIndex != exactIt->sequence) {
					result.mismatches.push_back(RenderTransitionMismatch{RenderTransitionMismatchType::Sequence, plan, *exactIt});
					result.MarkMismatch();
				}
				continue;
			}

			const auto relatedIt = std::find_if(records.begin(), records.end(), [&](const RenderResourceBarrierRecord &record) {
				const size_t index = static_cast<size_t>(&record - records.data());
				return !matchedRecords[index] &&
					   plan.resource == record.resource &&
					   plan.boundary == ToTransitionBoundary(record.point);
			});
			if(relatedIt == records.end()) {
				const auto boundaryMismatchIt = std::find_if(records.begin(), records.end(), [&](const RenderResourceBarrierRecord &record) {
					const size_t index = static_cast<size_t>(&record - records.data());
					return !matchedRecords[index] &&
						   plan.resource == record.resource &&
						   plan.beforeState == record.beforeState &&
						   plan.afterState == record.afterState;
				});
				if(boundaryMismatchIt != records.end()) {
					matchedRecords[static_cast<size_t>(boundaryMismatchIt - records.begin())] = true;
					result.mismatches.push_back(RenderTransitionMismatch{RenderTransitionMismatchType::Boundary, plan, *boundaryMismatchIt});
					result.MarkMismatch();
					continue;
				}
			}
			if(relatedIt == records.end()) {
				result.missingManualBarriers.push_back(plan);
				result.MarkMismatch();
				continue;
			}

			matchedRecords[static_cast<size_t>(relatedIt - records.begin())] = true;
			if(plan.beforeState != relatedIt->beforeState) {
				result.mismatches.push_back(RenderTransitionMismatch{RenderTransitionMismatchType::BeforeState, plan, *relatedIt});
				result.MarkMismatch();
			}
			if(plan.afterState != relatedIt->afterState) {
				result.mismatches.push_back(RenderTransitionMismatch{RenderTransitionMismatchType::AfterState, plan, *relatedIt});
				result.MarkMismatch();
			}
			if(plan.sequenceIndex != relatedIt->sequence) {
				result.mismatches.push_back(RenderTransitionMismatch{RenderTransitionMismatchType::Sequence, plan, *relatedIt});
				result.MarkMismatch();
			}
		}

		for(size_t i = 0; i < records.size(); ++i) {
			if(matchedRecords[i]) {
				continue;
			}
			result.unexpectedManualBarriers.push_back(records[i]);
			result.MarkMismatch();
		}

		return result;
	}

	void RenderGraph::AddDependencyForTesting(RenderPassId before, RenderPassId after, RenderResourceId resource) {
		AddDependency(before, after, resource);
	}

	void RenderGraph::RecordManualBarrierForTesting(const RenderResourceBarrierRecord &record) {
		RecordManualBarrier(record);
	}

	bool RenderGraph::IsExternalResource(RenderResourceId resourceId) const {
		return std::find(externalResources_.begin(), externalResources_.end(), resourceId) != externalResources_.end();
	}

	std::optional<RenderResourceState> RenderGraph::FindInitialState(RenderResourceId resourceId) const {
		const auto it = std::find_if(initialResourceStates_.begin(), initialResourceStates_.end(), [&](const RenderResourceStateRecord &record) {
			return record.resource == resourceId;
		});
		if(it == initialResourceStates_.end()) {
			return std::nullopt;
		}
		return it->state;
	}

	std::optional<RenderResourceState> RenderGraph::FindFinalState(RenderResourceId resourceId) const {
		const auto it = std::find_if(finalResourceStates_.begin(), finalResourceStates_.end(), [&](const RenderResourceStateRecord &record) {
			return record.resource == resourceId;
		});
		if(it == finalResourceStates_.end()) {
			return std::nullopt;
		}
		return it->state;
	}

	RenderResourceState &RenderGraph::FindCurrentState(std::vector<RenderResourceStateRecord> &states, RenderResourceId resourceId) const {
		const auto it = std::find_if(states.begin(), states.end(), [&](const RenderResourceStateRecord &record) {
			return record.resource == resourceId;
		});
		assert(it != states.end() && "RenderGraph resource state validation requires an initial state for every used resource.");
		return it->state;
	}

	bool RenderGraph::HasDependency(const RenderPassDependency &dependency) const {
		return std::any_of(dependencies_.begin(), dependencies_.end(), [&](const RenderPassDependency &registered) {
			return registered.before == dependency.before &&
				   registered.after == dependency.after &&
				   registered.resource == dependency.resource;
		});
	}

	void RenderGraph::AddDependency(RenderPassId before, RenderPassId after, RenderResourceId resource) {
		if(before == after) {
			return;
		}

		RenderPassDependency dependency{before, after, resource};
		if(!HasDependency(dependency)) {
			dependencies_.push_back(dependency);
		}
	}

	void RenderGraph::ValidateDuplicateUsages(const RenderPassEntry &pass) const {
		for(size_t i = 0; i < pass.resourceUsages.size(); ++i) {
			for(size_t j = i + 1; j < pass.resourceUsages.size(); ++j) {
				assert(pass.resourceUsages[i].resource != pass.resourceUsages[j].resource &&
					   "RenderPassResourceUsage must not declare the same resource twice. Use ReadWrite instead.");
			}
		}
	}

	void RenderGraph::ValidateWriteConflict(const std::vector<RenderPassEntry> &passes) const {
		for(size_t i = 0; i < passes.size(); ++i) {
			if(!passes[i].enabled) {
				continue;
			}
			for(size_t j = i + 1; j < passes.size(); ++j) {
				if(!passes[j].enabled ||
				   passes[i].phase != passes[j].phase ||
				   passes[i].order != passes[j].order) {
					continue;
				}

				for(const RenderPassResourceUsage &lhs : passes[i].resourceUsages) {
					if(!IsWriteAccess(lhs.access)) {
						continue;
					}
					for(const RenderPassResourceUsage &rhs : passes[j].resourceUsages) {
						assert(!(lhs.resource == rhs.resource && IsWriteAccess(rhs.access)) &&
							   "RenderGraph detected an unordered write conflict.");
					}
				}
			}
		}
	}

	void RenderGraph::ValidateAcyclic(const std::vector<RenderPassEntry> &passes) const {
		std::vector<uint8_t> states(passes.size(), 0);

		auto visit = [&](auto &&self, size_t index) -> bool {
			if(states[index] == 1) {
				return false;
			}
			if(states[index] == 2) {
				return true;
			}

			states[index] = 1;
			for(const RenderPassDependency &dependency : dependencies_) {
				if(dependency.before != passes[index].id) {
					continue;
				}
				const size_t nextIndex = FindPassIndex(passes, dependency.after);
				if(!self(self, nextIndex)) {
					return false;
				}
			}
			states[index] = 2;
			return true;
		};

		for(size_t i = 0; i < passes.size(); ++i) {
			assert(visit(visit, i) && "RenderGraph dependency cycle detected.");
		}
	}

	void RenderGraph::ValidateExecutionOrder(const std::vector<RenderPassEntry> &passes) const {
		for(const RenderPassDependency &dependency : dependencies_) {
			const size_t beforeIndex = FindPassIndex(passes, dependency.before);
			const size_t afterIndex = FindPassIndex(passes, dependency.after);
			assert(beforeIndex < afterIndex && "RenderGraph dependency conflicts with the current phase/order execution order.");
		}
	}

	void RenderGraph::ValidateRequiredStates(const std::vector<RenderPassEntry> &passes) const {
		for(const RenderPassEntry &pass : passes) {
			if(!pass.enabled) {
				continue;
			}
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				assert(usage.requiredState != RenderResourceState::Unknown &&
					   "RenderPassResourceUsage::requiredState must be set.");
				assert(FindInitialState(usage.resource).has_value() &&
					   "Every used RenderResourceId must have an initial resource state.");
			}
		}

		for(const RenderResourceBarrierRecord &barrier : manualBarriers_) {
			assert(barrier.beforeState != RenderResourceState::Unknown &&
				   barrier.afterState != RenderResourceState::Unknown &&
				   "Manual barrier records must declare before/after states.");
			assert(FindInitialState(barrier.resource).has_value() &&
				   "Manual barrier resource must have an initial resource state.");
		}
	}

	void RenderGraph::ValidateResourceStates(const std::vector<RenderPassEntry> &passes) const {
		struct StateEvent {
			enum class Kind : uint8_t {
				Barrier,
				Pass,
			};

			uint32_t sequence = 0;
			Kind kind = Kind::Pass;
			size_t index = 0;
		};

		std::vector<RenderResourceStateRecord> currentStates = initialResourceStates_;
		std::vector<RenderResourceBarrierRecord> barriers = manualBarriers_;
		std::sort(barriers.begin(), barriers.end(), [](const RenderResourceBarrierRecord &a, const RenderResourceBarrierRecord &b) {
			return std::tuple{a.sequence, a.resource, a.beforeState, a.afterState} <
				   std::tuple{b.sequence, b.resource, b.beforeState, b.afterState};
		});

		std::vector<StateEvent> events;
		for(size_t i = 0; i < barriers.size(); ++i) {
			events.push_back(StateEvent{barriers[i].sequence, StateEvent::Kind::Barrier, i});
		}
		for(size_t i = 0; i < passes.size(); ++i) {
			if(!passes[i].enabled) {
				continue;
			}
			// NOTE: 手動Barrierの前後関係を崩さずに検証するため、現在の固定実行順を検証用シーケンスに写す。
			const uint32_t sequence = 100u + static_cast<uint32_t>(i) * 10u;
			events.push_back(StateEvent{sequence, StateEvent::Kind::Pass, i});
		}

		std::sort(events.begin(), events.end(), [](const StateEvent &a, const StateEvent &b) {
			return std::tuple{a.sequence, a.kind, a.index} < std::tuple{b.sequence, b.kind, b.index};
		});

		for(const StateEvent &event : events) {
			if(event.kind == StateEvent::Kind::Barrier) {
				const RenderResourceBarrierRecord &barrier = barriers[event.index];
				RenderResourceState &state = FindCurrentState(currentStates, barrier.resource);
				assert(state == barrier.beforeState &&
					   "Manual barrier beforeState does not match the current tracked resource state.");
				state = barrier.afterState;
				continue;
			}

			const RenderPassEntry &pass = passes[event.index];
			for(const RenderPassResourceUsage &usage : pass.resourceUsages) {
				const RenderResourceState &state = FindCurrentState(currentStates, usage.resource);
				assert(state == usage.requiredState &&
					   "RenderPass requiredState does not match the current tracked resource state. Check manual barrier records.");
			}
		}

		for(const RenderResourceStateRecord &finalState : finalResourceStates_) {
			const RenderResourceState &state = FindCurrentState(currentStates, finalState.resource);
			assert(state == finalState.state &&
				   "Final resource state does not match the tracked resource state.");
		}
	}
}

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

	void RenderGraph::ClearRecordedBarriers() {
		manualBarriers_.clear();
	}

	void RenderGraph::ValidateRecordedResourceStates(const std::vector<RenderPassEntry> &passes) const {
		ValidateResourceStates(passes);
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

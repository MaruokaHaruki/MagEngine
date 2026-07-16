#include "Renderer.h"

#include "CloudRenderPass.h"
#include "DirectXCore.h"
#include "IRenderPass.h"
#include "LineRenderPass.h"
#include "OpaqueRenderPass.h"
#include "ParticleRenderPass.h"
#include "PostEffectRenderPass.h"
#include "RenderContext.h"
#include "RenderWorld.h"
#include "SkyboxRenderPass.h"
#include "SpriteRenderPass.h"
#include "TextRenderPass.h"
#include "TrailRenderPass.h"
#include "Logger.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <tuple>

namespace MagEngine {
	namespace {
		constexpr int32_t kSceneSkyboxOrder = 100;
		constexpr int32_t kSceneOpaqueOrder = 200;
		constexpr int32_t kSceneCloudOrder = 300;
		constexpr int32_t kSceneTrailOrder = 400;
		constexpr int32_t kSceneLineOrder = 450;
		constexpr int32_t kPostOverlayHudLineOrder = 50;
		constexpr int32_t kOverlaySpriteOrder = 100;
		constexpr int32_t kOverlayTextOrder = 110;
		constexpr int32_t kPostOverlayParticleOrder = 100;
		constexpr int32_t kPostProcessPostEffectOrder = 100;

		RenderBarrierPoint ToBarrierPoint(RenderTransitionBoundary boundary) {
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

		bool HasExecutedPlan(
			const std::vector<RenderResourceBarrierRecord> &barriers,
			const RenderResourceTransitionPlan &plan) {
			if(!plan.boundary.has_value()) {
				return false;
			}
			const RenderBarrierPoint point = ToBarrierPoint(*plan.boundary);
			return std::any_of(barriers.begin(), barriers.end(), [&](const RenderResourceBarrierRecord &barrier) {
				return barrier.resource == plan.resource &&
					   barrier.beforeState == plan.beforeState &&
					   barrier.afterState == plan.afterState &&
					   barrier.point == point;
			});
		}
	}

	std::string_view ToString(RenderPassId passId) {
		switch(passId) {
		case RenderPassId::Skybox:
			return "Skybox";
		case RenderPassId::Opaque:
			return "Opaque";
		case RenderPassId::Cloud:
			return "Cloud";
		case RenderPassId::Trail:
			return "Trail";
		case RenderPassId::Sprite:
			return "Sprite";
		case RenderPassId::Particle:
			return "Particle";
		case RenderPassId::Line:
			return "Line";
		case RenderPassId::HudLine:
			return "HudLine";
		case RenderPassId::Text:
			return "Text";
		case RenderPassId::PostEffect:
			return "PostEffect";
		}
		return "Unknown";
	}

	std::string_view ToString(RenderPhase phase) {
		switch(phase) {
		case RenderPhase::Scene:
			return "Scene";
		case RenderPhase::Overlay:
			return "Overlay";
		case RenderPhase::PostOverlay:
			return "PostOverlay";
		case RenderPhase::PostProcess:
			return "PostProcess";
		}
		return "Unknown";
	}

	void Renderer::Initialize(SkyboxSetup &skyboxSetup, Object3dSetup &object3dSetup, CloudSetup &cloudSetup, TrailEffectSetup &trailEffectSetup, SpriteSetup &spriteSetup, ParticleSetup &particleSetup, LineManager &lineManager, DirectXCore &dxCore, PostEffectManager &postEffectManager, TextureManager &textureManager) {
		renderPasses_.clear();
		renderTransitionExecutor_.reset();
		renderGraph_ = RenderGraph{};
		renderGraph_.AddExternalResource(RenderResourceId::SceneColor);
		renderGraph_.AddExternalResource(RenderResourceId::SceneDepth);
		renderGraph_.AddExternalResource(RenderResourceId::PresentColor);
		renderGraph_.SetInitialResourceState(RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource);
		renderGraph_.SetInitialResourceState(RenderResourceId::SceneDepth, RenderResourceState::DepthWrite);
		renderGraph_.SetInitialResourceState(RenderResourceId::PresentColor, RenderResourceState::Present);
		renderGraph_.SetFinalResourceState(RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource);
		renderGraph_.SetFinalResourceState(RenderResourceId::SceneDepth, RenderResourceState::DepthWrite);
		renderGraph_.SetFinalResourceState(RenderResourceId::PresentColor, RenderResourceState::Present);
		dxCore.SetRenderBarrierRecorder(&renderBarrierRecorder_);

		AddPass(RenderPassEntry{
			RenderPassId::Skybox,
			RenderPhase::Scene,
			kSceneSkyboxOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			},
			std::make_unique<SkyboxRenderPass>(skyboxSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Opaque,
			RenderPhase::Scene,
			kSceneOpaqueOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			},
			std::make_unique<OpaqueRenderPass>(object3dSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Cloud,
			RenderPhase::Scene,
			kSceneCloudOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			},
			std::make_unique<CloudRenderPass>(cloudSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Trail,
			RenderPhase::Scene,
			kSceneTrailOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			},
			std::make_unique<TrailRenderPass>(trailEffectSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Sprite,
			RenderPhase::Overlay,
			kOverlaySpriteOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			},
			std::make_unique<SpriteRenderPass>(spriteSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Text,
			RenderPhase::Overlay,
			kOverlayTextOrder,
			true,
			{{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget}},
			std::make_unique<TextRenderPass>()});
		AddPass(RenderPassEntry{
			RenderPassId::Particle,
			RenderPhase::PostOverlay,
			kPostOverlayParticleOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			},
			std::make_unique<ParticleRenderPass>(particleSetup)});
		AddPass(RenderPassEntry{
			RenderPassId::Line,
			RenderPhase::Scene,
			kSceneLineOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			},
			std::make_unique<LineRenderPass>(lineManager, LineRenderMode::World)});
		AddPass(RenderPassEntry{
			RenderPassId::HudLine,
			RenderPhase::PostOverlay,
			kPostOverlayHudLineOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
			},
			std::make_unique<LineRenderPass>(lineManager, LineRenderMode::Hud)});
		AddPass(RenderPassEntry{
			RenderPassId::PostEffect,
			RenderPhase::PostProcess,
			kPostProcessPostEffectOrder,
			true,
			{
				{RenderResourceId::SceneColor, RenderResourceAccess::Read, RenderResourceState::PixelShaderResource},
				{RenderResourceId::PresentColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
			},
			std::make_unique<PostEffectRenderPass>(dxCore, postEffectManager, textureManager)});
		SortPasses();
		renderGraph_.Build(renderPasses_);
		renderGraph_.Validate(renderPasses_);
		renderTransitionExecutor_ = std::make_unique<RenderTransitionExecutor>(renderGraph_, renderPasses_, renderBarrierRecorder_, dxCore);
		dxCore.SetRenderTransitionExecutor(renderTransitionExecutor_.get());
	}

	void Renderer::BeginFrameBarrierRecording() {
		renderGraph_.ClearRecordedBarriers();
	}

	void Renderer::ValidateFrameBarriers() const {
		const RenderGraphValidationResult result = renderGraph_.ValidateRecordedResourceStatesForTesting(renderPasses_);
		if(!result.isValid) {
			// NOTE: assert直前に詳細を残し、Debug Layer停止時でもRenderGraph側の原因を追えるようにする。
			Logger::Log("RenderGraph frame barrier validation failed.", Logger::LogLevel::Error);
			for(RenderGraphValidationError error : result.errors) {
				Logger::Log(std::format("RenderGraph validation error: {}", ToString(error)), Logger::LogLevel::Error);
			}
			ReportSmokeTestDiagnostics();
			assert(result.isValid && "RenderGraph frame barrier validation failed.");
		}
	}

	void Renderer::ReportSmokeTestDiagnostics() const {
#ifdef _DEBUG
		const RenderGraphValidationResult result = renderGraph_.ValidateRecordedResourceStatesForTesting(renderPasses_);
		Logger::Log(result.isValid ? "RenderGraph frame validation: Success." : "RenderGraph frame validation: Failed.",
					result.isValid ? Logger::LogLevel::Success : Logger::LogLevel::Error);
		for(RenderGraphValidationError error : result.errors) {
			Logger::Log(std::format("RenderGraph validation error: {}", ToString(error)), Logger::LogLevel::Error);
		}

		Logger::Log(std::format("RenderPass execution order: {}", renderPasses_.size()), Logger::LogLevel::Info);
		for(const RenderPassEntry &entry : renderPasses_) {
			// NOTE: Smoke Test時に固定順序とPhaseの崩れをログだけで確認できるようにする。
			Logger::Log(std::format("RenderPass {} Phase={} Order={} Enabled={}",
									ToString(entry.id),
									ToString(entry.phase),
									entry.order,
									entry.enabled),
						Logger::LogLevel::Info);
		}

		const std::vector<RenderResourceBarrierRecord> &barriers = renderGraph_.GetManualBarriers();
		Logger::Log(std::format("RenderGraph recorded manual barriers: {}", barriers.size()), Logger::LogLevel::Info);
		for(const RenderResourceBarrierRecord &barrier : barriers) {
			Logger::Log(std::format("Barrier Seq={} Resource={} {} -> {} Point={}",
									barrier.sequence,
									ToString(barrier.resource),
									ToString(barrier.beforeState),
									ToString(barrier.afterState),
									ToString(barrier.point)),
						Logger::LogLevel::Info);
		}

		const std::vector<RenderResourceTransitionPlan> plans = renderGraph_.BuildTransitionPlan(renderPasses_);
		const RenderTransitionPlanComparisonResult comparison = renderGraph_.CompareTransitionPlanWithManualBarriers(renderPasses_);
		Logger::Log(std::format("RenderGraph transition plan: {} manual barriers: {} match={}",
								plans.size(),
								barriers.size(),
								comparison.isMatch),
					comparison.isMatch ? Logger::LogLevel::Success : Logger::LogLevel::Warning);
		for(const RenderResourceTransitionPlan &plan : plans) {
			const bool autoTransitionEnabled = RenderTransitionExecutor::IsAutoTransitionSupported(plan);
			const bool executed = HasExecutedPlan(barriers, plan);
			Logger::Log(std::format("Plan Seq={} Resource={} {} -> {} Boundary={} AutoTransitionEnabled={} ExecutionResult={}",
									plan.sequenceIndex,
									ToString(plan.resource),
									ToString(plan.beforeState),
									ToString(plan.afterState),
									plan.boundary.has_value() ? ToString(*plan.boundary) : "None",
									autoTransitionEnabled,
									executed ? "Executed" : "NotExecuted"),
						Logger::LogLevel::Info);
		}
		for(const RenderResourceTransitionPlan &missing : comparison.missingManualBarriers) {
			Logger::Log(std::format("Missing manual barrier: Seq={} Resource={} {} -> {}",
									missing.sequenceIndex,
									ToString(missing.resource),
									ToString(missing.beforeState),
									ToString(missing.afterState)),
						Logger::LogLevel::Warning);
		}
		for(const RenderResourceBarrierRecord &unexpected : comparison.unexpectedManualBarriers) {
			Logger::Log(std::format("Unexpected manual barrier: Seq={} Resource={} {} -> {} Point={}",
									unexpected.sequence,
									ToString(unexpected.resource),
									ToString(unexpected.beforeState),
									ToString(unexpected.afterState),
									ToString(unexpected.point)),
						Logger::LogLevel::Warning);
		}
		for(const RenderTransitionMismatch &mismatch : comparison.mismatches) {
			Logger::Log(std::format("Transition mismatch: {} ExpectedSeq={} ActualSeq={}",
									ToString(mismatch.type),
									mismatch.expected.sequenceIndex,
									mismatch.actual.sequence),
						Logger::LogLevel::Warning);
		}
#endif
	}

	void Renderer::ExecutePhase(RenderPhase phase, RenderContext &renderContext, const RenderWorld &renderWorld) {
		for(const RenderPassEntry &entry : renderPasses_) {
			if(!entry.enabled || entry.phase != phase) {
				continue;
			}
			assert(entry.pass);
			entry.pass->Execute(renderContext, renderWorld);
		}
	}

	void Renderer::AddPass(RenderPassEntry entry) {
		assert(entry.pass && "RenderPassEntry::pass must not be null.");

		const bool duplicatedId = std::any_of(renderPasses_.begin(), renderPasses_.end(), [&](const RenderPassEntry &registered) {
			return registered.id == entry.id;
		});
		assert(!duplicatedId && "RenderPassId must be unique.");

		const bool duplicatedPhaseOrder = std::any_of(renderPasses_.begin(), renderPasses_.end(), [&](const RenderPassEntry &registered) {
			return registered.phase == entry.phase && registered.order == entry.order;
		});
		assert(!duplicatedPhaseOrder && "RenderPass phase/order pair must be unique.");

		for(size_t i = 0; i < entry.resourceUsages.size(); ++i) {
			for(size_t j = i + 1; j < entry.resourceUsages.size(); ++j) {
				assert(entry.resourceUsages[i].resource != entry.resourceUsages[j].resource &&
					   "RenderPassEntry must not declare duplicated resource usages.");
			}
		}

		renderPasses_.push_back(std::move(entry));
	}

	void Renderer::SortPasses() {
		std::sort(renderPasses_.begin(), renderPasses_.end(), [](const RenderPassEntry &a, const RenderPassEntry &b) {
			return std::tuple{a.phase, a.order, a.id} < std::tuple{b.phase, b.order, b.id};
		});
	}
}

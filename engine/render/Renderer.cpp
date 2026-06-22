#include "Renderer.h"

#include "CloudRenderPass.h"
#include "DirectXCore.h"
#include "IRenderPass.h"
#include "OpaqueRenderPass.h"
#include "ParticleRenderPass.h"
#include "PostEffectRenderPass.h"
#include "RenderContext.h"
#include "RenderWorld.h"
#include "SkyboxRenderPass.h"
#include "SpriteRenderPass.h"
#include "TrailRenderPass.h"

#include <algorithm>
#include <cassert>
#include <tuple>

namespace MagEngine {
	namespace {
		constexpr int32_t kSceneSkyboxOrder = 100;
		constexpr int32_t kSceneOpaqueOrder = 200;
		constexpr int32_t kSceneCloudOrder = 300;
		constexpr int32_t kSceneTrailOrder = 400;
		constexpr int32_t kOverlaySpriteOrder = 100;
		constexpr int32_t kPostOverlayParticleOrder = 100;
		constexpr int32_t kPostProcessPostEffectOrder = 100;
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

	void Renderer::Initialize(SkyboxSetup &skyboxSetup, Object3dSetup &object3dSetup, CloudSetup &cloudSetup, TrailEffectSetup &trailEffectSetup, SpriteSetup &spriteSetup, ParticleSetup &particleSetup, DirectXCore &dxCore, PostEffectManager &postEffectManager, TextureManager &textureManager) {
		renderPasses_.clear();
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
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			},
			std::make_unique<SpriteRenderPass>(spriteSetup)});
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
	}

	void Renderer::BeginFrameBarrierRecording() {
		renderGraph_.ClearRecordedBarriers();
	}

	void Renderer::ValidateFrameBarriers() const {
		renderGraph_.ValidateRecordedResourceStates(renderPasses_);
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

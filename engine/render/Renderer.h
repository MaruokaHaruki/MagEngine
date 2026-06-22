/*********************************************************************
 * \file   Renderer.h
 * \brief  RenderPassの所有と実行を担当するクラス
 *********************************************************************/
#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "IRenderPass.h"
#include "RenderBarrierRecorder.h"
#include "RenderGraph.h"

namespace MagEngine {
	class CloudSetup;
	class Object3dSetup;
	class ParticleSetup;
	class DirectXCore;
	class PostEffectManager;
	class SkyboxSetup;
	class SpriteSetup;
	class TextureManager;
	class TrailEffectSetup;
	struct RenderContext;
	class RenderWorld;

	enum class RenderPassId : uint8_t {
		Skybox,
		Opaque,
		Cloud,
		Trail,
		Sprite,
		Particle,
		PostEffect,
	};

	enum class RenderPhase : uint8_t {
		Scene,
		Overlay,
		PostOverlay,
		PostProcess,
	};

	struct RenderPassEntry {
		RenderPassId id;
		RenderPhase phase;
		int32_t order = 0;
		bool enabled = true;
		std::vector<RenderPassResourceUsage> resourceUsages;
		std::unique_ptr<IRenderPass> pass;
	};

	std::string_view ToString(RenderPassId passId);
	std::string_view ToString(RenderPhase phase);

	class Renderer {
	public:
		void Initialize(SkyboxSetup &skyboxSetup, Object3dSetup &object3dSetup, CloudSetup &cloudSetup, TrailEffectSetup &trailEffectSetup, SpriteSetup &spriteSetup, ParticleSetup &particleSetup, DirectXCore &dxCore, PostEffectManager &postEffectManager, TextureManager &textureManager);
		void BeginFrameBarrierRecording();
		void ValidateFrameBarriers() const;
		void ExecutePhase(RenderPhase phase, RenderContext &renderContext, const RenderWorld &renderWorld);
		void AddPass(RenderPassEntry entry);
		const RenderGraph &GetRenderGraph() const {
			return renderGraph_;
		}
		const std::vector<RenderPassDependency> &GetPassDependencies() const {
			return renderGraph_.GetDependencies();
		}

	private:
		void SortPasses();

		// NOTE: Pass構成を一元管理し、実行位置と順序を登録情報だけで決定する。
		std::vector<RenderPassEntry> renderPasses_;
		RenderGraph renderGraph_;
		RenderBarrierRecorder renderBarrierRecorder_{renderGraph_};
	};
}

#include "engine/render/RenderBarrierRecorder.h"
#include "engine/render/PipelineRecipe.h"
#include "engine/render/RenderTransitionExecutor.h"
#include "engine/render/Renderer.h"
#include "engine/postEffect/fullscreenPass/FullscreenPassRendere.h"
#include "engine/postEffect/PostEffectParameterSet.h"
#include "engine/postEffect/PostEffectManager.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

using namespace MagEngine;

namespace {
	struct TestResult {
		int total = 0;
		int failed = 0;
	};

	class FakeResourceBarrierSink final : public IResourceBarrierSink {
	public:
		void ApplyTransition(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) override {
			++callCount;
			lastBefore = beforeState;
			lastAfter = afterState;
		}

		int callCount = 0;
		D3D12_RESOURCE_STATES lastBefore = D3D12_RESOURCE_STATE_COMMON;
		D3D12_RESOURCE_STATES lastAfter = D3D12_RESOURCE_STATE_COMMON;
	};

	class FakeRenderResourceResolver final : public IRenderResourceResolver {
	public:
		ID3D12Resource *ResolveRenderResource(RenderResourceId resourceId) override {
			if(resourceId == RenderResourceId::SceneColor && resolveSceneColor) {
				return reinterpret_cast<ID3D12Resource *>(0x1);
			}
			if(resourceId == RenderResourceId::PresentColor && resolvePresentColor) {
				return reinterpret_cast<ID3D12Resource *>(0x2);
			}
			if(resourceId == RenderResourceId::SceneDepth && resolveSceneDepth) {
				return reinterpret_cast<ID3D12Resource *>(0x3);
			}
			return nullptr;
		}

		bool resolveSceneColor = true;
		bool resolvePresentColor = true;
		bool resolveSceneDepth = false;
	};

	void Expect(TestResult &result, bool condition, std::string_view name) {
		++result.total;
		if(condition) {
			return;
		}
		++result.failed;
		std::cerr << "FAILED: " << name << '\n';
	}

	RenderPassEntry MakePass(RenderPassId id, RenderPhase phase, int32_t order, std::vector<RenderPassResourceUsage> usages) {
		return RenderPassEntry{id, phase, order, true, std::move(usages), nullptr};
	}

	void AddDefaultStates(RenderGraph &graph) {
		graph.AddExternalResource(RenderResourceId::SceneColor);
		graph.AddExternalResource(RenderResourceId::SceneDepth);
		graph.AddExternalResource(RenderResourceId::PresentColor);
		graph.SetInitialResourceState(RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource);
		graph.SetInitialResourceState(RenderResourceId::SceneDepth, RenderResourceState::DepthWrite);
		graph.SetInitialResourceState(RenderResourceId::PresentColor, RenderResourceState::Present);
		graph.SetFinalResourceState(RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource);
		graph.SetFinalResourceState(RenderResourceId::SceneDepth, RenderResourceState::DepthWrite);
		graph.SetFinalResourceState(RenderResourceId::PresentColor, RenderResourceState::Present);
	}

	std::vector<RenderPassEntry> MakeDefaultPasses() {
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Line, RenderPhase::Scene, 50, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Skybox, RenderPhase::Scene, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Opaque, RenderPhase::Scene, 200, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Cloud, RenderPhase::Scene, 300, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Trail, RenderPhase::Scene, 400, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Sprite, RenderPhase::Overlay, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Particle, RenderPhase::PostOverlay, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::PostEffect, RenderPhase::PostProcess, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::Read, RenderResourceState::PixelShaderResource},
				{RenderResourceId::PresentColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
			}));
		return passes;
	}

	void RecordDefaultBarriers(RenderGraph &graph) {
		RenderBarrierRecorder recorder(graph);
		FakeResourceBarrierSink sink;
		recorder.TransitionForTesting(sink, RenderResourceId::SceneColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderBarrierPoint::RenderTexturePreDraw);
		recorder.TransitionForTesting(sink, RenderResourceId::SceneColor, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderBarrierPoint::RenderTexturePostDraw);
		recorder.TransitionForTesting(sink, RenderResourceId::PresentColor, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderBarrierPoint::BeginPresentRenderTarget);
		recorder.TransitionForTesting(sink, RenderResourceId::PresentColor, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, RenderBarrierPoint::BeforePresent);
	}

	bool HasPlan(
		const std::vector<RenderResourceTransitionPlan> &plans,
		RenderResourceId resource,
		RenderResourceState beforeState,
		RenderResourceState afterState,
		RenderTransitionBoundary boundary) {
		return std::any_of(plans.begin(), plans.end(), [&](const RenderResourceTransitionPlan &plan) {
			return plan.resource == resource &&
				   plan.beforeState == beforeState &&
				   plan.afterState == afterState &&
				   plan.boundary == boundary;
		});
	}

	ID3D12RootSignature *FakeRootSignature() {
		return reinterpret_cast<ID3D12RootSignature *>(0x1);
	}

	PipelineRecipe MakeSpriteRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Sprite.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Sprite.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe MakeLineRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Line.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Line.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		return recipe;
	}

	PipelineRecipe MakeParticleRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Particle.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Particle.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		recipe.blendState.RenderTarget[0].BlendEnable = TRUE;
		recipe.blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		return recipe;
	}

	PipelineRecipe MakeObject3dRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Object3d.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Object3d.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].BlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].LogicOpEnable = FALSE;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe MakeSkyboxRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Skybox.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Skybox.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].BlendEnable = FALSE;
		recipe.blendState.RenderTarget[0].LogicOpEnable = FALSE;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe MakeCloudRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Cloud.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Cloud.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].BlendEnable = TRUE;
		recipe.blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		recipe.blendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe MakeTrailRecipeForTesting() {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Trail.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Trail.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].BlendEnable = TRUE;
		recipe.blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		recipe.blendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		recipe.blendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	PipelineRecipe MakePostEffectRecipeForTesting(const std::wstring &pixelShaderPath) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/FullScreen.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {pixelShaderPath, L"main", L"ps_6_0"};
		recipe.rootSignature = FakeRootSignature();
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = false;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		return recipe;
	}

	void RenderGraphTest_NormalFrame(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RecordDefaultBarriers(graph);
		const auto passes = MakeDefaultPasses();
		const RenderGraphValidationResult validation = graph.ValidateRecordedResourceStatesForTesting(passes);
		Expect(result, validation.isValid, "RenderGraphTest_NormalFrame");
		Expect(result, !validation.HasError(RenderGraphValidationError::FinalStateMismatch), "RenderGraphTest_NormalFrame_FinalState");
		Expect(result, !validation.HasError(RenderGraphValidationError::DependencyOrderMismatch), "RenderGraphTest_NormalFrame_Order");
	}

	void RenderGraphTest_MissingBarrier(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Skybox, RenderPhase::Scene, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
			}));
		const RenderGraphValidationResult validation = graph.ValidateRecordedResourceStatesForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_MissingBarrier_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::MissingBarrier), "RenderGraphTest_MissingBarrier_Error");
	}

	void RenderGraphTest_InvalidBarrierBefore(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RenderBarrierRecorder recorder(graph);
		FakeResourceBarrierSink sink;
		recorder.TransitionForTesting(sink, RenderResourceId::SceneColor, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderBarrierPoint::RenderTexturePostDraw);
		const RenderGraphValidationResult validation = graph.ValidateRecordedResourceStatesForTesting({});
		Expect(result, !validation.isValid, "RenderGraphTest_InvalidBarrierBefore_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::InvalidBarrierBeforeState), "RenderGraphTest_InvalidBarrierBefore_Error");
	}

	void RenderGraphTest_FinalStateMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RenderBarrierRecorder recorder(graph);
		FakeResourceBarrierSink sink;
		recorder.TransitionForTesting(sink, RenderResourceId::PresentColor, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderBarrierPoint::BeginPresentRenderTarget);
		const RenderGraphValidationResult validation = graph.ValidateRecordedResourceStatesForTesting({});
		Expect(result, !validation.isValid, "RenderGraphTest_FinalStateMismatch_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::FinalStateMismatch), "RenderGraphTest_FinalStateMismatch_Error");
	}

	void RenderGraphTest_MissingRequiredState(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Opaque, RenderPhase::Scene, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::Unknown},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_MissingRequiredState_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::MissingRequiredState), "RenderGraphTest_MissingRequiredState_Error");
	}

	void RenderGraphTest_MissingWriter(TestResult &result) {
		RenderGraph graph;
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::PostEffect, RenderPhase::PostProcess, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::Read, RenderResourceState::PixelShaderResource},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_MissingWriter_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::MissingWriter), "RenderGraphTest_MissingWriter_Error");
	}

	void RenderGraphTest_DuplicateUsage(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Opaque, RenderPhase::Scene, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::Read, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneColor, RenderResourceAccess::Write, RenderResourceState::RenderTarget},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_DuplicateUsage_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::DuplicateUsage), "RenderGraphTest_DuplicateUsage_Error");
	}

	void RenderGraphTest_DependencyOrderMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Skybox, RenderPhase::Scene, 100, {}));
		passes.push_back(MakePass(RenderPassId::Opaque, RenderPhase::Scene, 200, {}));
		graph.AddDependencyForTesting(RenderPassId::Opaque, RenderPassId::Skybox, RenderResourceId::SceneColor);
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_DependencyOrderMismatch_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::DependencyOrderMismatch), "RenderGraphTest_DependencyOrderMismatch_Error");
	}

	void RenderGraphTest_CircularDependency(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Skybox, RenderPhase::Scene, 100, {}));
		passes.push_back(MakePass(RenderPassId::Opaque, RenderPhase::Scene, 200, {}));
		graph.AddDependencyForTesting(RenderPassId::Skybox, RenderPassId::Opaque, RenderResourceId::SceneColor);
		graph.AddDependencyForTesting(RenderPassId::Opaque, RenderPassId::Skybox, RenderResourceId::SceneColor);
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, !validation.isValid, "RenderGraphTest_CircularDependency_Invalid");
		Expect(result, validation.HasError(RenderGraphValidationError::CircularDependency), "RenderGraphTest_CircularDependency_Error");
	}

	void RenderGraphTest_BuildTransitionPlan(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const auto passes = MakeDefaultPasses();
		const std::vector<RenderResourceTransitionPlan> plans = graph.BuildTransitionPlan(passes);
		Expect(result, plans.size() == 4, "RenderGraphTest_BuildTransitionPlan_Count");
		Expect(result, HasPlan(plans, RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource, RenderResourceState::RenderTarget, RenderTransitionBoundary::RenderTexturePreDraw), "RenderGraphTest_BuildTransitionPlan_SceneColorBegin");
		Expect(result, HasPlan(plans, RenderResourceId::SceneColor, RenderResourceState::RenderTarget, RenderResourceState::PixelShaderResource, RenderTransitionBoundary::RenderTexturePostDraw), "RenderGraphTest_BuildTransitionPlan_SceneColorPost");
		Expect(result, HasPlan(plans, RenderResourceId::PresentColor, RenderResourceState::Present, RenderResourceState::RenderTarget, RenderTransitionBoundary::BeginPresentRenderTarget), "RenderGraphTest_BuildTransitionPlan_PresentBegin");
		Expect(result, HasPlan(plans, RenderResourceId::PresentColor, RenderResourceState::RenderTarget, RenderResourceState::Present, RenderTransitionBoundary::BeforePresent), "RenderGraphTest_BuildTransitionPlan_PresentFinal");
		Expect(result, !HasPlan(plans, RenderResourceId::SceneDepth, RenderResourceState::DepthWrite, RenderResourceState::DepthWrite, RenderTransitionBoundary::RenderTexturePreDraw), "RenderGraphTest_BuildTransitionPlan_NoSameState");
	}

	void RenderGraphTest_CompareTransitionPlanMatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RecordDefaultBarriers(graph);
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, comparison.isMatch, "RenderGraphTest_CompareTransitionPlanMatch_Result");
		Expect(result, comparison.missingManualBarriers.empty(), "RenderGraphTest_CompareTransitionPlanMatch_Missing");
		Expect(result, comparison.unexpectedManualBarriers.empty(), "RenderGraphTest_CompareTransitionPlanMatch_Unexpected");
		Expect(result, comparison.mismatches.empty(), "RenderGraphTest_CompareTransitionPlanMatch_Mismatch");
	}

	void RenderGraphTest_CompareTransitionPlanMissing(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanMissing_Result");
		Expect(result, comparison.missingManualBarriers.size() == 4, "RenderGraphTest_CompareTransitionPlanMissing_Count");
	}

	void RenderGraphTest_CompareTransitionPlanBeforeMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RecordDefaultBarriers(graph);
		graph.ClearRecordedBarriers();
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneColor, RenderResourceState::RenderTarget, RenderResourceState::RenderTarget, RenderBarrierPoint::RenderTexturePreDraw, 10u});
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneColor, RenderResourceState::RenderTarget, RenderResourceState::PixelShaderResource, RenderBarrierPoint::RenderTexturePostDraw, 165u});
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::PresentColor, RenderResourceState::Present, RenderResourceState::RenderTarget, RenderBarrierPoint::BeginPresentRenderTarget, 166u});
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::PresentColor, RenderResourceState::RenderTarget, RenderResourceState::Present, RenderBarrierPoint::BeforePresent, 1000u});
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanBeforeMismatch_Result");
		Expect(result, !comparison.mismatches.empty() && comparison.mismatches.front().type == RenderTransitionMismatchType::BeforeState, "RenderGraphTest_CompareTransitionPlanBeforeMismatch_Type");
	}

	void RenderGraphTest_CompareTransitionPlanAfterMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource, RenderResourceState::DepthRead, RenderBarrierPoint::RenderTexturePreDraw, 10u});
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanAfterMismatch_Result");
		Expect(result, !comparison.mismatches.empty() && comparison.mismatches.front().type == RenderTransitionMismatchType::AfterState, "RenderGraphTest_CompareTransitionPlanAfterMismatch_Type");
	}

	void RenderGraphTest_CompareTransitionPlanSequenceMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource, RenderResourceState::RenderTarget, RenderBarrierPoint::RenderTexturePreDraw, 11u});
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanSequenceMismatch_Result");
		Expect(result, !comparison.mismatches.empty() && comparison.mismatches.front().type == RenderTransitionMismatchType::Sequence, "RenderGraphTest_CompareTransitionPlanSequenceMismatch_Type");
	}

	void RenderGraphTest_CompareTransitionPlanBoundaryMismatch(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneColor, RenderResourceState::PixelShaderResource, RenderResourceState::RenderTarget, RenderBarrierPoint::RenderTexturePostDraw, 165u});
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanBoundaryMismatch_Result");
		Expect(result, !comparison.mismatches.empty() && comparison.mismatches.front().type == RenderTransitionMismatchType::Boundary, "RenderGraphTest_CompareTransitionPlanBoundaryMismatch_Type");
	}

	void RenderGraphTest_CompareTransitionPlanUnexpected(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RecordDefaultBarriers(graph);
		graph.RecordManualBarrierForTesting(RenderResourceBarrierRecord{RenderResourceId::SceneDepth, RenderResourceState::DepthWrite, RenderResourceState::DepthRead, RenderBarrierPoint::BeforePresent, 1000u});
		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(MakeDefaultPasses());
		Expect(result, !comparison.isMatch, "RenderGraphTest_CompareTransitionPlanUnexpected_Result");
		Expect(result, comparison.unexpectedManualBarriers.size() == 1, "RenderGraphTest_CompareTransitionPlanUnexpected_Count");
	}

	void RenderGraphTest_LinePassResourceUsage(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Line, RenderPhase::Scene, 50, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		passes.push_back(MakePass(RenderPassId::Skybox, RenderPhase::Scene, 100, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
				{RenderResourceId::SceneDepth, RenderResourceAccess::Read, RenderResourceState::DepthWrite},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, validation.isValid, "RenderGraphTest_LinePassResourceUsage_Valid");
		Expect(result, passes[0].phase == RenderPhase::Scene && passes[0].order < passes[1].order, "RenderGraphTest_LinePassResourceUsage_Order");
	}

	void RenderGraphTest_LinePassMissingSceneColor(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Line, RenderPhase::Scene, 50, {
				{RenderResourceId::SceneDepth, RenderResourceAccess::ReadWrite, RenderResourceState::DepthWrite},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, validation.isValid, "RenderGraphTest_LinePassMissingSceneColor_GraphAllowsDepthOnly");
		const bool declaresSceneColor = std::any_of(passes[0].resourceUsages.begin(), passes[0].resourceUsages.end(), [](const RenderPassResourceUsage &usage) {
			return usage.resource == RenderResourceId::SceneColor &&
				   usage.access == RenderResourceAccess::ReadWrite &&
				   usage.requiredState == RenderResourceState::RenderTarget;
		});
		Expect(result, !declaresSceneColor, "RenderGraphTest_LinePassMissingSceneColor_DetectedByUsageCheck");
	}

	void RenderGraphTest_LinePassMissingSceneDepth(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		std::vector<RenderPassEntry> passes;
		passes.push_back(MakePass(RenderPassId::Line, RenderPhase::Scene, 50, {
				{RenderResourceId::SceneColor, RenderResourceAccess::ReadWrite, RenderResourceState::RenderTarget},
			}));
		const RenderGraphValidationResult validation = graph.ValidateForTesting(passes);
		Expect(result, validation.isValid, "RenderGraphTest_LinePassMissingSceneDepth_GraphAllowsColorOnly");
		const bool declaresSceneDepth = std::any_of(passes[0].resourceUsages.begin(), passes[0].resourceUsages.end(), [](const RenderPassResourceUsage &usage) {
			return usage.resource == RenderResourceId::SceneDepth && usage.requiredState == RenderResourceState::DepthWrite;
		});
		Expect(result, !declaresSceneDepth, "RenderGraphTest_LinePassMissingSceneDepth_DetectedByUsageCheck");
	}

	void RenderTransitionExecutorTest_BoundaryExecution(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const auto passes = MakeDefaultPasses();
		RenderBarrierRecorder recorder(graph);
		FakeRenderResourceResolver resolver;
		FakeResourceBarrierSink sink;
		RenderTransitionExecutor executor(graph, passes, recorder, resolver);

		const RenderTransitionExecutionResult preDraw = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePreDraw);
		Expect(result, preDraw.isValid && preDraw.plan.resource == RenderResourceId::SceneColor, "RenderTransitionExecutorTest_PreDraw");
		const RenderTransitionExecutionResult postDraw = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePostDraw);
		Expect(result, postDraw.isValid && postDraw.plan.resource == RenderResourceId::SceneColor, "RenderTransitionExecutorTest_PostDraw");
		const RenderTransitionExecutionResult beginPresent = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::BeginPresentRenderTarget);
		Expect(result, beginPresent.isValid && beginPresent.plan.resource == RenderResourceId::PresentColor, "RenderTransitionExecutorTest_BeginPresent");
		const RenderTransitionExecutionResult beforePresent = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::BeforePresent);
		Expect(result, beforePresent.isValid && beforePresent.plan.resource == RenderResourceId::PresentColor, "RenderTransitionExecutorTest_BeforePresent");

		const RenderTransitionPlanComparisonResult comparison = graph.CompareTransitionPlanWithManualBarriers(passes);
		Expect(result, comparison.isMatch, "RenderTransitionExecutorTest_PlanComparison");
		Expect(result, graph.GetManualBarriers().size() == 4, "RenderTransitionExecutorTest_RecordCount");
	}

	void RenderTransitionExecutorTest_ResolveFailed(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const auto passes = MakeDefaultPasses();
		RenderBarrierRecorder recorder(graph);
		FakeRenderResourceResolver resolver;
		resolver.resolveSceneColor = false;
		FakeResourceBarrierSink sink;
		RenderTransitionExecutor executor(graph, passes, recorder, resolver);

		const RenderTransitionExecutionResult execution = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePreDraw);
		Expect(result, !execution.isValid && execution.error == RenderTransitionExecutionError::ResourceResolveFailed, "RenderTransitionExecutorTest_ResolveFailed");
		Expect(result, graph.GetManualBarriers().empty(), "RenderTransitionExecutorTest_ResolveFailed_NoRecord");
	}

	void RenderTransitionExecutorTest_MissingPlan(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const std::vector<RenderPassEntry> passes;
		RenderBarrierRecorder recorder(graph);
		FakeRenderResourceResolver resolver;
		FakeResourceBarrierSink sink;
		RenderTransitionExecutor executor(graph, passes, recorder, resolver);

		const RenderTransitionExecutionResult execution = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePreDraw);
		Expect(result, !execution.isValid && execution.error == RenderTransitionExecutionError::MissingPlan, "RenderTransitionExecutorTest_MissingPlan");
	}

	void RenderTransitionExecutorTest_DuplicateBoundary(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		const auto passes = MakeDefaultPasses();
		RenderBarrierRecorder recorder(graph);
		FakeRenderResourceResolver resolver;
		FakeResourceBarrierSink sink;
		RenderTransitionExecutor executor(graph, passes, recorder, resolver);

		const RenderTransitionExecutionResult first = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePreDraw);
		const RenderTransitionExecutionResult second = executor.ExecuteBoundaryForTesting(sink, RenderTransitionBoundary::RenderTexturePreDraw);
		Expect(result, first.isValid, "RenderTransitionExecutorTest_DuplicateBoundary_First");
		Expect(result, !second.isValid && second.error == RenderTransitionExecutionError::DuplicateBoundaryExecution, "RenderTransitionExecutorTest_DuplicateBoundary_Second");
	}

	void RenderTransitionExecutorTest_UnsupportedPlanWhitelist(TestResult &result) {
		RenderResourceTransitionPlan depthPlan{
			RenderResourceId::SceneDepth,
			RenderResourceState::DepthWrite,
			RenderResourceState::DepthRead,
			std::nullopt,
			std::nullopt,
			RenderTransitionBoundary::BeforePresent,
			1000u};
		RenderResourceTransitionPlan unsupportedStatePlan{
			RenderResourceId::SceneColor,
			RenderResourceState::CopySource,
			RenderResourceState::CopyDest,
			std::nullopt,
			std::nullopt,
			RenderTransitionBoundary::RenderTexturePreDraw,
			10u};
		Expect(result, !RenderTransitionExecutor::IsAutoTransitionSupported(depthPlan), "RenderTransitionExecutorTest_UnsupportedPlan_Depth");
		Expect(result, !RenderTransitionExecutor::IsAutoTransitionSupported(unsupportedStatePlan), "RenderTransitionExecutorTest_UnsupportedPlan_State");
	}

	void RenderBarrierRecorderTest_RecordTransition(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RenderBarrierRecorder recorder(graph);
		FakeResourceBarrierSink sink;
		const RenderBarrierRecorderResult recorderResult = recorder.TransitionForTesting(
			sink,
			RenderResourceId::SceneColor,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			RenderBarrierPoint::RenderTexturePreDraw);
		Expect(result, recorderResult.isValid, "RenderBarrierRecorderTest_RecordTransition_Result");
		Expect(result, sink.callCount == 1, "RenderBarrierRecorderTest_RecordTransition_CallCount");
		Expect(result, graph.GetManualBarriers().size() == 1, "RenderBarrierRecorderTest_RecordTransition_RecordCount");
		const RenderResourceBarrierRecord &record = graph.GetManualBarriers().front();
		Expect(result, record.resource == RenderResourceId::SceneColor, "RenderBarrierRecorderTest_RecordTransition_Resource");
		Expect(result, record.beforeState == RenderResourceState::PixelShaderResource, "RenderBarrierRecorderTest_RecordTransition_Before");
		Expect(result, record.afterState == RenderResourceState::RenderTarget, "RenderBarrierRecorderTest_RecordTransition_After");
		Expect(result, record.point == RenderBarrierPoint::RenderTexturePreDraw, "RenderBarrierRecorderTest_RecordTransition_Point");
	}

	void RenderBarrierRecorderTest_InvalidInputs(TestResult &result) {
		RenderGraph graph;
		AddDefaultStates(graph);
		RenderBarrierRecorder recorder(graph);
		FakeResourceBarrierSink sink;
		const RenderBarrierRecorderResult sameState = recorder.TransitionForTesting(
			sink,
			RenderResourceId::SceneColor,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			RenderBarrierPoint::RenderTexturePreDraw);
		Expect(result, !sameState.isValid && sameState.error == RenderBarrierRecorderError::SameState, "RenderBarrierRecorderTest_SameState");

		recorder.TransitionForTesting(sink, RenderResourceId::SceneColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderBarrierPoint::RenderTexturePreDraw);
		const RenderBarrierRecorderResult conflict = recorder.TransitionForTesting(
			sink,
			RenderResourceId::SceneColor,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			RenderBarrierPoint::RenderTexturePreDraw);
		Expect(result, !conflict.isValid && conflict.error == RenderBarrierRecorderError::ConflictingBarrierPoint, "RenderBarrierRecorderTest_Conflict");
	}

	void PipelineRecipeTest_SpriteDefaults(TestResult &result) {
		const PipelineRecipe recipe = MakeSpriteRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_SpriteDefaults_Valid");
		Expect(result, recipe.vertexShader.path == L"resources/shader/Sprite.VS.hlsl", "PipelineRecipeTest_SpriteDefaults_VS");
		Expect(result, recipe.pixelShader.path == L"resources/shader/Sprite.PS.hlsl", "PipelineRecipeTest_SpriteDefaults_PS");
		Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_SpriteDefaults_RTV");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_SpriteDefaults_Topology");
	}

	void PipelineRecipeTest_LineDepthWrite(TestResult &result) {
		const PipelineRecipe recipe = MakeLineRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_LineDepthWrite_Valid");
		Expect(result, recipe.depthStencilState.DepthEnable == TRUE, "PipelineRecipeTest_LineDepthWrite_Enable");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL, "PipelineRecipeTest_LineDepthWrite_WriteAll");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, "PipelineRecipeTest_LineDepthWrite_Topology");
	}

	void PipelineRecipeTest_ParticleBlendAndDepth(TestResult &result) {
		const PipelineRecipe recipe = MakeParticleRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_ParticleBlendAndDepth_Valid");
		Expect(result, recipe.blendState.RenderTarget[0].BlendEnable == TRUE, "PipelineRecipeTest_ParticleBlendAndDepth_Blend");
		Expect(result, recipe.blendState.RenderTarget[0].DestBlend == D3D12_BLEND_ONE, "PipelineRecipeTest_ParticleBlendAndDepth_Additive");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ZERO, "PipelineRecipeTest_ParticleBlendAndDepth_DepthWriteZero");
	}

	void PipelineRecipeTest_Object3dDefaults(TestResult &result) {
		const PipelineRecipe recipe = MakeObject3dRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_Object3dDefaults_Valid");
		Expect(result, recipe.vertexShader.path == L"resources/shader/Object3d.VS.hlsl", "PipelineRecipeTest_Object3dDefaults_VS");
		Expect(result, recipe.pixelShader.path == L"resources/shader/Object3d.PS.hlsl", "PipelineRecipeTest_Object3dDefaults_PS");
		Expect(result, recipe.rootSignature != nullptr, "PipelineRecipeTest_Object3dDefaults_RootSignature");
		Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_Object3dDefaults_RTV");
		Expect(result, !recipe.inputLayout.empty(), "PipelineRecipeTest_Object3dDefaults_InputLayout");
		Expect(result, recipe.depthStencilState.DepthEnable == TRUE, "PipelineRecipeTest_Object3dDefaults_DepthEnable");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL, "PipelineRecipeTest_Object3dDefaults_DepthWrite");
		Expect(result, recipe.depthStencilState.DepthFunc == D3D12_COMPARISON_FUNC_LESS_EQUAL, "PipelineRecipeTest_Object3dDefaults_DepthFunc");
		Expect(result, recipe.rasterizerState.CullMode == D3D12_CULL_MODE_BACK, "PipelineRecipeTest_Object3dDefaults_CullMode");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_Object3dDefaults_Topology");
	}

	void PipelineRecipeTest_SkyboxDefaults(TestResult &result) {
		const PipelineRecipe recipe = MakeSkyboxRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_SkyboxDefaults_Valid");
		Expect(result, recipe.vertexShader.path == L"resources/shader/Skybox.VS.hlsl", "PipelineRecipeTest_SkyboxDefaults_VS");
		Expect(result, recipe.pixelShader.path == L"resources/shader/Skybox.PS.hlsl", "PipelineRecipeTest_SkyboxDefaults_PS");
		Expect(result, recipe.rootSignature != nullptr, "PipelineRecipeTest_SkyboxDefaults_RootSignature");
		Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_SkyboxDefaults_RTV");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_SkyboxDefaults_Topology");
		Expect(result, recipe.rasterizerState.CullMode == D3D12_CULL_MODE_NONE, "PipelineRecipeTest_SkyboxDefaults_CullNone");
		Expect(result, recipe.rasterizerState.FillMode == D3D12_FILL_MODE_SOLID, "PipelineRecipeTest_SkyboxDefaults_FillSolid");
		Expect(result, recipe.depthStencilState.DepthEnable == TRUE, "PipelineRecipeTest_SkyboxDefaults_DepthEnable");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ZERO, "PipelineRecipeTest_SkyboxDefaults_DepthWriteZero");
		Expect(result, recipe.depthStencilState.DepthFunc == D3D12_COMPARISON_FUNC_LESS_EQUAL, "PipelineRecipeTest_SkyboxDefaults_DepthLessEqual");
	}

	void PipelineRecipeTest_CloudDefaults(TestResult &result) {
		const PipelineRecipe recipe = MakeCloudRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_CloudDefaults_Valid");
		Expect(result, recipe.vertexShader.path == L"resources/shader/Cloud.VS.hlsl", "PipelineRecipeTest_CloudDefaults_VS");
		Expect(result, recipe.pixelShader.path == L"resources/shader/Cloud.PS.hlsl", "PipelineRecipeTest_CloudDefaults_PS");
		Expect(result, recipe.rootSignature != nullptr, "PipelineRecipeTest_CloudDefaults_RootSignature");
		Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_CloudDefaults_RTV");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_CloudDefaults_Topology");
		Expect(result, recipe.inputLayout.size() == 2, "PipelineRecipeTest_CloudDefaults_InputLayout");
		Expect(result, recipe.blendState.RenderTarget[0].BlendEnable == TRUE, "PipelineRecipeTest_CloudDefaults_BlendEnable");
		Expect(result, recipe.blendState.RenderTarget[0].SrcBlend == D3D12_BLEND_SRC_ALPHA, "PipelineRecipeTest_CloudDefaults_SrcBlend");
		Expect(result, recipe.blendState.RenderTarget[0].DestBlend == D3D12_BLEND_INV_SRC_ALPHA, "PipelineRecipeTest_CloudDefaults_DestBlend");
		Expect(result, recipe.rasterizerState.CullMode == D3D12_CULL_MODE_NONE, "PipelineRecipeTest_CloudDefaults_CullNone");
		Expect(result, recipe.rasterizerState.FillMode == D3D12_FILL_MODE_SOLID, "PipelineRecipeTest_CloudDefaults_FillSolid");
		Expect(result, recipe.depthStencilState.DepthEnable == TRUE, "PipelineRecipeTest_CloudDefaults_DepthEnable");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL, "PipelineRecipeTest_CloudDefaults_DepthWriteAll");
		Expect(result, recipe.depthStencilState.DepthFunc == D3D12_COMPARISON_FUNC_LESS_EQUAL, "PipelineRecipeTest_CloudDefaults_DepthLessEqual");
	}

	void PipelineRecipeTest_TrailDefaults(TestResult &result) {
		const PipelineRecipe recipe = MakeTrailRecipeForTesting();
		Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_TrailDefaults_Valid");
		Expect(result, recipe.vertexShader.path == L"resources/shader/Trail.VS.hlsl", "PipelineRecipeTest_TrailDefaults_VS");
		Expect(result, recipe.pixelShader.path == L"resources/shader/Trail.PS.hlsl", "PipelineRecipeTest_TrailDefaults_PS");
		Expect(result, recipe.rootSignature != nullptr, "PipelineRecipeTest_TrailDefaults_RootSignature");
		Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_TrailDefaults_RTV");
		Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_TrailDefaults_Topology");
		Expect(result, recipe.inputLayout.size() == 3, "PipelineRecipeTest_TrailDefaults_InputLayout");
		Expect(result, recipe.blendState.RenderTarget[0].BlendEnable == TRUE, "PipelineRecipeTest_TrailDefaults_BlendEnable");
		Expect(result, recipe.blendState.RenderTarget[0].SrcBlend == D3D12_BLEND_SRC_ALPHA, "PipelineRecipeTest_TrailDefaults_SrcBlend");
		Expect(result, recipe.blendState.RenderTarget[0].DestBlend == D3D12_BLEND_INV_SRC_ALPHA, "PipelineRecipeTest_TrailDefaults_DestBlend");
		Expect(result, recipe.rasterizerState.CullMode == D3D12_CULL_MODE_NONE, "PipelineRecipeTest_TrailDefaults_CullNone");
		Expect(result, recipe.rasterizerState.FillMode == D3D12_FILL_MODE_SOLID, "PipelineRecipeTest_TrailDefaults_FillSolid");
		Expect(result, recipe.depthStencilState.DepthEnable == TRUE, "PipelineRecipeTest_TrailDefaults_DepthEnable");
		Expect(result, recipe.depthStencilState.DepthWriteMask == D3D12_DEPTH_WRITE_MASK_ALL, "PipelineRecipeTest_TrailDefaults_DepthWriteAll");
		Expect(result, recipe.depthStencilState.DepthFunc == D3D12_COMPARISON_FUNC_LESS_EQUAL, "PipelineRecipeTest_TrailDefaults_DepthLessEqual");
	}

	void PipelineRecipeTest_PostEffectDefaults(TestResult &result) {
		const std::vector<std::wstring> pixelShaderPaths = {
			L"resources/shader/FullScreen.PS.hlsl",
			L"resources/shader/Fullscreen.PS.hlsl",
			L"resources/shader/Grayscale.PS.hlsl",
			L"resources/shader/Vignetting.hlsl",
		};

		for(const std::wstring &pixelShaderPath : pixelShaderPaths) {
			const PipelineRecipe recipe = MakePostEffectRecipeForTesting(pixelShaderPath);
			Expect(result, recipe.Validate().isValid, "PipelineRecipeTest_PostEffectDefaults_Valid");
			Expect(result, recipe.vertexShader.path == L"resources/shader/FullScreen.VS.hlsl", "PipelineRecipeTest_PostEffectDefaults_VS");
			Expect(result, recipe.pixelShader.path == pixelShaderPath, "PipelineRecipeTest_PostEffectDefaults_PS");
			Expect(result, recipe.rootSignature != nullptr, "PipelineRecipeTest_PostEffectDefaults_RootSignature");
			Expect(result, recipe.renderTargetFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "PipelineRecipeTest_PostEffectDefaults_RTV");
			Expect(result, recipe.primitiveTopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, "PipelineRecipeTest_PostEffectDefaults_Topology");
			Expect(result, recipe.inputLayout.empty(), "PipelineRecipeTest_PostEffectDefaults_InputLayoutEmpty");
			Expect(result, recipe.blendState.RenderTarget[0].RenderTargetWriteMask == D3D12_COLOR_WRITE_ENABLE_ALL, "PipelineRecipeTest_PostEffectDefaults_WriteMask");
			Expect(result, recipe.rasterizerState.CullMode == D3D12_CULL_MODE_NONE, "PipelineRecipeTest_PostEffectDefaults_CullNone");
			Expect(result, recipe.rasterizerState.FillMode == D3D12_FILL_MODE_SOLID, "PipelineRecipeTest_PostEffectDefaults_FillSolid");
			Expect(result, recipe.depthStencilState.DepthEnable == FALSE, "PipelineRecipeTest_PostEffectDefaults_DepthDisabled");
		}
	}

	void PipelineRecipeTest_ValidationErrors(TestResult &result) {
		PipelineRecipe recipe = MakeSpriteRecipeForTesting();
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_MissingRootSignature");

		recipe = MakeSpriteRecipeForTesting();
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_MissingRTV");

		recipe = MakeSpriteRecipeForTesting();
		recipe.vertexShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingVertexShader, "PipelineRecipeTest_MissingVS");

		recipe = MakeSpriteRecipeForTesting();
		recipe.pixelShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingPixelShader, "PipelineRecipeTest_MissingPS");

		recipe = MakeObject3dRecipeForTesting();
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_Object3dMissingRootSignature");

		recipe = MakeObject3dRecipeForTesting();
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_Object3dMissingRTV");

		recipe = MakeSkyboxRecipeForTesting();
		recipe.vertexShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingVertexShader, "PipelineRecipeTest_SkyboxMissingVS");

		recipe = MakeSkyboxRecipeForTesting();
		recipe.pixelShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingPixelShader, "PipelineRecipeTest_SkyboxMissingPS");

		recipe = MakeSkyboxRecipeForTesting();
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_SkyboxMissingRootSignature");

		recipe = MakeSkyboxRecipeForTesting();
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_SkyboxMissingRTV");

		recipe = MakeCloudRecipeForTesting();
		recipe.vertexShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingVertexShader, "PipelineRecipeTest_CloudMissingVS");

		recipe = MakeCloudRecipeForTesting();
		recipe.pixelShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingPixelShader, "PipelineRecipeTest_CloudMissingPS");

		recipe = MakeCloudRecipeForTesting();
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_CloudMissingRootSignature");

		recipe = MakeCloudRecipeForTesting();
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_CloudMissingRTV");

		recipe = MakeTrailRecipeForTesting();
		recipe.vertexShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingVertexShader, "PipelineRecipeTest_TrailMissingVS");

		recipe = MakeTrailRecipeForTesting();
		recipe.pixelShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingPixelShader, "PipelineRecipeTest_TrailMissingPS");

		recipe = MakeTrailRecipeForTesting();
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_TrailMissingRootSignature");

		recipe = MakeTrailRecipeForTesting();
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_TrailMissingRTV");

		recipe = MakePostEffectRecipeForTesting(L"resources/shader/Grayscale.PS.hlsl");
		recipe.vertexShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingVertexShader, "PipelineRecipeTest_PostEffectMissingVS");

		recipe = MakePostEffectRecipeForTesting(L"resources/shader/Grayscale.PS.hlsl");
		recipe.pixelShader.path.clear();
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingPixelShader, "PipelineRecipeTest_PostEffectMissingPS");

		recipe = MakePostEffectRecipeForTesting(L"resources/shader/Grayscale.PS.hlsl");
		recipe.rootSignature = nullptr;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRootSignature, "PipelineRecipeTest_PostEffectMissingRootSignature");

		recipe = MakePostEffectRecipeForTesting(L"resources/shader/Grayscale.PS.hlsl");
		recipe.renderTargetFormat = DXGI_FORMAT_UNKNOWN;
		Expect(result, !recipe.Validate().isValid && recipe.Validate().error == PipelineRecipeValidationError::MissingRenderTargetFormat, "PipelineRecipeTest_PostEffectMissingRTV");
	}

	bool HasMismatch(
		const PostEffectManager::PostEffectTransitionComparisonResult &comparison,
		PostEffectManager::PostEffectTransitionMismatchReason reason) {
		return std::any_of(comparison.mismatches.begin(), comparison.mismatches.end(), [&](const PostEffectManager::PostEffectTransitionMismatch &mismatch) {
			return mismatch.reason == reason;
		});
	}

	void PostEffectResourceTransitionTest_OneEffectPlan(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(1, 0);

		// NOTE: 1 Effect時は中間RTVを使わない既存実装のため、推測Barrierを追加しない。
		Expect(result, plan.empty(), "PostEffectResourceTransitionTest_OneEffectPlan_Empty");
	}

	void PostEffectResourceTransitionTest_TwoEffectPingPongPlan(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);

		Expect(result, plan.size() == 2, "PostEffectResourceTransitionTest_TwoEffectPlan_Count");
		Expect(result, plan[0].slot == PostEffectManager::PostEffectResourceSlot::Pong, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginSlot");
		Expect(result, plan[0].stage == PostEffectManager::PostEffectStage::BeforeEffect, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginStage");
		Expect(result, plan[0].sequence == 0, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginSequence");
		Expect(result, plan[0].resourceIndex == 1, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginResource");
		Expect(result, plan[0].beforeState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginBefore");
		Expect(result, plan[0].afterState == D3D12_RESOURCE_STATE_RENDER_TARGET, "PostEffectResourceTransitionTest_TwoEffectPlan_BeginAfter");
		Expect(result, plan[1].slot == PostEffectManager::PostEffectResourceSlot::Pong, "PostEffectResourceTransitionTest_TwoEffectPlan_EndSlot");
		Expect(result, plan[1].stage == PostEffectManager::PostEffectStage::AfterEffect, "PostEffectResourceTransitionTest_TwoEffectPlan_EndStage");
		Expect(result, plan[1].sequence == 1, "PostEffectResourceTransitionTest_TwoEffectPlan_EndSequence");
		Expect(result, plan[1].beforeState == D3D12_RESOURCE_STATE_RENDER_TARGET, "PostEffectResourceTransitionTest_TwoEffectPlan_EndBefore");
		Expect(result, plan[1].afterState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "PostEffectResourceTransitionTest_TwoEffectPlan_EndAfter");
	}

	void PostEffectResourceTransitionTest_TwoEffectInitialPongPlan(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 1);

		Expect(result, plan.size() == 2, "PostEffectResourceTransitionTest_InitialPongPlan_Count");
		Expect(result, plan[0].slot == PostEffectManager::PostEffectResourceSlot::Ping, "PostEffectResourceTransitionTest_InitialPongPlan_BeginSlot");
		Expect(result, plan[0].resourceIndex == 0, "PostEffectResourceTransitionTest_InitialPongPlan_BeginResource");
		Expect(result, plan[1].slot == PostEffectManager::PostEffectResourceSlot::Ping, "PostEffectResourceTransitionTest_InitialPongPlan_EndSlot");
	}

	void PostEffectResourceTransitionTest_CompareMatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, plan);

		Expect(result, comparison.isMatch, "PostEffectResourceTransitionTest_CompareMatch_Result");
		Expect(result, comparison.plannedCount == 2, "PostEffectResourceTransitionTest_CompareMatch_PlannedCount");
		Expect(result, comparison.recordedCount == 2, "PostEffectResourceTransitionTest_CompareMatch_RecordedCount");
		Expect(result, comparison.mismatches.empty(), "PostEffectResourceTransitionTest_CompareMatch_Mismatches");
	}

	void PostEffectResourceTransitionTest_CompareMissing(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded.pop_back();

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareMissing_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::MissingRecordedTransition), "PostEffectResourceTransitionTest_CompareMissing_Reason");
	}

	void PostEffectResourceTransitionTest_CompareSlotMismatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded[0].slot = PostEffectManager::PostEffectResourceSlot::Ping;

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareSlotMismatch_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::SlotMismatch), "PostEffectResourceTransitionTest_CompareSlotMismatch_Reason");
	}

	void PostEffectResourceTransitionTest_CompareBeforeMismatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded[0].beforeState = D3D12_RESOURCE_STATE_COMMON;

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareBeforeMismatch_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::BeforeStateMismatch), "PostEffectResourceTransitionTest_CompareBeforeMismatch_Reason");
	}

	void PostEffectResourceTransitionTest_CompareAfterMismatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded[0].afterState = D3D12_RESOURCE_STATE_COMMON;

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareAfterMismatch_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::AfterStateMismatch), "PostEffectResourceTransitionTest_CompareAfterMismatch_Reason");
	}

	void PostEffectResourceTransitionTest_CompareStageMismatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded[0].stage = PostEffectManager::PostEffectStage::AfterEffect;

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareStageMismatch_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::StageMismatch), "PostEffectResourceTransitionTest_CompareStageMismatch_Reason");
	}

	void PostEffectResourceTransitionTest_CompareSequenceMismatch(TestResult &result) {
		const PostEffectManager manager;
		const std::vector<PostEffectManager::PostEffectResourceTransition> plan = manager.BuildResourceTransitionPlan(2, 0);
		std::vector<PostEffectManager::PostEffectResourceTransition> recorded = plan;
		recorded[1].sequence = 9;

		const PostEffectManager::PostEffectTransitionComparisonResult comparison =
			manager.CompareResourceTransitionPlanWithRecordedTransitions(plan, recorded);

		Expect(result, !comparison.isMatch, "PostEffectResourceTransitionTest_CompareSequenceMismatch_Result");
		Expect(result, HasMismatch(comparison, PostEffectManager::PostEffectTransitionMismatchReason::SequenceMismatch), "PostEffectResourceTransitionTest_CompareSequenceMismatch_Reason");
	}

	void PostEffectParameterSetTest_SourceTexture(TestResult &result) {
		PostEffectParameterSet parameters(PostEffectBindingLayout{0, PostEffectBindingLayout::kInvalidRootParameter});
		D3D12_GPU_DESCRIPTOR_HANDLE handle{};
		handle.ptr = 0x1234;
		parameters.SetSourceTexture(handle);

		Expect(result, parameters.GetLayout().sourceTextureRootParameter == 0, "PostEffectParameterSetTest_SourceTexture_RootParameter");
		Expect(result, parameters.GetSourceTextureForTesting().ptr == handle.ptr, "PostEffectParameterSetTest_SourceTexture_Handle");
		Expect(result, parameters.Validate().isValid, "PostEffectParameterSetTest_SourceTexture_Validate");
	}

	void PostEffectParameterSetTest_ConstantBuffer(TestResult &result) {
		PostEffectParameterSet parameters(PostEffectBindingLayout{0, 1});
		parameters.SetConstantBuffer(0x2000);

		Expect(result, parameters.GetLayout().constantBufferRootParameter == 1, "PostEffectParameterSetTest_ConstantBuffer_RootParameter");
		Expect(result, parameters.GetConstantBufferForTesting() == 0x2000, "PostEffectParameterSetTest_ConstantBuffer_Address");
		Expect(result, parameters.Validate().isValid, "PostEffectParameterSetTest_ConstantBuffer_Validate");
	}

	void PostEffectParameterSetTest_EffectLayouts(TestResult &result) {
		const PostEffectBindingLayout grayscaleLayout = GrayscaleEffect::CreateBindingLayout();
		const PostEffectBindingLayout vignettingLayout = Vignetting::CreateBindingLayout();
		const PostEffectBindingLayout fullscreenLayout = FullscreenPassRendere::CreateBindingLayout();

		Expect(result, grayscaleLayout.sourceTextureRootParameter == 0, "PostEffectParameterSetTest_Grayscale_SourceRoot");
		Expect(result, !grayscaleLayout.HasConstantBuffer(), "PostEffectParameterSetTest_Grayscale_NoConstantBuffer");
		Expect(result, vignettingLayout.sourceTextureRootParameter == 0, "PostEffectParameterSetTest_Vignetting_SourceRoot");
		Expect(result, !vignettingLayout.HasConstantBuffer(), "PostEffectParameterSetTest_Vignetting_NoConstantBuffer");
		Expect(result, fullscreenLayout.sourceTextureRootParameter == 0, "PostEffectParameterSetTest_Fullscreen_SourceRoot");
		Expect(result, !fullscreenLayout.HasConstantBuffer(), "PostEffectParameterSetTest_Fullscreen_NoConstantBuffer");
	}

	void PostEffectParameterSetTest_InvalidLayouts(TestResult &result) {
		PostEffectParameterSet missingSource(PostEffectBindingLayout{
			PostEffectBindingLayout::kInvalidRootParameter,
			PostEffectBindingLayout::kInvalidRootParameter});
		const PostEffectParameterValidationResult missingSourceResult = missingSource.Validate();
		Expect(result, !missingSourceResult.isValid, "PostEffectParameterSetTest_MissingSource_Invalid");
		Expect(result, missingSourceResult.error == PostEffectParameterValidationError::MissingSourceTextureRootParameter, "PostEffectParameterSetTest_MissingSource_Error");

		PostEffectParameterSet missingConstant(PostEffectBindingLayout{0, PostEffectBindingLayout::kInvalidRootParameter});
		missingConstant.SetConstantBuffer(0x3000);
		const PostEffectParameterValidationResult missingConstantResult = missingConstant.Validate();
		Expect(result, !missingConstantResult.isValid, "PostEffectParameterSetTest_MissingConstant_Invalid");
		Expect(result, missingConstantResult.error == PostEffectParameterValidationError::MissingConstantBufferRootParameter, "PostEffectParameterSetTest_MissingConstant_Error");

		PostEffectParameterSet duplicated(PostEffectBindingLayout{0, 0});
		const PostEffectParameterValidationResult duplicatedResult = duplicated.Validate();
		Expect(result, !duplicatedResult.isValid, "PostEffectParameterSetTest_Duplicate_Invalid");
		Expect(result, duplicatedResult.error == PostEffectParameterValidationError::DuplicateRootParameter, "PostEffectParameterSetTest_Duplicate_Error");
	}
}

int main() {
	TestResult result;

	RenderGraphTest_NormalFrame(result);
	RenderGraphTest_MissingBarrier(result);
	RenderGraphTest_InvalidBarrierBefore(result);
	RenderGraphTest_FinalStateMismatch(result);
	RenderGraphTest_MissingRequiredState(result);
	RenderGraphTest_MissingWriter(result);
	RenderGraphTest_DuplicateUsage(result);
	RenderGraphTest_DependencyOrderMismatch(result);
	RenderGraphTest_CircularDependency(result);
	RenderGraphTest_BuildTransitionPlan(result);
	RenderGraphTest_CompareTransitionPlanMatch(result);
	RenderGraphTest_CompareTransitionPlanMissing(result);
	RenderGraphTest_CompareTransitionPlanBeforeMismatch(result);
	RenderGraphTest_CompareTransitionPlanAfterMismatch(result);
	RenderGraphTest_CompareTransitionPlanSequenceMismatch(result);
	RenderGraphTest_CompareTransitionPlanBoundaryMismatch(result);
	RenderGraphTest_CompareTransitionPlanUnexpected(result);
	RenderGraphTest_LinePassResourceUsage(result);
	RenderGraphTest_LinePassMissingSceneColor(result);
	RenderGraphTest_LinePassMissingSceneDepth(result);
	RenderTransitionExecutorTest_BoundaryExecution(result);
	RenderTransitionExecutorTest_ResolveFailed(result);
	RenderTransitionExecutorTest_MissingPlan(result);
	RenderTransitionExecutorTest_DuplicateBoundary(result);
	RenderTransitionExecutorTest_UnsupportedPlanWhitelist(result);
	RenderBarrierRecorderTest_RecordTransition(result);
	RenderBarrierRecorderTest_InvalidInputs(result);
	PipelineRecipeTest_SpriteDefaults(result);
	PipelineRecipeTest_LineDepthWrite(result);
	PipelineRecipeTest_ParticleBlendAndDepth(result);
	PipelineRecipeTest_Object3dDefaults(result);
	PipelineRecipeTest_SkyboxDefaults(result);
	PipelineRecipeTest_CloudDefaults(result);
	PipelineRecipeTest_TrailDefaults(result);
	PipelineRecipeTest_PostEffectDefaults(result);
	PipelineRecipeTest_ValidationErrors(result);
	PostEffectResourceTransitionTest_OneEffectPlan(result);
	PostEffectResourceTransitionTest_TwoEffectPingPongPlan(result);
	PostEffectResourceTransitionTest_TwoEffectInitialPongPlan(result);
	PostEffectResourceTransitionTest_CompareMatch(result);
	PostEffectResourceTransitionTest_CompareMissing(result);
	PostEffectResourceTransitionTest_CompareSlotMismatch(result);
	PostEffectResourceTransitionTest_CompareBeforeMismatch(result);
	PostEffectResourceTransitionTest_CompareAfterMismatch(result);
	PostEffectResourceTransitionTest_CompareStageMismatch(result);
	PostEffectResourceTransitionTest_CompareSequenceMismatch(result);
	PostEffectParameterSetTest_SourceTexture(result);
	PostEffectParameterSetTest_ConstantBuffer(result);
	PostEffectParameterSetTest_EffectLayouts(result);
	PostEffectParameterSetTest_InvalidLayouts(result);

	std::cout << "Render validation tests: " << (result.total - result.failed) << "/" << result.total << " passed\n";
	return result.failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

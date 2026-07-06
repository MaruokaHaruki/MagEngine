/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#include "PostEffectManager.h"
#include "DirectXCore.h"
#include "PostEffectParameterSet.h"
#include "engine/render/graph/RenderBarrierRecorder.h"
#include "TextureManager.h"

#include <utility>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	// TODO: DXCore内に実装すること
	//       また､本当に重ねて実行できるかを確認すること
	///=============================================================================
	///                        初期化処理
	void PostEffectManager::Initialize(DirectXCore *dxCore) {
		dxCore_ = dxCore;

		grayscaleEffect_ = std::make_unique<GrayscaleEffect>();
		grayscaleEffect_->Initialize(dxCore);

		vignetting_ = std::make_unique<Vignetting>();
		vignetting_->Initialize(dxCore);
		// 他エフェクトもここで初期化
	}
	///=============================================================================
	///                        エフェクトの有効/無効設定
	void PostEffectManager::SetEffectEnabled(EffectType type, bool enabled) {
		effectEnabled_[static_cast<size_t>(type)] = enabled;
	}
	///=============================================================================
	///                        エフェクトの有効状態取得
	bool PostEffectManager::IsEffectEnabled(EffectType type) const {
		return effectEnabled_[static_cast<size_t>(type)];
	}

	uint32_t PostEffectManager::CountEnabledEffects() const {
		uint32_t enabledEffectCount = 0;
		for (size_t i = 0; i < static_cast<size_t>(EffectType::Count); ++i) {
			if (effectEnabled_[i]) {
				++enabledEffectCount;
			}
		}
		return enabledEffectCount;
	}

	std::vector<PostEffectManager::PostEffectResourceTransition> PostEffectManager::BuildResourceTransitionPlan() const {
		if (!dxCore_) {
			return {};
		}
		return BuildResourceTransitionPlan(CountEnabledEffects(), dxCore_->GetRenderResourceIndex());
	}

	PostEffectManager::PostEffectTransitionComparisonResult PostEffectManager::CompareResourceTransitionPlanWithRecordedTransitions() const {
		return CompareResourceTransitionPlanWithRecordedTransitions(resourceTransitionPlan_, resourceTransitions_);
	}

	///=============================================================================
	///                        エフェクトの適用
	void PostEffectManager::ApplyEffects(TextureManager &textureManager) {
		resourceTransitionPlan_.clear();
		resourceTransitions_.clear();

		// 有効なエフェクトの数をカウント
		int enabledEffectCount = 0;
		EffectType enabledEffects[static_cast<size_t>(EffectType::Count)];

		for (size_t i = 0; i < static_cast<size_t>(EffectType::Count); ++i) {
			if (effectEnabled_[i]) {
				enabledEffects[enabledEffectCount] = static_cast<EffectType>(i);
				enabledEffectCount++;
			}
		}

		// エフェクトがない場合はデフォルト描画
		if (enabledEffectCount == 0) {
			uint32_t renderResourceIndex = dxCore_->GetRenderResourceIndex();

			dxCore_->GetCommandList()->SetGraphicsRootSignature(dxCore_->GetRenderTextureRootSignature().Get());
			dxCore_->GetCommandList()->SetPipelineState(dxCore_->GetRenderTexturePipelineState().Get());

			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
			if (renderResourceIndex == 0) {
				srvHandle = textureManager.GetSrvHandleGPU("RenderTexture0");
			} else {
				srvHandle = textureManager.GetSrvHandleGPU("RenderTexture1");
			}

			assert(srvHandle.ptr != 0);
			PostEffectParameterSet parameters(PostEffectBindingLayout{0, PostEffectBindingLayout::kInvalidRootParameter});
			parameters.SetSourceTexture(srvHandle);
			parameters.Bind(*dxCore_->GetCommandList().Get());
			dxCore_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
			return;
		}

		// 複数エフェクトをピンポンバッファで適用
		uint32_t inputIndex = dxCore_->GetRenderResourceIndex();
		uint32_t outputIndex = 1 - inputIndex;
		resourceTransitionPlan_ = BuildResourceTransitionPlan(static_cast<uint32_t>(enabledEffectCount), inputIndex);

		for (int i = 0; i < enabledEffectCount; ++i) {
			// 最後のエフェクト以外は、次のエフェクト用にレンダーターゲットを切り替える
			if (i < enabledEffectCount - 1) {
				// レンダーターゲットに遷移
				TransitionPostEffectResource(
					ToResourceSlot(outputIndex),
					PostEffectStage::BeforeEffect,
					outputIndex,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET);
				SwitchRenderTarget(outputIndex);
			}

			// エフェクトを適用
			ApplySingleEffect(enabledEffects[i], inputIndex, outputIndex, textureManager);

			// 最後のエフェクト以外は、次の入力用にピクセルシェーダーリソースに遷移
			if (i < enabledEffectCount - 1) {
				TransitionPostEffectResource(
					ToResourceSlot(outputIndex),
					PostEffectStage::AfterEffect,
					outputIndex,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				// ピンポン: 入力と出力を入れ替え
				std::swap(inputIndex, outputIndex);
			}
		}
	}
	///=============================================================================
	///                        単一エフェクトを適用
	void PostEffectManager::ApplySingleEffect(EffectType effectType, uint32_t inputIndex, uint32_t outputIndex, TextureManager &textureManager) {
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
		PostEffectBindingLayout bindingLayout{};

		switch (effectType) {
		case EffectType::Grayscale:
			grayscaleEffect_->PreDraw();
			bindingLayout = grayscaleEffect_->GetBindingLayout();
			break;
		case EffectType::Vignette:
			vignetting_->PreDraw();
			bindingLayout = vignetting_->GetBindingLayout();
			break;
		// 他のエフェクトもここに追加
		default:
			return;
		}

		// 入力テクスチャを設定
		if (inputIndex == 0) {
			srvHandle = textureManager.GetSrvHandleGPU("RenderTexture0");
		} else {
			srvHandle = textureManager.GetSrvHandleGPU("RenderTexture1");
		}

		assert(srvHandle.ptr != 0);
		PostEffectParameterSet parameters(bindingLayout);
		parameters.SetSourceTexture(srvHandle);
		parameters.Bind(*dxCore_->GetCommandList().Get());
		dxCore_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}
	///=============================================================================
	///                        レンダーターゲットを切り替え
	void PostEffectManager::SwitchRenderTarget(uint32_t index) {
		auto *cmdList = dxCore_->GetCommandList().Get();
		// NOTE:RTV DescriptorはDirectXCore内のAllocator確保済みHandleを使う
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCore_->GetRenderTextureRtvHandle(index);

		// レンダーターゲットをクリア
		float clearColor[] = {0.298f, 0.427f, 0.698f, 1.0f};
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// レンダーターゲットを設定
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
	}
	///=============================================================================
	///                        テクスチャバリアを設定
	void PostEffectManager::SetTextureBarrier(uint32_t index, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
		TransitionPostEffectResource(
			ToResourceSlot(index),
			beforeState == D3D12_RESOURCE_STATE_RENDER_TARGET ? PostEffectStage::AfterEffect : PostEffectStage::BeforeEffect,
			index,
			beforeState,
			afterState);
	}

	void PostEffectManager::TransitionPostEffectResource(
		PostEffectResourceSlot slot,
		PostEffectStage stage,
		uint32_t index,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState) {
		assert(dxCore_);
		auto commandList = dxCore_->GetCommandList();
		auto resource = dxCore_->GetRenderTextureResource(index);
		assert(commandList);
		assert(resource);
		assert(dxCore_->GetRenderBarrierRecorder() && "PostEffect internal barriers require RenderBarrierRecorder after Renderer initialization.");

		// NOTE: PostEffect内部ResourceはSceneColorと同じ実体を共有し得るため、一般RenderGraphには固定IDとして露出しない。
		dxCore_->GetRenderBarrierRecorder()->Transition(*commandList.Get(), *resource.Get(), beforeState, afterState);
		resourceTransitions_.push_back(PostEffectResourceTransition{
			slot,
			beforeState,
			afterState,
			stage,
			static_cast<uint32_t>(resourceTransitions_.size()),
			index});
	}
}

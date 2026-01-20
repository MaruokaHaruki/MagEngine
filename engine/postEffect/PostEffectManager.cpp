/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#include "PostEffectManager.h"
#include "DirectXCore.h"
#include "TextureManager.h"

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
	///=============================================================================
	///                        エフェクトの適用
	void PostEffectManager::ApplyEffects() {
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
				srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture0");
			} else {
				srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture1");
			}

			assert(srvHandle.ptr != 0);
			dxCore_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvHandle);
			dxCore_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
			return;
		}

		// 複数エフェクトをピンポンバッファで適用
		uint32_t inputIndex = dxCore_->GetRenderResourceIndex();
		uint32_t outputIndex = 1 - inputIndex;

		for (int i = 0; i < enabledEffectCount; ++i) {
			// 最後のエフェクト以外は、次のエフェクト用にレンダーターゲットを切り替える
			if (i < enabledEffectCount - 1) {
				// レンダーターゲットに遷移
				SetTextureBarrier(outputIndex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				SwitchRenderTarget(outputIndex);
			}

			// エフェクトを適用
			ApplySingleEffect(enabledEffects[i], inputIndex, outputIndex);

			// 最後のエフェクト以外は、次の入力用にピクセルシェーダーリソースに遷移
			if (i < enabledEffectCount - 1) {
				SetTextureBarrier(outputIndex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				// ピンポン: 入力と出力を入れ替え
				std::swap(inputIndex, outputIndex);
			}
		}
	}
	///=============================================================================
	///                        単一エフェクトを適用
	void PostEffectManager::ApplySingleEffect(EffectType effectType, uint32_t inputIndex, uint32_t outputIndex) {
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;

		switch (effectType) {
		case EffectType::Grayscale:
			grayscaleEffect_->PreDraw();
			break;
		case EffectType::Vignette:
			vignetting_->PreDraw();
			break;
		// 他のエフェクトもここに追加
		default:
			return;
		}

		// 入力テクスチャを設定
		if (inputIndex == 0) {
			srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture0");
		} else {
			srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture1");
		}

		assert(srvHandle.ptr != 0);
		dxCore_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvHandle);
		dxCore_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	}
	///=============================================================================
	///                        レンダーターゲットを切り替え
	void PostEffectManager::SwitchRenderTarget(uint32_t index) {
		auto *cmdList = dxCore_->GetCommandList().Get();
		auto rtvDescriptorHeap = dxCore_->GetRtvDescriptorHeap();
		uint32_t descriptorSize = dxCore_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// RTVハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += static_cast<unsigned long long>(descriptorSize) * (2 + index);

		// レンダーターゲットをクリア
		float clearColor[] = {0.298f, 0.427f, 0.698f, 1.0f};
		cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// レンダーターゲットを設定
		cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
	}
	///=============================================================================
	///                        テクスチャバリアを設定
	void PostEffectManager::SetTextureBarrier(uint32_t index, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = dxCore_->GetRenderTextureResource(index).Get();
		barrier.Transition.StateBefore = beforeState;
		barrier.Transition.StateAfter = afterState;

		dxCore_->GetCommandList()->ResourceBarrier(1, &barrier);
	}
}
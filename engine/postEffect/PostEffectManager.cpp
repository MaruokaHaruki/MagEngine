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

// TODO: DXCore内に実装すること
//       また､本当に重ねて実行できるかを確認すること
///=============================================================================
///                        初期化処理
void PostEffectManager::Initialize(DirectXCore *dxCore) {
	dxCore_ = dxCore;

	grayscaleEffect_ = std::make_unique<GrayscaleEffect>();
	grayscaleEffect_->Initialize(dxCore);
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
	// Grayscale
	if (effectEnabled_[static_cast<size_t>(EffectType::Grayscale)]) {
		grayscaleEffect_->PreDraw();

		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
		uint32_t renderResourceIndex = 0; // TODO: DirectXCoreから取得する方法を検討

		if (renderResourceIndex == 0) {
			srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture0");
		} else {
			srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture1");
		}

		assert(srvHandle.ptr != 0);
		dxCore_->GetCommandList()->SetGraphicsRootDescriptorTable(0, srvHandle);
		dxCore_->GetCommandList()->DrawInstanced(3, 1, 0, 0);
	} 
	// 他エフェクトも同様に
}

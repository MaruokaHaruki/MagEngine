/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#include "PostEffectManager.h"
#include "DirectXCore.h"

// TODO: DXCore内に実装すること
//       また､本当に重ねて実行できるかを確認すること
///=============================================================================
///                        初期化処理
void PostEffectManager::Initialize(DirectXCore *dxCore) {
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
		// ...必要ならPostDrawも
	}
	// 他エフェクトも同様に
}

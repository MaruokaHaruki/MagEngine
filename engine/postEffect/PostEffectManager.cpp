/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#include "PostEffectManager.h"
#include "DirectXCore.h"

void PostEffectManager::Initialize(DirectXCore *dxCore) {
	grayscaleEffect_ = std::make_unique<GrayscaleEffect>();
	grayscaleEffect_->Initialize(dxCore);
	// 他エフェクトもここで初期化
}

void PostEffectManager::SetEffectEnabled(EffectType type, bool enabled) {
	effectEnabled_[static_cast<size_t>(type)] = enabled;
}

bool PostEffectManager::IsEffectEnabled(EffectType type) const {
	return effectEnabled_[static_cast<size_t>(type)];
}

void PostEffectManager::ApplyEffects() {
	// Grayscale
	if (effectEnabled_[static_cast<size_t>(EffectType::Grayscale)]) {
		grayscaleEffect_->PreDraw();
		// ...必要ならPostDrawも
	}
	// 他エフェクトも同様に
}

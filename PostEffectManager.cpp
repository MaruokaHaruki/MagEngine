#include "PostEffectManager.h"

void PostEffectManager::Initialize() {
	effects_.clear();
}

void PostEffectManager::AddEffect(std::shared_ptr<GrayscaleEffect> effect) {
	effects_.push_back(effect);
}

void PostEffectManager::ApplyEffects() {
	for (auto &effect : effects_) {
		effect->Draw(); // 各エフェクトでPing-Pong描画
	}
}

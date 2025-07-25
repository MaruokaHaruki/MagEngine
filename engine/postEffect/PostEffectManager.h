#pragma once
//#include "GrayscaleEffect.h"
#include <memory>
#include <vector>
// 今後グリッチ等も追加予定

class PostEffectManager {
public:
	void Initialize();
	//void AddEffect(std::shared_ptr<GrayscaleEffect> effect); // 今後は基底クラス化推奨
	void ApplyEffects();

private:
	//std::vector<std::shared_ptr<GrayscaleEffect>> effects_; // 今後は基底クラス化推奨
};

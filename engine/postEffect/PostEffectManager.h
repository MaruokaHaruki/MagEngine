/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#pragma once
#include "GrayscaleEffect.h"
#include <memory>

class DirectXCore;

class PostEffectManager {
public:
	// 利用可能なエフェクト種別
	enum class EffectType {
		Grayscale,
		// 今後追加する場合はここに列挙
		Count
	};

	void Initialize(DirectXCore *dxCore);

	// エフェクトのON/OFF切り替え
	void SetEffectEnabled(EffectType type, bool enabled);

	// エフェクトが有効かどうか取得
	bool IsEffectEnabled(EffectType type) const;

	// 有効なエフェクトを適用
	void ApplyEffects();

private:
	// 各エフェクトのON/OFF状態
	bool effectEnabled_[static_cast<size_t>(EffectType::Count)] = {};

	// 各エフェクトのインスタンス
	std::unique_ptr<GrayscaleEffect> grayscaleEffect_;
	// 今後追加する場合はここにメンバ追加
};

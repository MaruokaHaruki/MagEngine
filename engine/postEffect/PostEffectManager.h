/*********************************************************************
 * \file   PostEffectManager.h
 * \brief  ポストエフェクトマネージャークラス
 *
 * \author Harukichimaru
 * \date   July 2025
 *********************************************************************/
#pragma once
#include "GrayscaleEffect.h"
#include "Vignetting.h"
#include <memory>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	class DirectXCore;

	class PostEffectManager {
	public:
		// 利用可能なエフェクト種別
		enum class EffectType {
			// 基本エフェクト
			Grayscale,
			Vignette,
			Smooth,
			GaussianBlur,
			Outline,
			RadisleBlur,
			Dissolve,
			RandomNoise,
			// 特殊エフェクト
			CRT,
			PS1,
			Count
		};

		void Initialize(DirectXCore *dxCore);

		// エフェクトのON/OFF切り替え
		void SetEffectEnabled(EffectType type, bool enabled);

		// エフェクトが有効かどうか取得
		bool IsEffectEnabled(EffectType type) const;

		// 有効なエフェクトを適用
		void ApplyEffects();

		// DirectXCoreへのアクセス
		DirectXCore *GetDXCore() const {
			return dxCore_;
		}

	private:
		// DirectXCoreポインタ
		DirectXCore *dxCore_ = nullptr;

		// 各エフェクトのON/OFF状態
		bool effectEnabled_[static_cast<size_t>(EffectType::Count)] = {};

		// 各エフェクトのインスタンス
		std::unique_ptr<GrayscaleEffect> grayscaleEffect_;
		std::unique_ptr<Vignetting> vignetting_;
		// 今後追加する場合はここにメンバ追加

		//========================================
		/// @brief 単一エフェクトを適用
		/// @param effectType エフェクトタイプ
		/// @param inputIndex 入力テクスチャインデックス
		/// @param outputIndex 出力テクスチャインデックス
		void ApplySingleEffect(EffectType effectType, uint32_t inputIndex, uint32_t outputIndex);

		//========================================
		/// @brief レンダーターゲットを切り替え
		/// @param index レンダーターゲットインデックス
		void SwitchRenderTarget(uint32_t index);

		//========================================
		/// @brief テクスチャバリアを設定
		/// @param index テクスチャインデックス
		/// @param beforeState 遷移前の状態
		/// @param afterState 遷移後の状態
		void SetTextureBarrier(uint32_t index, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	};
}
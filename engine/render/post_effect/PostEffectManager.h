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
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <utility>
#include <vector>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	class DirectXCore;
	class TextureManager;

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

		enum class PostEffectResourceSlot : uint8_t {
			Ping,
			Pong,
		};

		enum class PostEffectStage : uint8_t {
			BeforeEffect,
			AfterEffect,
		};

		struct PostEffectResourceTransition {
			PostEffectResourceSlot slot = PostEffectResourceSlot::Ping;
			D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_COMMON;
			D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_COMMON;
			PostEffectStage stage = PostEffectStage::BeforeEffect;
			uint32_t sequence = 0;
			uint32_t resourceIndex = 0;
		};

		enum class PostEffectTransitionMismatchReason : uint8_t {
			MissingRecordedTransition,
			UnexpectedRecordedTransition,
			SlotMismatch,
			BeforeStateMismatch,
			AfterStateMismatch,
			StageMismatch,
			SequenceMismatch,
		};

		struct PostEffectTransitionMismatch {
			PostEffectTransitionMismatchReason reason = PostEffectTransitionMismatchReason::MissingRecordedTransition;
			PostEffectResourceTransition expected{};
			PostEffectResourceTransition actual{};
		};

		struct PostEffectTransitionComparisonResult {
			bool isMatch = true;
			size_t plannedCount = 0;
			size_t recordedCount = 0;
			std::vector<PostEffectTransitionMismatch> mismatches;
		};

		void Initialize(DirectXCore *dxCore);

		// エフェクトのON/OFF切り替え
		void SetEffectEnabled(EffectType type, bool enabled);

		// エフェクトが有効かどうか取得
		bool IsEffectEnabled(EffectType type) const;

		// 有効なエフェクトを適用
		void ApplyEffects(TextureManager &textureManager);

		const std::vector<PostEffectResourceTransition> &GetResourceTransitions() const {
			return resourceTransitions_;
		}

		const std::vector<PostEffectResourceTransition> &GetResourceTransitionPlan() const {
			return resourceTransitionPlan_;
		}

		[[nodiscard]]
		std::vector<PostEffectResourceTransition> BuildResourceTransitionPlan() const;

		[[nodiscard]]
		std::vector<PostEffectResourceTransition> BuildResourceTransitionPlan(uint32_t enabledEffectCount, uint32_t initialInputIndex) const;

		[[nodiscard]]
		PostEffectTransitionComparisonResult CompareResourceTransitionPlanWithRecordedTransitions() const;

		[[nodiscard]]
		PostEffectTransitionComparisonResult CompareResourceTransitionPlanWithRecordedTransitions(
			const std::vector<PostEffectResourceTransition> &plan,
			const std::vector<PostEffectResourceTransition> &recordedTransitions) const;

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

		std::vector<PostEffectResourceTransition> resourceTransitionPlan_;
		std::vector<PostEffectResourceTransition> resourceTransitions_;

		//========================================
		/// @brief 単一エフェクトを適用
		/// @param effectType エフェクトタイプ
		/// @param inputIndex 入力テクスチャインデックス
		/// @param outputIndex 出力テクスチャインデックス
		void ApplySingleEffect(EffectType effectType, uint32_t inputIndex, uint32_t outputIndex, TextureManager &textureManager);

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

		void TransitionPostEffectResource(
			PostEffectResourceSlot slot,
			PostEffectStage stage,
			uint32_t index,
			D3D12_RESOURCE_STATES beforeState,
			D3D12_RESOURCE_STATES afterState);

		[[nodiscard]]
		uint32_t CountEnabledEffects() const;

		[[nodiscard]]
		static PostEffectResourceSlot ToResourceSlot(uint32_t index);
	};

	inline PostEffectManager::PostEffectResourceSlot PostEffectManager::ToResourceSlot(uint32_t index) {
		return index == 0 ? PostEffectResourceSlot::Ping : PostEffectResourceSlot::Pong;
	}

	inline std::vector<PostEffectManager::PostEffectResourceTransition> PostEffectManager::BuildResourceTransitionPlan(uint32_t enabledEffectCount, uint32_t initialInputIndex) const {
		std::vector<PostEffectResourceTransition> plan;
		if (enabledEffectCount <= 1) {
			return plan;
		}

		uint32_t inputIndex = initialInputIndex;
		uint32_t outputIndex = 1 - inputIndex;
		uint32_t sequence = 0;

		for (uint32_t i = 0; i < enabledEffectCount; ++i) {
			if (i < enabledEffectCount - 1) {
				// NOTE: 既存実装で実際に発行していた中間RTV化だけを計画化し、Source側の推測Barrierは増やさない。
				plan.push_back(PostEffectResourceTransition{
					ToResourceSlot(outputIndex),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					PostEffectStage::BeforeEffect,
					sequence++,
					outputIndex});
			}

			if (i < enabledEffectCount - 1) {
				// NOTE: 次段のSRV入力に戻す既存BarrierをPlanへ残し、Ping-Pong切替の検証根拠にする。
				plan.push_back(PostEffectResourceTransition{
					ToResourceSlot(outputIndex),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					PostEffectStage::AfterEffect,
					sequence++,
					outputIndex});
				std::swap(inputIndex, outputIndex);
			}
		}

		return plan;
	}

	inline PostEffectManager::PostEffectTransitionComparisonResult PostEffectManager::CompareResourceTransitionPlanWithRecordedTransitions(
		const std::vector<PostEffectResourceTransition> &plan,
		const std::vector<PostEffectResourceTransition> &recordedTransitions) const {
		PostEffectTransitionComparisonResult result{};
		result.plannedCount = plan.size();
		result.recordedCount = recordedTransitions.size();
		result.isMatch = plan.size() == recordedTransitions.size();

		const size_t maxCount = plan.size() > recordedTransitions.size() ? plan.size() : recordedTransitions.size();
		for (size_t i = 0; i < maxCount; ++i) {
			if (i >= recordedTransitions.size()) {
				result.mismatches.push_back(PostEffectTransitionMismatch{
					PostEffectTransitionMismatchReason::MissingRecordedTransition,
					plan[i],
					{}});
				result.isMatch = false;
				continue;
			}
			if (i >= plan.size()) {
				result.mismatches.push_back(PostEffectTransitionMismatch{
					PostEffectTransitionMismatchReason::UnexpectedRecordedTransition,
					{},
					recordedTransitions[i]});
				result.isMatch = false;
				continue;
			}

			const PostEffectResourceTransition &expected = plan[i];
			const PostEffectResourceTransition &actual = recordedTransitions[i];
			if (expected.slot != actual.slot) {
				result.mismatches.push_back(PostEffectTransitionMismatch{PostEffectTransitionMismatchReason::SlotMismatch, expected, actual});
			}
			if (expected.beforeState != actual.beforeState) {
				result.mismatches.push_back(PostEffectTransitionMismatch{PostEffectTransitionMismatchReason::BeforeStateMismatch, expected, actual});
			}
			if (expected.afterState != actual.afterState) {
				result.mismatches.push_back(PostEffectTransitionMismatch{PostEffectTransitionMismatchReason::AfterStateMismatch, expected, actual});
			}
			if (expected.stage != actual.stage) {
				result.mismatches.push_back(PostEffectTransitionMismatch{PostEffectTransitionMismatchReason::StageMismatch, expected, actual});
			}
			if (expected.sequence != actual.sequence) {
				result.mismatches.push_back(PostEffectTransitionMismatch{PostEffectTransitionMismatchReason::SequenceMismatch, expected, actual});
			}
		}

		result.isMatch = result.isMatch && result.mismatches.empty();
		return result;
	}
}

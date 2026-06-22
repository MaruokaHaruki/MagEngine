/*********************************************************************
 * \file   RenderGraph.h
 * \brief  RenderPass間の論理リソース依存を構築・検証するクラス
 *********************************************************************/
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace MagEngine {
	class RenderBarrierRecorder;
	enum class RenderPassId : uint8_t;
	struct RenderPassEntry;

	enum class RenderResourceId : uint8_t {
		SceneColor,
		SceneDepth,
		PresentColor,
	};

	enum class RenderResourceAccess : uint8_t {
		Read,
		Write,
		ReadWrite,
	};

	enum class RenderResourceState : uint8_t {
		Unknown,
		RenderTarget,
		PixelShaderResource,
		DepthWrite,
		DepthRead,
		Present,
		CopySource,
		CopyDest,
		GenericRead,
	};

	enum class RenderBarrierPoint : uint8_t {
		RenderTexturePreDraw,
		RenderTexturePostDraw,
		BeginPresentRenderTarget,
		BeforePresent,
	};

	struct RenderPassResourceUsage {
		RenderResourceId resource;
		RenderResourceAccess access;
		RenderResourceState requiredState = RenderResourceState::Unknown;
	};

	struct RenderPassDependency {
		RenderPassId before;
		RenderPassId after;
		RenderResourceId resource;
	};

	struct RenderResourceBarrierRecord {
		RenderResourceId resource;
		RenderResourceState beforeState = RenderResourceState::Unknown;
		RenderResourceState afterState = RenderResourceState::Unknown;
		RenderBarrierPoint point = RenderBarrierPoint::RenderTexturePreDraw;
		uint32_t sequence = 0;
	};

	struct RenderResourceStateRecord {
		RenderResourceId resource;
		RenderResourceState state = RenderResourceState::Unknown;
	};

	std::string_view ToString(RenderResourceId resourceId);
	std::string_view ToString(RenderResourceAccess access);
	std::string_view ToString(RenderResourceState state);
	std::string_view ToString(RenderBarrierPoint point);

	class RenderGraph {
	public:
		/// @brief Graph外で初期化済みのリソースを登録
		void AddExternalResource(RenderResourceId resourceId);

		/// @brief フレーム開始時に手動Barrierが前提とする状態を登録
		void SetInitialResourceState(RenderResourceId resourceId, RenderResourceState state);

		/// @brief フレーム終了時に保証したい状態を登録
		void SetFinalResourceState(RenderResourceId resourceId, RenderResourceState state);

		/// @brief Pass登録完了後に一度だけ依存関係を構築する
		void Build(const std::vector<RenderPassEntry> &passes);

		/// @brief 構築済み依存関係が現在の実行順と矛盾しないことを検証
		void Validate(const std::vector<RenderPassEntry> &passes) const;

		/// @brief フレーム中に統合ヘルパーが記録したBarrierを破棄
		void ClearRecordedBarriers();

		/// @brief 実行済みBarrier列とPass Required Stateの整合性を検証
		void ValidateRecordedResourceStates(const std::vector<RenderPassEntry> &passes) const;

		const std::vector<RenderPassDependency> &GetDependencies() const {
			return dependencies_;
		}

		const std::vector<RenderResourceId> &GetExternalResources() const {
			return externalResources_;
		}

		const std::vector<RenderResourceBarrierRecord> &GetManualBarriers() const {
			return manualBarriers_;
		}

		const std::vector<RenderResourceStateRecord> &GetInitialResourceStates() const {
			return initialResourceStates_;
		}

		const std::vector<RenderResourceStateRecord> &GetFinalResourceStates() const {
			return finalResourceStates_;
		}

	private:
		friend class RenderBarrierRecorder;

		/// @brief 統合Barrier発行ヘルパーから実行済みBarrierだけを記録する
		void RecordManualBarrier(const RenderResourceBarrierRecord &record);

		bool IsExternalResource(RenderResourceId resourceId) const;
		std::optional<RenderResourceState> FindInitialState(RenderResourceId resourceId) const;
		std::optional<RenderResourceState> FindFinalState(RenderResourceId resourceId) const;
		RenderResourceState &FindCurrentState(std::vector<RenderResourceStateRecord> &states, RenderResourceId resourceId) const;
		bool HasDependency(const RenderPassDependency &dependency) const;
		void AddDependency(RenderPassId before, RenderPassId after, RenderResourceId resource);
		void ValidateDuplicateUsages(const RenderPassEntry &pass) const;
		void ValidateWriteConflict(const std::vector<RenderPassEntry> &passes) const;
		void ValidateAcyclic(const std::vector<RenderPassEntry> &passes) const;
		void ValidateExecutionOrder(const std::vector<RenderPassEntry> &passes) const;
		void ValidateRequiredStates(const std::vector<RenderPassEntry> &passes) const;
		void ValidateResourceStates(const std::vector<RenderPassEntry> &passes) const;

		// NOTE: GPU Resource本体は所有しない。論理リソース名だけで依存を検証する。
		std::vector<RenderResourceId> externalResources_;
		std::vector<RenderPassDependency> dependencies_;
		std::vector<RenderResourceStateRecord> initialResourceStates_;
		std::vector<RenderResourceStateRecord> finalResourceStates_;
		std::vector<RenderResourceBarrierRecord> manualBarriers_;
	};
}

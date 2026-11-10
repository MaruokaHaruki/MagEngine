/*********************************************************************
 * \file WindowSurfaceState.h
 * \brief GPUへ渡す最新のクライアント寸法と適用済み寸法を管理する
 *********************************************************************/
#pragma once
#include <cstdint>
#include <optional>

///=============================================================================
/// ウィンドウ寸法の要求と適用状態
namespace MagEngine {
	/// @brief ピクセル単位の描画寸法
	struct SurfaceSize {
		uint32_t width = 1280, height = 720;
		bool operator==(const SurfaceSize &) const = default;
	};
	class WindowSurfaceState {
	public:
		///--------------------------------------------------------------
		/// 要求と成功時の反映（ウィンドウ所有スレッドで使用）
		/// @brief 最新要求で上書きする。片辺0は最小化扱い
		void Request(SurfaceSize size) { requested_ = size; }
		/// @brief GPU更新成功後のみ適用済み寸法を記録する
		void Commit(SurfaceSize size) { if (size.width && size.height) applied_ = size; }
		/// @brief 生成可能な変更要求。最小化・変更なしでは空
		std::optional<SurfaceSize> Pending() const {
			if (IsMinimized() || requested_ == applied_) return std::nullopt;
			return requested_;
		}
		/// @brief クライアントが描画できない状態か
		bool IsMinimized() const { return !requested_.width || !requested_.height; }
		/// @brief GPUが使用中の適用済み寸法
		SurfaceSize Applied() const { return applied_; }
		/// @brief 最新クライアント寸法。最小化中は0を含む
		SurfaceSize Requested() const { return requested_; }
	private:
		///--------------------------------------------------------------
		/// GPU更新まで旧寸法を保持する
		SurfaceSize requested_{}, applied_{};
	};
}

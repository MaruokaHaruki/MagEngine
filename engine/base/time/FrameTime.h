#pragma once

#include <algorithm>
#include <cmath>

///=============================================================================
///                        フレーム時間
///
/// EngineとSceneの境界で使用する、ゲーム固有ではない経過時間。
/// TimeScaleはApplication層で解決するため、この型には保持しない。
struct FrameTime {
	static constexpr float kMaximumUnscaledDeltaTime = 0.1f; // 停止復帰時の大きな時間差分をScene更新へ渡さない上限（秒）。

	float rawDeltaTime = 0.0f;
	float unscaledDeltaTime = 0.0f;

	/// @brief 計測値を検証し、Scene更新に渡せるフレーム時間を作成
	/// @details rawDeltaTimeは計測値を保持し、unscaledDeltaTimeのみを安全な範囲へ制限する。
	[[nodiscard]] static FrameTime Create(float rawDeltaTime) {
		// NOTE: 計測境界で不正値を除外し、下流のSceneやゲームロジックへ伝播させない。
		const float sanitizedRawDeltaTime = std::isfinite(rawDeltaTime)
			? (std::max)(0.0f, rawDeltaTime)
			: 0.0f;

		return {
			sanitizedRawDeltaTime,
			std::clamp(sanitizedRawDeltaTime, 0.0f, kMaximumUnscaledDeltaTime),
		};
	}
};

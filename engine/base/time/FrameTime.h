#pragma once

#include <algorithm>
#include <cmath>

///=============================================================================
///                        フレーム時間
///
/// EngineとSceneの境界で使用する、ゲーム固有ではない経過時間。
/// TimeScaleはApplication層で解決するため、この型には保持しない。
struct FrameTime {
	static constexpr float kMaximumUnscaledDeltaTime = 0.1f;

	float rawDeltaTime = 0.0f;
	float unscaledDeltaTime = 0.0f;

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

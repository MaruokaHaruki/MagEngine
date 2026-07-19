#pragma once

namespace MagEngine {
	/// @brief スプライトをUI空間またはワールド空間のどちらで描画するかを表す区分
	/// @note 選択した値はSpriteの座標解釈と描画用行列に影響するため、登録後に用途を混在させない。
	enum class SpriteRenderMode {
		Ui,
		World,
	};
}

/*********************************************************************
 * \file EditorViewport.h
 * \brief 両起動モードで共有する画像領域とゲーム入力の所有権
 *********************************************************************/
#pragma once
#include "EditorViewportMath.h"
#include <functional>
namespace MagEngine {
	class EditorViewport {
	public:
		/// @brief パネル未表示でも古い画像領域へ入力を流さないため、毎フレーム先に呼ぶ
		void BeginFrame();
		void Draw(uint64_t texture, MagMath::Vector2 renderSize);
		const EditorViewportInputState &GetInputState() const { return input_; }
		/// @brief 画像の後に編集用選択・軸だけを描く。所有元破棄前に解除する
		void SetOverlay(std::function<void(const ViewportRect &)> overlay) { overlay_ = std::move(overlay); }
	private:
		EditorViewportInputState input_;
		std::function<void(const ViewportRect &)> overlay_;
	};
}

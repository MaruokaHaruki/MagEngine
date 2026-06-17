/*********************************************************************
 * \file   RenderWorld.h
 * \brief  フレーム単位の描画対象を保持するクラス
 *********************************************************************/
#pragma once

#include <vector>

namespace MagEngine {
	class Object3d;

	struct OpaqueRenderItem {
		Object3d *object = nullptr;
		bool visible = true;
	};

	class RenderWorld {
	public:
		/// @brief 前フレームの非所有参照を消し、確保済み容量は再利用する
		void Clear();

		/// @brief 3D不透明描画対象を登録
		void AddOpaque(const OpaqueRenderItem &item);

		/// @brief 3D不透明描画対象を取得
		const std::vector<OpaqueRenderItem> &GetOpaqueItems() const {
			return opaqueItems_;
		}

	private:
		// NOTE: SceneやGameObjectの所有権は持たない。参照先は描画完了までScene側が保持する。
		std::vector<OpaqueRenderItem> opaqueItems_;
	};
}

/*********************************************************************
 * \file   TrailEffectPreset.h
 * \brief  トレイルエフェクトプリセット定義
 *
 * \author MagEngine
 * \date   March 2026
 * \note   トレイルエフェクトの設定テンプレート
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include <map>
#include <string>

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						プリセット構造体

	/**----------------------------------------------------------------------------
	 * \brief  TrailEffectPreset トレイルエフェクトプリセット
	 * \note   複数のトレイルエフェクトで再利用可能な設定情報
	 */
	struct TrailEffectPreset {
		// 基本情報
		std::string name; ///< プリセット名（例："SmokeTrail"）

		// ビジュアルパラメータ
		MagMath::Vector3 color{0.5f, 0.5f, 0.5f}; ///< エフェクト色
		float opacity = 0.8f;					  ///< 透明度 (0.0-1.0)
		float width = 2.0f;						  ///< 軌跡幅

		// ライフサイクル
		float lifeTime = 3.0f;		///< ライフタイム（秒）
		float emissionRate = 50.0f; ///< 生成レート（個/フレーム）

		// 物理パラメータ
		float velocityDamping = 0.95f; ///< 速度減衰 (0.0-1.0)
		float gravityInfluence = 0.2f; ///< 重力影響度

		// シェーダー情報
		std::string shaderName = "TrailDefault"; ///< 使用シェーダー名

		// 拡張用キー・バリュー
		std::map<std::string, float> customParams; ///< 拡張パラメータ
	};
}

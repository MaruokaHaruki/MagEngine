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

		//========================================
		// ライフタイム設定
		float lifeTime = 3.0f;		   ///< ライフタイム（秒）
		float minPointDistance = 0.1f; ///< ポイント生成の最小距離
		uint32_t maxPoints = 128;	   ///< 最大ポイント数

		//========================================
		// 幅設定
		float width = 2.0f;			  ///< 基本軌跡幅
		float widthMultiplier = 1.0f; ///< 幅乗数
		float startWidth = 1.0f;	  ///< 開始時の幅（相対値）
		float endWidth = 0.0f;		  ///< 終了時の幅（相対値）

		//========================================
		// カラー設定
		MagMath::Vector3 color{0.5f, 0.5f, 0.5f};	   ///< エフェクト色（結合用）
		float opacity = 0.8f;						   ///< 透明度 (0.0-1.0)
		MagMath::Vector3 startColor{1.0f, 1.0f, 1.0f}; ///< グラデーション開始色
		MagMath::Vector3 endColor{0.0f, 0.0f, 0.0f};   ///< グラデーション終了色

		//========================================
		// アライメント・シミュレーション空間
		uint32_t alignment = 0;		  ///< 0=World, 1=Local, 2=View
		uint32_t simulationSpace = 0; ///< 0=World, 1=Local

		//========================================
		// マテリアル設定
		std::string material = "Default";	 ///< マテリアル名
		uint32_t textureMode = 0;			 ///< 0=None, 1=Tiled, 2=Stretched
		MagMath::Vector2 tiling{1.0f, 1.0f}; ///< テクスチャタイリング
		MagMath::Vector2 offset{0.0f, 0.0f}; ///< テクスチャオフセット

		//========================================
		// 発光・クリア設定
		bool emitting = true;	///< 発光状態
		bool autoClear = false; ///< 自動クリア

		//========================================
		// コーナー・細分化設定
		float cornerSmooth = 0.1f; ///< コーナースムーズ度
		uint32_t subdivisions = 1; ///< サブディビジョン数

		//========================================
		// ノイズ設定
		float noiseAmplitude = 0.0f; ///< ノイズ振幅
		float noiseFrequency = 1.0f; ///< ノイズ周波数

		//========================================
		// フェード設定
		uint32_t fadeMode = 0; ///< 0=Linear, 1=EaseOut, 2=EaseIn

		//========================================
		// 物理パラメータ
		float emissionRate = 50.0f;	   ///< 生成レート（個/フレーム）
		float velocityDamping = 0.95f; ///< 速度減衰 (0.0-1.0)
		float gravityInfluence = 0.2f; ///< 重力影響度

		//========================================
		// シェーダー情報
		std::string shaderName = "TrailDefault"; ///< 使用シェーダー名

		//========================================
		// 拡張用キー・バリュー
		std::map<std::string, float> customParams; ///< 拡張パラメータ
	};
}

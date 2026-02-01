#pragma once
/*
 * クラウドインパクト統合用ヘッダー
 *
 * 使用方法：
 * 1. GamePlaySceneにスタティックアクセッサを追加
 * 2. 弾丸のOnCollisionEnter()で呼び出す
 */

#include "Cloud.h"
#include "MagMath.h"

class CloudImpactHelper {
public:
	/// @brief グローバルクラウドインスタンスを設定
	/// @param cloud 雲ポインタ
	static void SetGlobalCloud(MagEngine::Cloud *cloud) {
		globalCloud_ = cloud;
	}

	/// @brief 弾丸衝突時の影響を適用
	/// @param position 衝突位置
	/// @param bulletType 弾丸のタイプ（プレイヤー or 敵）
	static void ApplyBulletImpact(const MagMath::Vector3 &position, bool isPlayerBullet = true) {
		if (!globalCloud_)
			return;

		// プレイヤー弾と敵弾で異なるパラメータを使用
		if (isPlayerBullet) {
			// 自機弾：大きめの効果
			globalCloud_->AddImpact(
				position,
				35.0f, // 影響半径
				0.75f, // 強度
				1.2f   // 復帰時間
			);
		} else {
			// 敵弾：小さめの効果
			globalCloud_->AddImpact(
				position,
				25.0f,
				0.6f,
				0.8f);
		}
	}

	/// @brief 爆発時の影響を適用
	/// @param position 爆発位置
	/// @param explosionRadius 爆発の範囲
	static void ApplyExplosionImpact(const MagMath::Vector3 &position, float explosionRadius = 50.0f) {
		if (!globalCloud_)
			return;

		globalCloud_->AddImpact(
			position,
			explosionRadius * 0.7f, // 爆発範囲に応じて調整
			0.85f,					// 高い強度
			1.5f					// 復帰時間
		);
	}

private:
	static MagEngine::Cloud *globalCloud_;
};

// Static member initialization
// CloudImpactHelper.cpp に以下を追加：
// MagEngine::Cloud *CloudImpactHelper::globalCloud_ = nullptr;

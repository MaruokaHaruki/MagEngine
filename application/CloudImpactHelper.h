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
			// 自機弾：強い衝撃で明確な吹き飛び効果
			globalCloud_->AddImpact(
				position,
				45.0f, // 影響半径（より広い範囲に影響）
				0.85f, // 強度（高い強度で効果を強調）
				1.5f   // 復帰時間（ゆっくり戻る）
			);
		} else {
			// 敵弾：小さめで短い効果
			globalCloud_->AddImpact(
				position,
				30.0f,
				0.65f,
				1.0f);
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
			explosionRadius * 0.8f, // 爆発範囲に応じて調整（より大きな範囲）
			0.9f,					// 非常に高い強度（爆発らしさを表現）
			2.0f					// 長い復帰時間（爆発の余韻を表現）
		);
	}

private:
	static MagEngine::Cloud *globalCloud_;
};

// Static member initialization
// CloudImpactHelper.cpp に以下を追加：
// MagEngine::Cloud *CloudImpactHelper::globalCloud_ = nullptr;

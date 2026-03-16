/*********************************************************************
 * \file   SceneContext.h
 * \brief  シーンが必要とするすべての共通リソースをまとめるコンテキストクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: Initialize関数の引数削減のため、セットアップとマネージャーをまとめる
 *         NOTE: 各シーンはこれを通じて共通リソースにアクセスする
 *********************************************************************/
#pragma once
#include <memory>

// Forward declarations
namespace MagEngine {
	class SpriteSetup;
	class Object3dSetup;
	class ParticleSetup;
	class SkyboxSetup;
	class CloudSetup;
	class TrailEffectSetup;
	class TrailEffectManager;
}

///=============================================================================
///                         シーンコンテキスト
/// NOTE: 複数のセットアップを一つのクラスにまとめることで、
///       Initialize関数の引数を削減する
class SceneContext {
public:
	SceneContext() = default;
	~SceneContext() = default;

	// ========================================
	// Setters
	// ========================================

	/// @brief スプライトセットアップを設定
	void SetSpriteSetup(MagEngine::SpriteSetup *spriteSetup) {
		spriteSetup_ = spriteSetup;
	}

	/// @brief 3Dオブジェクトセットアップを設定
	void SetObject3dSetup(MagEngine::Object3dSetup *object3dSetup) {
		object3dSetup_ = object3dSetup;
	}

	/// @brief パーティクルセットアップを設定
	void SetParticleSetup(MagEngine::ParticleSetup *particleSetup) {
		particleSetup_ = particleSetup;
	}

	/// @brief スカイボックスセットアップを設定
	void SetSkyboxSetup(MagEngine::SkyboxSetup *skyboxSetup) {
		skyboxSetup_ = skyboxSetup;
	}

	/// @brief クラウドセットアップを設定
	void SetCloudSetup(MagEngine::CloudSetup *cloudSetup) {
		cloudSetup_ = cloudSetup;
	}

	/// @brief トレイルエフェクトセットアップを設定
	void SetTrailEffectSetup(MagEngine::TrailEffectSetup *trailEffectSetup) {
		trailEffectSetup_ = trailEffectSetup;
	}

	/// @brief トレイルエフェクトマネージャーを設定
	void SetTrailEffectManager(MagEngine::TrailEffectManager *trailEffectManager) {
		trailEffectManager_ = trailEffectManager;
	}

	// ========================================
	// Getters
	// ========================================

	/// @brief スプライトセットアップを取得
	MagEngine::SpriteSetup *GetSpriteSetup() const {
		return spriteSetup_;
	}

	/// @brief 3Dオブジェクトセットアップを取得
	MagEngine::Object3dSetup *GetObject3dSetup() const {
		return object3dSetup_;
	}

	/// @brief パーティクルセットアップを取得
	MagEngine::ParticleSetup *GetParticleSetup() const {
		return particleSetup_;
	}

	/// @brief スカイボックスセットアップを取得
	MagEngine::SkyboxSetup *GetSkyboxSetup() const {
		return skyboxSetup_;
	}

	/// @brief クラウドセットアップを取得
	MagEngine::CloudSetup *GetCloudSetup() const {
		return cloudSetup_;
	}

	/// @brief トレイルエフェクトセットアップを取得
	MagEngine::TrailEffectSetup *GetTrailEffectSetup() const {
		return trailEffectSetup_;
	}

	/// @brief トレイルエフェクトマネージャーを取得
	MagEngine::TrailEffectManager *GetTrailEffectManager() const {
		return trailEffectManager_;
	}

private:
	// ========================================
	// メンバ変数
	// ========================================
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	MagEngine::Object3dSetup *object3dSetup_ = nullptr;
	MagEngine::ParticleSetup *particleSetup_ = nullptr;
	MagEngine::SkyboxSetup *skyboxSetup_ = nullptr;
	MagEngine::CloudSetup *cloudSetup_ = nullptr;
	MagEngine::TrailEffectSetup *trailEffectSetup_ = nullptr;
	MagEngine::TrailEffectManager *trailEffectManager_ = nullptr;
};

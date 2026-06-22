/*********************************************************************
 * \file   RenderWorld.h
 * \brief  フレーム単位の描画対象を保持するクラス
 *********************************************************************/
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace MagEngine {
	class Cloud;
	class Object3d;
	class Particle;
	class Skybox;
	class Sprite;
	class TrailEffect;

	struct OpaqueRenderItem {
		Object3d *object = nullptr;
		bool visible = true;
	};

	struct SkyboxRenderItem {
		Skybox *skybox = nullptr;
		bool visible = true;
	};

	struct ParticleRenderItem {
		Particle *particle = nullptr;
		bool visible = true;
	};

	struct CloudRenderItem {
		Cloud *cloud = nullptr;
		bool visible = true;
	};

	struct TrailRenderItem {
		TrailEffect *trail = nullptr;
		bool visible = true;
	};

	struct SpriteRenderItem {
		Sprite *sprite = nullptr;
		uint32_t submissionOrder = 0;
		bool visible = true;
	};

	class RenderWorld {
	public:
		/// @brief 前フレームの非所有参照を消し、確保済み容量は再利用する
		void Clear();

		/// @brief 3D不透明描画対象を登録
		void AddOpaque(const OpaqueRenderItem &item);

		/// @brief Particle描画対象を登録
		void AddParticle(const ParticleRenderItem &item);

		/// @brief Trail描画対象を登録
		void AddTrail(const TrailRenderItem &item);

		/// @brief Sprite描画対象を登録
		void AddSprite(const SpriteRenderItem &item);

		/// @brief フレームで使用するCloudを登録
		void SetCloud(const CloudRenderItem &item);

		/// @brief 登録済みCloudを取得
		const CloudRenderItem *GetCloud() const;

		/// @brief フレームで使用するSkyboxを登録
		void SetSkybox(const SkyboxRenderItem &item);

		/// @brief 登録済みSkyboxを取得
		const SkyboxRenderItem *GetSkybox() const;

		/// @brief 3D不透明描画対象を取得
		const std::vector<OpaqueRenderItem> &GetOpaqueItems() const {
			return opaqueItems_;
		}

		/// @brief Particle描画対象を取得
		const std::vector<ParticleRenderItem> &GetParticleItems() const {
			return particleItems_;
		}

		/// @brief Trail描画対象を取得
		const std::vector<TrailRenderItem> &GetTrailItems() const {
			return trailItems_;
		}

		/// @brief Sprite描画対象を取得
		const std::vector<SpriteRenderItem> &GetSpriteItems() const {
			return spriteItems_;
		}

	private:
		// NOTE: SceneやGameObjectの所有権は持たない。参照先は描画完了までScene側が保持する。
		std::optional<SkyboxRenderItem> skyboxItem_;
		std::optional<CloudRenderItem> cloudItem_;
		std::vector<OpaqueRenderItem> opaqueItems_;
		std::vector<TrailRenderItem> trailItems_;
		std::vector<SpriteRenderItem> spriteItems_;
		std::vector<ParticleRenderItem> particleItems_;
		uint32_t nextSpriteSubmissionOrder_ = 0;
	};
}

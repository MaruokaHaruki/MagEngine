#include "RenderWorld.h"

namespace MagEngine {
	void RenderWorld::Clear() {
		skyboxItem_.reset();
		cloudItem_.reset();
		opaqueItems_.clear();
		trailItems_.clear();
		spriteItems_.clear();
		lineItems_.clear();
		particleItems_.clear();
		nextSpriteSubmissionOrder_ = 0;
		nextLineSubmissionOrder_ = 0;
	}

	void RenderWorld::AddOpaque(const OpaqueRenderItem &item) {
		opaqueItems_.push_back(item);
	}

	void RenderWorld::AddParticle(const ParticleRenderItem &item) {
		particleItems_.push_back(item);
	}

	void RenderWorld::AddTrail(const TrailRenderItem &item) {
		trailItems_.push_back(item);
	}

	void RenderWorld::AddSprite(const SpriteRenderItem &item) {
		SpriteRenderItem registeredItem = item;
		// NOTE: 透明Spriteは登録順が見た目に直結するため、RenderWorld側で決定的な順序を発行する。
		registeredItem.submissionOrder = nextSpriteSubmissionOrder_++;
		spriteItems_.push_back(registeredItem);
	}

	void RenderWorld::AddLine(const LineRenderItem &item) {
		LineRenderItem registeredItem = item;
		// NOTE: LineManagerは一括描画型のため、コマンド分解せずManager参照だけを描画フレームへ渡す。
		registeredItem.submissionOrder = nextLineSubmissionOrder_++;
		lineItems_.push_back(registeredItem);
	}

	void RenderWorld::SetCloud(const CloudRenderItem &item) {
		cloudItem_ = item;
	}

	const CloudRenderItem *RenderWorld::GetCloud() const {
		return cloudItem_ ? &(*cloudItem_) : nullptr;
	}

	void RenderWorld::SetSkybox(const SkyboxRenderItem &item) {
		skyboxItem_ = item;
	}

	const SkyboxRenderItem *RenderWorld::GetSkybox() const {
		return skyboxItem_ ? &(*skyboxItem_) : nullptr;
	}
}

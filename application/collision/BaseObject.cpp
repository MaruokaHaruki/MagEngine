#include "BaseObject.h"

///=============================================================================
///						初期化
void BaseObject::Initialize(const Vector3 &position, float radius, CollisionType type, uint16_t group) {
	// コライダーの生成
	collider_ = std::make_unique<Collider>();

	// キャラの位置とコライダーの位置を同期
	collider_->SetPosition(position); // カプセルの位置を設定
	collider_->SetRadius(radius);	  // カプセルの半径を設定

	// 衝突タイプとグループ設定
	collisionType_ = type;
	group_ = group;
	isCollisionEnabled_ = true;

	// デフォルトではすべてのグループと衝突（デフォルトは 0xFFFF）
	collisionLayerMask_ = 0xFFFF;
}

///=============================================================================
///						更新
void BaseObject::Update(const Vector3 &position) {
	collider_->SetPosition(position);
}

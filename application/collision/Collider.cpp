#include "Collider.h"
#include <cmath>

///=============================================================================
///						円同士の判定
bool Collider::Intersects(const Collider &other) const {
	// 2つの球体間の距離の二乗を計算
	Vector3 diff = position_ - other.position_;
	float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

	// 半径の合計の二乗を計算
	float radiusSum = radius_ + other.radius_;
	float radiusSumSquared = radiusSum * radiusSum;

	// 距離の二乗が半径の合計の二乗以下であれば衝突
	return distanceSquared <= radiusSumSquared;
}

///=============================================================================
///						衝突詳細情報の計算
bool Collider::GetCollisionInfo(const Collider &other, CollisionInfo &outInfo) const {
	Vector3 diff = position_ - other.position_;
	float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

	outInfo.distance = distance;

	// 最小距離を計算（球体の半径を除いたもの）
	float radiusSum = radius_ + other.radius_;
	outInfo.penetration = radiusSum - distance;

	// 衝突している場合
	if (distance < 0.0001f) {
		// 距離がほぼ0の場合は、上向きが法線
		outInfo.normal = {0.0f, 1.0f, 0.0f};
		outInfo.contactPoint = position_;
	} else {
		// 法線向きを計算
		outInfo.normal = diff * (1.0f / distance);

		// 接触点を計算（2つのコライダーの中心から半径分内側に移動）
		outInfo.contactPoint = position_ - outInfo.normal * radius_;
	}

	return outInfo.penetration > 0.0f;
}
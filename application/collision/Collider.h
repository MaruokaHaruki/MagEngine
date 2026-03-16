#pragma once
#include "MagMath.h"
using namespace MagMath;
#include <memory>

class BaseObject;

///=============================================================================
///						衝突情報構造体
/**
 * @struct CollisionInfo
 * @brief 2つのコライダー間の衝突詳細情報
 */
struct CollisionInfo {
	Vector3 contactPoint = {0.0f, 0.0f, 0.0f}; ///< 接触点
	Vector3 normal = {0.0f, 1.0f, 0.0f};	   ///< 衝突法線
	float distance = 0.0f;					   ///< 2つのコライダーの中心間距離
	float penetration = 0.0f;				   ///< 貫通量
};

///=============================================================================
///						コライダー
class Collider {
public:
	/// \brief 位置の取得
	Vector3 &GetPosition() {
		return position_;
	}

	/// \brief 位置の設定
	void SetPosition(const Vector3 &position) {
		position_ = position;
	}

	/// \brief 半径の取得
	float GetRadius() const {
		return radius_;
	}

	/// \brief 半径の設定
	void SetRadius(float radius) {
		radius_ = radius;
	}

	/**
	 * @brief 球同士の衝突判定を行う
	 * @param other 衝突判定対象となるコライダー
	 * @return 衝突している場合true、していない場合false
	 * @note 両コライダーの中心間距離と半径の合計で判定
	 */
	bool Intersects(const Collider &other) const;

	/**
	 * @brief 衝突詳細情報を計算
	 * @param other 衝突判定対象となるコライダー
	 * @param outInfo 接触点などの詳細情報を出力
	 * @return 衝突している場合true
	 */
	bool GetCollisionInfo(const Collider &other, CollisionInfo &outInfo) const;

private:
	// 位置
	Vector3 position_ = {0.0f, 0.0f, 0.0f};

	// 半径（球体コライダーを想定）
	float radius_ = 1.0f;

	// 色
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
};

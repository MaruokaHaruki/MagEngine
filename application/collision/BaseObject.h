#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Collider.h"
#include <memory>
#include <unordered_set>

///=============================================================================
///						基底オブジェクト
/**
 * @brief 衝突判定システムに参加するすべてのゲームオブジェクトの基底クラス
 *
 * 責務：
 * - コライダーの管理と衝突判定システムとの連携
 * - 衝突イベント（開始・継続・終了）のライフサイクル管理
 * - 衝突中のオブジェクト集合の保持と管理
 *
 * 継承先は純粋仮想関数OnCollisionEnter/Stay/Exitを実装し、
 * 衝突イベントに対する固有の処理を定義する必要があります。
 */
class BaseObject {
public:
	/// \brief 仮想デストラクタ
	virtual ~BaseObject() = default;

	/// @brief 初期化
	/// @param position 初期位置（参照で渡されるが内容は変更されない）
	/// @param radius コライダーの半径（衝突判定用）
	void Initialize(const Vector3 &position, float radius);

	/// @brief 更新
	/// @param position 最新の位置情報（毎フレーム更新される）
	void Update(const Vector3 &position);

	/// \brief 衝突開始時の処理
	virtual void OnCollisionEnter(BaseObject *other) = 0;

	/// \brief 衝突継続時の処理
	virtual void OnCollisionStay(BaseObject *other) = 0;

	/// \brief 衝突終了時の処理
	virtual void OnCollisionExit(BaseObject *other) = 0;

	/// \brief コライダーの取得
	std::shared_ptr<Collider> GetCollider() const {
		return collider_;
	}

	/// \brief コライダーの設定
	void SetCollider(std::shared_ptr<Collider> collider) {
		collider_ = collider;
	}

	/// \brief 衝突中のオブジェクトのセットの取得
	std::unordered_set<BaseObject *> &GetCollidingObjects() {
		return collidingObjects_;
	}

protected:
	// コライダー
	std::shared_ptr<Collider> collider_;

	// 衝突中のオブジェクトのセット
	std::unordered_set<BaseObject *> collidingObjects_;
};

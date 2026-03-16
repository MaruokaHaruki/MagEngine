#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Collider.h"
#include <cstdint>
#include <memory>
#include <unordered_set>

///=============================================================================
///						衝突判定タイプ
/**
 * @enum CollisionType
 * @brief オブジェクトの衝突判定タイプ
 *
 * - DYNAMIC: 通常の物理的衝突判定が行われるオブジェクト
 * - STATIC: 他のオブジェクトとの衝突は判定されるが、自身は動かないオブジェクト
 * - TRIGGER: 衝突イベントは発火するが、物理的な衝突は行わないオブジェクト
 */
enum class CollisionType {
	DYNAMIC = 0, ///< 動的オブジェクト
	STATIC = 1,	 ///< 静的オブジェクト
	TRIGGER = 2	 ///< トリガーオブジェクト
};

///=============================================================================
///						基底オブジェクト
/**
 * @brief 衝突判定システムに参加するすべてのゲームオブジェクトの基底クラス
 *
 * 責務：
 * - コライダーの管理と衝突判定システムとの連携
 * - 衝突イベント（開始・継続・終了）のライフサイクル管理
 * - 衝突中のオブジェクト集合の保持と管理
 * - 衝突タイプとグループ/レイヤー管理
 *
 * 継承先は純粋仮想関数OnCollisionEnter/Stay/Exitを実装し、
 * 衝突イベントに対する固有の処理を定義する必要があります。
 */
class BaseObject {
public:
	/// \brief 仮想デストラクタ
	virtual ~BaseObject() = default;

	/// @brief 初期化（拡張版）
	/// @param position 初期位置
	/// @param radius コライダーの半径
	/// @param type 衝突判定タイプ（デフォルト: DYNAMIC）
	/// @param group グループID（デフォルト: 0）
	void Initialize(const Vector3 &position, float radius, CollisionType type = CollisionType::DYNAMIC, uint16_t group = 0);

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

	//========================================
	// 有効/無効制御
	//========================================

	/// @brief 衝突判定が有効かどうか
	bool IsCollisionEnabled() const {
		return isCollisionEnabled_;
	}

	/// @brief 衝突判定を有効/無効に設定
	void SetCollisionEnabled(bool enabled) {
		isCollisionEnabled_ = enabled;
	}

	//========================================
	// 衝突タイプ制御
	//========================================

	/// @brief 衝突判定タイプを取得
	CollisionType GetCollisionType() const {
		return collisionType_;
	}

	/// @brief 衝突判定タイプを設定
	void SetCollisionType(CollisionType type) {
		collisionType_ = type;
	}

	//========================================
	// グループ/レイヤー制御
	//========================================

	/// @brief グループIDを取得
	uint16_t GetGroup() const {
		return group_;
	}

	/// @brief グループIDを設定
	void SetGroup(uint16_t group) {
		group_ = group;
	}

	/// @brief 衝突対象レイヤーマスク（誰と衝突するか）を取得
	uint16_t GetCollisionLayerMask() const {
		return collisionLayerMask_;
	}

	/// @brief 衝突対象レイヤーマスクを設定
	/// @param mask ビットマスク（ビットiがマスクされたグループとの衝突判定）
	void SetCollisionLayerMask(uint16_t mask) {
		collisionLayerMask_ = mask;
	}

	/// @brief 指定グループと衝突するかチェック
	bool CanCollideWith(uint16_t otherGroup) const {
		// 衝突判定が無効の場合は衝突しない
		if (!isCollisionEnabled_)
			return false;
		// レイヤーマスクでフィルタリング
		return (collisionLayerMask_ & (1 << otherGroup)) != 0;
	}

protected:
	// コライダー
	std::shared_ptr<Collider> collider_;

	// 衝突中のオブジェクトのセット
	std::unordered_set<BaseObject *> collidingObjects_;

	// 衝突判定の有効/無効
	bool isCollisionEnabled_ = true;

	// 衝突判定タイプ
	CollisionType collisionType_ = CollisionType::DYNAMIC;

	// グループID
	uint16_t group_ = 0;

	// 衝突対象のレイヤーマスク（ビット単位）デフォルトはすべてのグループと衝突
	uint16_t collisionLayerMask_ = 0xFFFF;
};

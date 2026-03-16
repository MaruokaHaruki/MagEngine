/*********************************************************************
 * \file   CollisionManager.h
 * \brief  軽量で高効率な当たり判定管理システム
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   エンジン内部使用に最適化済み（ver2.0に改良）
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "BaseObject.h"
#include "Collider.h"
#include <bitset>
#include <memory>
#include <unordered_map>
#include <vector>

//========================================
// コリジョンシステム定数
namespace CollisionConstants {
	// グリッドセルのデフォルトサイズ（単位：ゲーム座標）
	constexpr float kDefaultCellSize = 32.0f;
	// グリッド内の最大オブジェクト数
	constexpr int kDefaultMaxObjects = 1024;
}

//========================================
// グリッド座標構造体（ハッシュ問題を解決）
struct GridCoord {
	int x, y, z;

	GridCoord() : x(0), y(0), z(0) {
	}
	GridCoord(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {
	}

	bool operator==(const GridCoord &other) const {
		return x == other.x && y == other.y && z == other.z;
	}
};

//========================================
// ハッシュ関数（GridCoord専用）
struct GridCoordHash {
	std::size_t operator()(const GridCoord &coord) const {
		// Cantor pairing をベースにした3D座標ハッシュ
		static constexpr int PRIME1 = 73856093;
		static constexpr int PRIME2 = 19349663;
		static constexpr int PRIME3 = 83492791;
		return (coord.x * PRIME1) ^ (coord.y * PRIME2) ^ (coord.z * PRIME3);
	}
};

//========================================
// 最適化されたグリッドセル
struct GridCell {
	std::vector<BaseObject *> objects;

	void Clear() {
		objects.clear();
	}
	bool IsEmpty() const {
		return objects.empty();
	}
	size_t Size() const {
		return objects.size();
	}
	void Reserve(size_t capacity) {
		objects.reserve(capacity);
	}
};

//========================================
// コリジョンペア（軽量化）
struct CollisionPair {
	BaseObject *objA;
	BaseObject *objB;

	CollisionPair(BaseObject *a, BaseObject *b)
		: objA(a < b ? a : b), objB(a < b ? b : a) {
	}

	bool operator==(const CollisionPair &other) const {
		return objA == other.objA && objB == other.objB;
	}
};

//========================================
// ハッシュ関数（CollisionPair専用）
struct CollisionPairHash {
	std::size_t operator()(const CollisionPair &pair) const {
		return std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(pair.objA)) ^
			   (std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(pair.objB)) << 1);
	}
};

///=============================================================================
///						軽量コリジョンマネージャー（改良版）
class CollisionManager {
public:
	/// \brief 初期化
	void Initialize(float cellSize = CollisionConstants::kDefaultCellSize, int maxObjects = CollisionConstants::kDefaultMaxObjects);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// @brief ImGuiの描画
	void DrawImGui();

	/// \brief リセット
	void Reset();

	/// \brief オブジェクト登録
	void RegisterObject(BaseObject *obj);

	/// \brief オブジェクト登録解除
	void UnregisterObject(BaseObject *obj);

	/// \brief 全ての当たり判定をチェック
	void CheckAllCollisions();

	/// \brief セルサイズの設定
	void SetCellSize(float size) {
		cellSize_ = size;
		invCellSize_ = 1.0f / cellSize_;
	}

	/// \brief グリッド情報取得（デバッグ用）
	size_t GetActiveGridCount() const {
		return grid_.size();
	}
	size_t GetTotalObjectCount() const {
		return activeObjects_.size();
	}
	size_t GetCollisionChecksThisFrame() const {
		return collisionChecksThisFrame_;
	}

	//========================================
	// グループ/レイヤー管理
	//========================================

	/// @brief グループ間の衝突設定（グループAがグループBと衝突するか）
	/// @param groupA グループA
	/// @param groupB グループB
	/// @param canCollide 衝突するかどうか
	void SetGroupCollision(uint16_t groupA, uint16_t groupB, bool canCollide);

	/// @brief すべての衝突設定をリセット（すべて衝突するようにする）
	void ResetGroupCollisions();

	/// @brief グループAがグループBと衝突するかを問い合わせ
	bool CanGroupsCollide(uint16_t groupA, uint16_t groupB) const;

private:
	/// \brief グリッド座標計算（改良版）
	GridCoord CalculateGridCoord(const Vector3 &position) const;

	/// \brief オブジェクトをグリッドに配置
	void AssignObjectsToGrid();

	/// \brief セル内衝突判定
	void CheckCollisionsInCell(const GridCell &cell);

	/// \brief 隣接セル間衝突判定（改良版）
	void CheckAdjacentCellCollisions(const GridCoord &coord, const GridCell &cell);

	/// \brief 衝突処理実行
	void ProcessCollision(BaseObject *objA, BaseObject *objB, bool isColliding);

	/// \brief 高速衝突判定（AABB事前チェック付き）
	bool FastIntersects(BaseObject *objA, BaseObject *objB) const;

	/// \brief デバッグ描画（最適化版）
	void DrawDebugColliders();

	/// \brief グループとレイヤーマスクの衝突判定
	bool CanCollideByGroupAndMask(BaseObject *objA, BaseObject *objB) const;

private:
	//========================================
	// グリッドシステム（改良版）
	std::unordered_map<GridCoord, GridCell, GridCoordHash> grid_;
	float cellSize_;
	float invCellSize_; // 1/cellSize_（除算回避）

	//========================================
	// オブジェクト管理
	std::vector<BaseObject *> activeObjects_;
	std::vector<BaseObject *> objectPool_; // 再利用プール

	//========================================
	// 衝突状態管理（軽量化）
	std::unordered_map<CollisionPair, bool, CollisionPairHash> collisionStates_;

	//========================================
	// グループ間の衝突フラグマトリクス（16x16 = 256フラグ）
	// groupCollisionMatrix[i][j] = グループiがグループjと衝突するかどうか
	bool groupCollisionMatrix_[16][16];

	//========================================
	// デバッグ情報
	bool enableDebugDraw_;
	bool enableGroupFilter_;	///< グループフィルタリングの有効/無効
	uint16_t debugGroupFilter_; ///< デバッグ表示用のグループフィルタ
	size_t collisionChecksThisFrame_;
};

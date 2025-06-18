/*********************************************************************
 * \file   CollisionManager.h
 * \brief  軽量で高効率な当たり判定管理システム
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   エンジン内部使用に最適化済み
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Collider.h"
#include <bitset>
#include <memory>
#include <unordered_map>
#include <vector>

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
// ハッシュ関数（最適化済み）
struct CollisionPairHash {
	std::size_t operator()(const CollisionPair &pair) const {
		return std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(pair.objA)) ^
			   (std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(pair.objB)) << 1);
	}
};

///=============================================================================
///						軽量コリジョンマネージャー
class CollisionManager {
public:
	/// \brief 初期化
	void Initialize(float cellSize = 32.0f, int maxObjects = 1024);

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
	}

	/// \brief グリッド情報取得（デバッグ用）
	size_t GetActiveGridCount() const {
		return grid_.size();
	}
	size_t GetTotalObjectCount() const {
		return activeObjects_.size();
	}

private:
	/// \brief グリッドインデックス計算（最適化済み）
	int CalculateGridIndex(const Vector3 &position) const;

	/// \brief オブジェクトをグリッドに配置
	void AssignObjectsToGrid();

	/// \brief セル内衝突判定
	void CheckCollisionsInCell(const GridCell &cell);

	/// \brief 隣接セル間衝突判定
	void CheckCollisionsBetweenCells(const GridCell &cellA, const GridCell &cellB);

	/// \brief 衝突処理実行
	void ProcessCollision(BaseObject *objA, BaseObject *objB, bool isColliding);

private:
	//========================================
	// グリッドシステム
	std::unordered_map<int, GridCell> grid_;
	float cellSize_;

	//========================================
	// オブジェクト管理
	std::vector<BaseObject *> activeObjects_;

	//========================================
	// 衝突状態管理（軽量化）
	std::unordered_map<CollisionPair, bool, CollisionPairHash> collisionStates_;

	//========================================
	// デバッグ情報
	bool enableDebugDraw_;
	size_t collisionChecksThisFrame_;

	//========================================
	// 最適化パラメータ
	static constexpr int GRID_HASH_PRIME1 = 73856093;
	static constexpr int GRID_HASH_PRIME2 = 19349663;
	static constexpr int GRID_HASH_PRIME3 = 83492791;
};

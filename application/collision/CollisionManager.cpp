#include "CollisionManager.h"
#include "BaseObject.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>
using namespace MagEngine;

///=============================================================================
///						初期化
void CollisionManager::Initialize(float cellSize, int maxObjects) {
	cellSize_ = cellSize;
	invCellSize_ = 1.0f / cellSize; // 除算回避用
	enableDebugDraw_ = false;
	collisionChecksThisFrame_ = 0;
	skipDistantCells_ = true;
	maxCellDistance_ = 2; // 2セルまでの隣接をチェック

	// メモリ予約（パフォーマンス最適化）
	activeObjects_.reserve(maxObjects);
	objectPool_.reserve(maxObjects);
	grid_.reserve(maxObjects / 8);			  // より小さい初期容量
	collisionStates_.reserve(maxObjects * 2); // 衝突ペア予約
}

///=============================================================================
///						更新処理
void CollisionManager::Update() {
	collisionChecksThisFrame_ = 0;

	//========================================
	// グリッドクリアと再配置
	for (auto &pair : grid_) {
		pair.second.Clear();
	}

	AssignObjectsToGrid();
	CheckAllCollisions();

	//========================================
	// デバッグ描画
	if (enableDebugDraw_) {
		for (const auto &obj : activeObjects_) {
			if (obj && obj->GetCollider()) {
				Vector3 position = obj->GetCollider()->GetPosition();
				float radius = obj->GetCollider()->GetRadius();
				LineManager::GetInstance()->DrawSphere(position, radius,
													   Vector4{1.0f, 0.0f, 0.0f, 1.0f});
			}
		}
	}
}

///=============================================================================
///						描画
void CollisionManager::Draw() {
	// 必要に応じて追加の描画処理
}

///=============================================================================
///						ImGuiの描画
void CollisionManager::DrawImGui() {
	ImGui::Begin("CollisionManager");
	ImGui::Text("Active Objects: %zu", activeObjects_.size());
	ImGui::Text("Active Grids: %zu", grid_.size());
	ImGui::Text("Collision Checks: %zu", collisionChecksThisFrame_);
	ImGui::Text("Cell Size: %.1f", cellSize_);

	ImGui::Separator();
	ImGui::Checkbox("Debug Draw", &enableDebugDraw_);

	if (ImGui::SliderFloat("Cell Size", &cellSize_, 16.0f, 128.0f)) {
		// セルサイズ変更時にグリッドを再構築
		for (auto &pair : grid_) {
			pair.second.Clear();
		}
	}

	ImGui::End();
}

///=============================================================================
///						リセット
void CollisionManager::Reset() {
	activeObjects_.clear();
	for (auto &pair : grid_) {
		pair.second.Clear();
	}
	collisionStates_.clear();
}

///=============================================================================
///						オブジェクト登録
void CollisionManager::RegisterObject(BaseObject *obj) {
	if (obj && std::find(activeObjects_.begin(), activeObjects_.end(), obj) == activeObjects_.end()) {
		activeObjects_.push_back(obj);
	}
}

///=============================================================================
///						オブジェクト登録解除
void CollisionManager::UnregisterObject(BaseObject *obj) {
	auto it = std::find(activeObjects_.begin(), activeObjects_.end(), obj);
	if (it != activeObjects_.end()) {
		activeObjects_.erase(it);

		// 関連する衝突状態も削除
		auto stateIt = collisionStates_.begin();
		while (stateIt != collisionStates_.end()) {
			if (stateIt->first.objA == obj || stateIt->first.objB == obj) {
				stateIt = collisionStates_.erase(stateIt);
			} else {
				++stateIt;
			}
		}
	}
}

///=============================================================================
///						グリッドインデックス計算
int CollisionManager::CalculateGridIndex(const Vector3 &position) const {
	// 除算を乗算に変更（高速化）
	int x = static_cast<int>(std::floor(position.x * invCellSize_));
	int y = static_cast<int>(std::floor(position.y * invCellSize_));
	int z = static_cast<int>(std::floor(position.z * invCellSize_));

	return (x * GRID_HASH_PRIME1) ^ (y * GRID_HASH_PRIME2) ^ (z * GRID_HASH_PRIME3);
}

///=============================================================================
///						隣接セル取得
std::vector<int> CollisionManager::GetAdjacentCells(int cellIndex) const {
	std::vector<int> adjacentCells;
	adjacentCells.reserve(27); // 3x3x3の最大数

	// 元のセル座標を逆算
	int x = cellIndex / (GRID_HASH_PRIME1);
	int y = (cellIndex % GRID_HASH_PRIME1) / GRID_HASH_PRIME2;
	int z = cellIndex % GRID_HASH_PRIME2;

	// 隣接セルを生成（最適化：1セル範囲のみ）
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dz = -1; dz <= 1; ++dz) {
				if (dx == 0 && dy == 0 && dz == 0)
					continue; // 自分自身をスキップ

				int adjX = x + dx;
				int adjY = y + dy;
				int adjZ = z + dz;
				int adjIndex = (adjX * GRID_HASH_PRIME1) ^ (adjY * GRID_HASH_PRIME2) ^ (adjZ * GRID_HASH_PRIME3);

				if (grid_.find(adjIndex) != grid_.end()) {
					adjacentCells.push_back(adjIndex);
				}
			}
		}
	}

	return adjacentCells;
}

///=============================================================================
///						高速衝突判定
bool CollisionManager::FastIntersects(BaseObject *objA, BaseObject *objB) const {
	auto colliderA = objA->GetCollider();
	auto colliderB = objB->GetCollider();

	if (!colliderA || !colliderB)
		return false;

	// 早期リターン：ざっくりした距離チェック
	Vector3 diff = colliderA->GetPosition() - colliderB->GetPosition();
	float radiusSum = colliderA->GetRadius() + colliderB->GetRadius();

	// まずX軸だけでチェック（最も高速）
	if (std::abs(diff.x) > radiusSum)
		return false;
	if (std::abs(diff.y) > radiusSum)
		return false;
	if (std::abs(diff.z) > radiusSum)
		return false;

	// 詳細な距離計算
	float distanceSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	return distanceSquared <= (radiusSum * radiusSum);
}

///=============================================================================
///						セル内の当たり判定をチェック
void CollisionManager::CheckCollisionsInCell(const GridCell &cell) {
	if (cell.Size() < 2)
		return;

	// 最適化：大量オブジェクトのセルは分割処理
	if (cell.Size() > 20) {
		// サブグリッド処理（必要に応じて実装）
		return;
	}

	for (size_t i = 0; i < cell.Size(); ++i) {
		for (size_t j = i + 1; j < cell.Size(); ++j) {
			BaseObject *objA = cell.objects[i];
			BaseObject *objB = cell.objects[j];

			if (objA && objB) {
				bool isColliding = FastIntersects(objA, objB);
				ProcessCollision(objA, objB, isColliding);
				++collisionChecksThisFrame_;
			}
		}
	}
}

///=============================================================================
///						セル間の当たり判定をチェック
void CollisionManager::CheckCollisionsBetweenCells(const GridCell &cellA, const GridCell &cellB) {
	// 最適化：空セルや大量セルをスキップ
	if (cellA.IsEmpty() || cellB.IsEmpty())
		return;
	if (cellA.Size() > 15 || cellB.Size() > 15)
		return; // 重いセルをスキップ

	for (BaseObject *objA : cellA.objects) {
		for (BaseObject *objB : cellB.objects) {
			if (objA && objB) {
				bool isColliding = FastIntersects(objA, objB);
				ProcessCollision(objA, objB, isColliding);
				++collisionChecksThisFrame_;
			}
		}
	}
}

///=============================================================================
///						すべての当たり判定をチェック
void CollisionManager::CheckAllCollisions() {
	// 最適化：アクティブセルのみ処理
	std::vector<int> activeCellIndices;
	activeCellIndices.reserve(grid_.size());

	for (const auto &pair : grid_) {
		if (!pair.second.IsEmpty()) {
			activeCellIndices.push_back(pair.first);
			CheckCollisionsInCell(pair.second);
		}
	}

	// セル間衝突判定（隣接セルのみ、最適化済み）
	for (int cellIndex : activeCellIndices) {
		auto adjacentCells = GetAdjacentCells(cellIndex);
		const GridCell &currentCell = grid_[cellIndex];

		for (int adjIndex : adjacentCells) {
			if (adjIndex > cellIndex) { // 重複チェック回避
				CheckCollisionsBetweenCells(currentCell, grid_[adjIndex]);
			}
		}
	}
}

///=============================================================================
///						オブジェクトをグリッドに配置
void CollisionManager::AssignObjectsToGrid() {
	for (BaseObject *obj : activeObjects_) {
		if (obj && obj->GetCollider()) {
			int index = CalculateGridIndex(obj->GetCollider()->GetPosition());
			grid_[index].objects.push_back(obj);
		}
	}
}

///=============================================================================
///						衝突処理実行
void CollisionManager::ProcessCollision(BaseObject *objA, BaseObject *objB, bool isColliding) {
	CollisionPair pair(objA, objB);
	auto it = collisionStates_.find(pair);
	bool wasColliding = (it != collisionStates_.end()) ? it->second : false;

	if (isColliding && !wasColliding) {
		// 衝突開始
		objA->OnCollisionEnter(objB);
		objB->OnCollisionEnter(objA);
		collisionStates_[pair] = true;
	} else if (isColliding && wasColliding) {
		// 衝突継続
		objA->OnCollisionStay(objB);
		objB->OnCollisionStay(objA);
	} else if (!isColliding && wasColliding) {
		// 衝突終了
		objA->OnCollisionExit(objB);
		objB->OnCollisionExit(objA);
		collisionStates_.erase(pair);
	}
}

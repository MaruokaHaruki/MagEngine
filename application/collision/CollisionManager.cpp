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
		DrawDebugColliders();
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
		SetCellSize(cellSize_);
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

		// 衝突イベントを全て終了させる
		auto collidingCopy = obj->GetCollidingObjects();
		for (BaseObject *collidingObj : collidingCopy) {
			ProcessCollision(obj, collidingObj, false);
		}

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
///						グリッド座標計算（改良版）
///						ハッシュではなく直接座標を使用
GridCoord CollisionManager::CalculateGridCoord(const Vector3 &position) const {
	// NOTE: 座標を直接計算。復元の問題なし
	int x = static_cast<int>(std::floor(position.x * invCellSize_));
	int y = static_cast<int>(std::floor(position.y * invCellSize_));
	int z = static_cast<int>(std::floor(position.z * invCellSize_));

	return GridCoord(x, y, z);
}

///=============================================================================
///						セル内の当たり判定をチェック
void CollisionManager::CheckCollisionsInCell(const GridCell &cell) {
	if (cell.Size() < 2)
		return;

	// セル内のオブジェクト同士をチェック
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
///						隣接セル間衝突判定（改良版）
///						座標から隣接セルを直接計算
void CollisionManager::CheckAdjacentCellCollisions(const GridCoord &coord, const GridCell &cell) {
	if (cell.IsEmpty())
		return;

	// 隣接セルを逐一生成（3x3x3グリッド、自分除外）
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dz = -1; dz <= 1; ++dz) {
				if (dx == 0 && dy == 0 && dz == 0)
					continue; // 自分自身をスキップ

				GridCoord adjCoord(coord.x + dx, coord.y + dy, coord.z + dz);
				auto adjIt = grid_.find(adjCoord);

				if (adjIt != grid_.end() && !adjIt->second.IsEmpty()) {
					const GridCell &adjCell = adjIt->second;

					// 両セルのオブジェクト同士をチェック
					for (BaseObject *objA : cell.objects) {
						for (BaseObject *objB : adjCell.objects) {
							if (objA && objB) {
								bool isColliding = FastIntersects(objA, objB);
								ProcessCollision(objA, objB, isColliding);
								++collisionChecksThisFrame_;
							}
						}
					}
				}
			}
		}
	}
}

///=============================================================================
///						すべての当たり判定をチェック（改良版）
void CollisionManager::CheckAllCollisions() {
	// セル内衝突判定
	for (auto &pair : grid_) {
		CheckCollisionsInCell(pair.second);
	}

	// 隣接セル間衝突判定（座標ベース）
	// NOTE: 重複チェック回避のため、各セルは一度だけ処理
	std::vector<GridCoord> processedCoords;
	processedCoords.reserve(grid_.size());

	for (const auto &pair : grid_) {
		const GridCoord &coord = pair.first;
		const GridCell &cell = pair.second;

		CheckAdjacentCellCollisions(coord, cell);
	}
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
///						オブジェクトをグリッドに配置
void CollisionManager::AssignObjectsToGrid() {
	for (BaseObject *obj : activeObjects_) {
		if (obj && obj->GetCollider()) {
			GridCoord coord = CalculateGridCoord(obj->GetCollider()->GetPosition());
			grid_[coord].objects.push_back(obj);
		}
	}
}

///=============================================================================
///						衝突処理実行（改良版）
void CollisionManager::ProcessCollision(BaseObject *objA, BaseObject *objB, bool isColliding) {
	CollisionPair pair(objA, objB);
	auto it = collisionStates_.find(pair);
	bool wasColliding = (it != collisionStates_.end()) ? it->second : false;

	if (isColliding && !wasColliding) {
		// 衝突開始
		objA->OnCollisionEnter(objB);
		objB->OnCollisionEnter(objA);

		// collidingObjects_セットに追加
		objA->GetCollidingObjects().insert(objB);
		objB->GetCollidingObjects().insert(objA);

		collisionStates_[pair] = true;
	} else if (isColliding && wasColliding) {
		// 衝突継続
		objA->OnCollisionStay(objB);
		objB->OnCollisionStay(objA);
	} else if (!isColliding && wasColliding) {
		// 衝突終了
		objA->OnCollisionExit(objB);
		objB->OnCollisionExit(objA);

		// collidingObjects_セットから削除
		objA->GetCollidingObjects().erase(objB);
		objB->GetCollidingObjects().erase(objA);

		collisionStates_.erase(pair);
	}
}

///=============================================================================
///						デバッグ描画（最適化版）
void CollisionManager::DrawDebugColliders() {
	for (const auto &obj : activeObjects_) {
		if (obj && obj->GetCollider()) {
			Vector3 position = obj->GetCollider()->GetPosition();
			float radius = obj->GetCollider()->GetRadius();

			// 衝突中のオブジェクトは赤、そうでなければ白
			bool isColliding = !obj->GetCollidingObjects().empty();
			Vector4 color = isColliding ? Vector4{1.0f, 0.0f, 0.0f, 1.0f} : Vector4{1.0f, 1.0f, 1.0f, 1.0f};

			LineManager::GetInstance()->DrawSphere(position, radius, color);
		}
	}
}

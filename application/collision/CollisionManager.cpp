#include "CollisionManager.h"
#include "BaseObject.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>

///=============================================================================
///						初期化
void CollisionManager::Initialize(float cellSize, int maxObjects) {
	cellSize_ = cellSize;
	enableDebugDraw_ = false;
	collisionChecksThisFrame_ = 0;

	// メモリ予約（パフォーマンス最適化）
	activeObjects_.reserve(maxObjects);
	grid_.reserve(maxObjects / 4); // 適度な初期容量
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
	int x = static_cast<int>(std::floor(position.x / cellSize_));
	int y = static_cast<int>(std::floor(position.y / cellSize_));
	int z = static_cast<int>(std::floor(position.z / cellSize_));

	return (x * GRID_HASH_PRIME1) ^ (y * GRID_HASH_PRIME2) ^ (z * GRID_HASH_PRIME3);
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
///						セル内の当たり判定をチェック
void CollisionManager::CheckCollisionsInCell(const GridCell &cell) {
	if (cell.Size() < 2)
		return;

	for (size_t i = 0; i < cell.Size(); ++i) {
		for (size_t j = i + 1; j < cell.Size(); ++j) {
			BaseObject *objA = cell.objects[i];
			BaseObject *objB = cell.objects[j];

			if (objA && objB && objA->GetCollider() && objB->GetCollider()) {
				bool isColliding = objA->GetCollider()->Intersects(*objB->GetCollider());
				ProcessCollision(objA, objB, isColliding);
				++collisionChecksThisFrame_;
			}
		}
	}
}

///=============================================================================
///						セル間の当たり判定をチェック
void CollisionManager::CheckCollisionsBetweenCells(const GridCell &cellA, const GridCell &cellB) {
	for (BaseObject *objA : cellA.objects) {
		for (BaseObject *objB : cellB.objects) {
			if (objA && objB && objA->GetCollider() && objB->GetCollider()) {
				bool isColliding = objA->GetCollider()->Intersects(*objB->GetCollider());
				ProcessCollision(objA, objB, isColliding);
				++collisionChecksThisFrame_;
			}
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

///=============================================================================
///						すべての当たり判定をチェック
void CollisionManager::CheckAllCollisions() {
	// セル内衝突判定
	for (const auto &pair : grid_) {
		if (!pair.second.IsEmpty()) {
			CheckCollisionsInCell(pair.second);
		}
	}

	// セル間衝突判定（隣接セルのみ）
	auto it1 = grid_.begin();
	while (it1 != grid_.end()) {
		auto it2 = it1;
		++it2;
		while (it2 != grid_.end()) {
			CheckCollisionsBetweenCells(it1->second, it2->second);
			++it2;
		}
		++it1;
	}
}

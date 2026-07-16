#include "CollisionManager.h"
#include "BaseObject.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>
using namespace MagEngine;

///=============================================================================
///						初期化
void CollisionManager::Initialize(MagEngine::LineManager &lineManager, float cellSize, int maxObjects) {
	lineManager_ = &lineManager;
	cellSize_ = cellSize;
	invCellSize_ = 1.0f / cellSize; // 除算回避用
	enableDebugDraw_ = false;
	enableGroupFilter_ = false;
	debugGroupFilter_ = 0;
	collisionChecksThisFrame_ = 0;

	// COMMENT: メモリ予約（パフォーマンス最適化）アロケーション回数を削減
	activeObjects_.reserve(maxObjects);
	objectPool_.reserve(maxObjects);
	grid_.reserve(maxObjects / 8);			  // より小さい初期容量
	collisionStates_.reserve(maxObjects * 2); // 衝突ペア予約
	currentCollisionPairs_.reserve(maxObjects * 2);

	// グループ衝突マトリクスを初期化（デフォルトはすべて衝突可能）
	ResetGroupCollisions();
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
	// 破棄済みオブジェクトを含む衝突状態の除去
	// 前フレームの衝突状態には、今フレームの登録処理より前に破棄された
	// オブジェクトの生ポインタが残る場合がある。破棄済みポインタへ
	// OnCollisionExit()を呼ぶとアクセス違反になるため、コールバックを
	// 発行せずに状態だけを破棄する。
	for (auto it = collisionStates_.begin(); it != collisionStates_.end();) {
		const CollisionPair pair = it->first;
		const bool isObjAActive = std::find(activeObjects_.begin(), activeObjects_.end(), pair.objA) != activeObjects_.end();
		const bool isObjBActive = std::find(activeObjects_.begin(), activeObjects_.end(), pair.objB) != activeObjects_.end();

		if (!isObjAActive || !isObjBActive) {
			// 生存している側だけは、相手への参照を取り除く。
			if (isObjAActive) {
				pair.objA->GetCollidingObjects().erase(pair.objB);
			}
			if (isObjBActive) {
				pair.objB->GetCollidingObjects().erase(pair.objA);
			}

			it = collisionStates_.erase(it);
		} else {
			++it;
		}
	}

	// 処理内容：前フレームから継続しているが、今フレームに検出されなかった衝突を終了する
	// 理由：Enter、Stay、Exitを正しく判定するため
	for (auto it = collisionStates_.begin(); it != collisionStates_.end();) {
		const CollisionPair pair = it->first;
		++it;
		if (currentCollisionPairs_.find(pair) == currentCollisionPairs_.end()) {
			ProcessCollision(pair.objA, pair.objB, false);
		}
	}

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

	//========================================
	// システム情報
	ImGui::Text("Active Objects: %zu", activeObjects_.size());
	ImGui::Text("Active Grids: %zu", grid_.size());
	ImGui::Text("Collision Checks: %zu", collisionChecksThisFrame_);
	ImGui::Text("Cell Size: %.1f", cellSize_);

	ImGui::Separator();

	//========================================
	// 有効/無効と主要設定
	ImGui::Checkbox("Debug Draw", &enableDebugDraw_);
	ImGui::Checkbox("Group Filter", &enableGroupFilter_);

	if (enableGroupFilter_) {
		int filterGroupInt = static_cast<int>(debugGroupFilter_);
		if (ImGui::SliderInt("Filter Group", &filterGroupInt, 0, 15)) {
			debugGroupFilter_ = static_cast<uint16_t>(filterGroupInt);
		}
	}

	if (ImGui::SliderFloat("Cell Size", &cellSize_, 16.0f, 128.0f)) {
		SetCellSize(cellSize_);
		// セルサイズ変更時にグリッドを再構築
		for (auto &pair : grid_) {
			pair.second.Clear();
		}
	}

	ImGui::Separator();

	//========================================
	// グループ衝突マトリクス表示
	if (ImGui::CollapsingHeader("Group Collision Matrix")) {
		// リセットボタン
		if (ImGui::Button("Reset All Collisions")) {
			ResetGroupCollisions();
		}

		// グループ衝突マトリクスを表示（下三角のみ）
		ImGui::Text("Groups can collide:");
		for (int i = 0; i < 16; ++i) {
			for (int j = i; j < 16; ++j) {
				std::string label = "G" + std::to_string(i) + "-G" + std::to_string(j);
				ImGui::Checkbox(label.c_str(), &groupCollisionMatrix_[i][j]);
				if (j < 15)
					ImGui::SameLine();
			}
		}
	}

	//========================================
	// オブジェクト一覧表示
	if (ImGui::CollapsingHeader("Active Objects")) {
		for (size_t idx = 0; idx < activeObjects_.size(); ++idx) {
			BaseObject *obj = activeObjects_[idx];
			if (!obj)
				continue;

			ImGui::PushID(static_cast<int>(idx));

			auto collider = obj->GetCollider();
			if (collider) {
				Vector3 pos = collider->GetPosition();
				float radius = collider->GetRadius();
				bool enabled = obj->IsCollisionEnabled();
				int type = static_cast<int>(obj->GetCollisionType());
				uint16_t group = obj->GetGroup();
				size_t collidingCount = obj->GetCollidingObjects().size();

				ImGui::Text("Obj[%zu]: Pos(%.1f, %.1f, %.1f) Radius=%.1f", idx, pos.x, pos.y, pos.z, radius);
				ImGui::Text("  Enabled: %s | Type: %s | Group: %u | Colliding: %zu",
							enabled ? "Yes" : "No",
							type == 0 ? "DYNAMIC" : (type == 1 ? "STATIC" : "TRIGGER"),
							group, collidingCount);

				// 有効/無効の切り替え
				if (ImGui::Checkbox(("Enable##" + std::to_string(idx)).c_str(), &enabled)) {
					obj->SetCollisionEnabled(enabled);
				}
			}

			ImGui::PopID();
			ImGui::Separator();
		}
	}

	ImGui::End();
}

///=============================================================================
///						フレーム開始
void CollisionManager::BeginFrame() {
	activeObjects_.clear();
	for (auto &pair : grid_) {
		pair.second.Clear();
	}
	currentCollisionPairs_.clear();
}

///=============================================================================
///						完全初期化
void CollisionManager::ClearAll() {
	// 処理内容：管理中の衝突対象と衝突状態をすべて破棄する
	// 理由：シーン終了時に前シーンのオブジェクト参照を残さないため
	activeObjects_.clear();
	for (auto &pair : grid_) {
		pair.second.Clear();
	}
	for (const auto &state : collisionStates_) {
		if (state.first.objA) {
			state.first.objA->GetCollidingObjects().erase(state.first.objB);
		}
		if (state.first.objB) {
			state.first.objB->GetCollidingObjects().erase(state.first.objA);
		}
	}
	collisionStates_.clear();
	currentCollisionPairs_.clear();
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
	if (!obj) {
		return;
	}

	activeObjects_.erase(
		std::remove(activeObjects_.begin(), activeObjects_.end(), obj),
		activeObjects_.end());

	//========================================
	// 関連する衝突状態の除去
	// objは破棄直前に渡されるため、コールバックを発行して相手へ再入
	// すると、削除処理と衝突処理の順序次第で生ポインタが無効になる。
	// ここでは状態と相互参照だけを先に除去する。
	for (auto stateIt = collisionStates_.begin(); stateIt != collisionStates_.end();) {
		const CollisionPair pair = stateIt->first;
		if (pair.objA == obj || pair.objB == obj) {
			BaseObject *other = pair.objA == obj ? pair.objB : pair.objA;
			if (other && std::find(activeObjects_.begin(), activeObjects_.end(), other) != activeObjects_.end()) {
				other->GetCollidingObjects().erase(obj);
			}
			stateIt = collisionStates_.erase(stateIt);
		} else {
			++stateIt;
		}
	}

	for (auto pairIt = currentCollisionPairs_.begin(); pairIt != currentCollisionPairs_.end();) {
		if (pairIt->objA == obj || pairIt->objB == obj) {
			pairIt = currentCollisionPairs_.erase(pairIt);
		} else {
			++pairIt;
		}
	}

	obj->GetCollidingObjects().clear();
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

	// 隣接セルを正方向だけ生成し、同じセルペアを一度だけ判定する。
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dz = -1; dz <= 1; ++dz) {
				if (dx < 0 || (dx == 0 && dy < 0) || (dx == 0 && dy == 0 && dz <= 0))
					continue;

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
	for (const auto &pair : grid_) {
		const GridCoord &coord = pair.first;
		const GridCell &cell = pair.second;

		CheckAdjacentCellCollisions(coord, cell);
	}
}

///=============================================================================
///						高速衝突判定
bool CollisionManager::FastIntersects(BaseObject *objA, BaseObject *objB) const {
	if (!CanStartCollisionPair(objA, objB)) {
		return false;
	}

	auto colliderA = objA->GetCollider();
	auto colliderB = objB->GetCollider();

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

bool CollisionManager::CanStartCollisionPair(BaseObject *objA, BaseObject *objB) const {
	if (!objA || !objB || objA == objB) {
		return false;
	}

	if (!objA->IsCollisionEnabled() || !objB->IsCollisionEnabled()) {
		return false;
	}

	if (!objA->GetCollider() || !objB->GetCollider()) {
		return false;
	}

	return CanCollideByGroupAndMask(objA, objB);
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
	// 処理内容：Pair開始直前の有効性を再確認する。
	// 理由：先行Pairのコールバックで無効化された対象を、後続Pairへ参加させないため。
	if (isColliding && !CanStartCollisionPair(objA, objB)) {
		return;
	}

	CollisionPair pair(objA, objB);
	if (isColliding && !currentCollisionPairs_.insert(pair).second) {
		return;
	}
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

			// グループフィルタリングが有効な場合
			bool passFilter = true;
			if (enableGroupFilter_) {
				passFilter = (obj->GetGroup() == debugGroupFilter_);
			}

			if (!passFilter)
				continue;

			// 衝突中のオブジェクトは赤、無効は灰色、その他は白
			Vector4 color;
			if (!obj->IsCollisionEnabled()) {
				color = Vector4{0.5f, 0.5f, 0.5f, 1.0f}; // 灰色：無効
			} else if (!obj->GetCollidingObjects().empty()) {
				color = Vector4{1.0f, 0.0f, 0.0f, 1.0f}; // 赤：衝突中
			} else {
				color = Vector4{1.0f, 1.0f, 1.0f, 1.0f}; // 白：衝突していない
			}

			if (lineManager_) {
				lineManager_->DrawSphere(position, radius, color);
			}
		}
	}
}

///=============================================================================
///						グループ間衝突設定
void CollisionManager::SetGroupCollision(uint16_t groupA, uint16_t groupB, bool canCollide) {
	if (groupA >= 16 || groupB >= 16)
		return;

	groupCollisionMatrix_[groupA][groupB] = canCollide;
	groupCollisionMatrix_[groupB][groupA] = canCollide; // 双方向
}

///=============================================================================
///						グループ衝突設定リセット
void CollisionManager::ResetGroupCollisions() {
	// すべてのグループが衝突可能に
	for (int i = 0; i < 16; ++i) {
		for (int j = 0; j < 16; ++j) {
			groupCollisionMatrix_[i][j] = true;
		}
	}
}

///=============================================================================
///						グループ衝突可能性を問い合わせ
bool CollisionManager::CanGroupsCollide(uint16_t groupA, uint16_t groupB) const {
	if (groupA >= 16 || groupB >= 16)
		return false;

	return groupCollisionMatrix_[groupA][groupB];
}

///=============================================================================
///						グループ・レイヤーマスクによる衝突可否判定
bool CollisionManager::CanCollideByGroupAndMask(BaseObject *objA, BaseObject *objB) const {
	uint16_t groupA = objA->GetGroup();
	uint16_t groupB = objB->GetGroup();

	// グループマトリクスで許可されているか確認
	if (!CanGroupsCollide(groupA, groupB))
		return false;

	// グループIDがレイヤーマスク内か確認（ビット単位）
	if (groupB >= 16 || groupA >= 16)
		return false; // グループIDが範囲外

	// objAがobjBのグループと衝突可能か
	if ((objA->GetCollisionLayerMask() & (1 << groupB)) == 0)
		return false;

	// objBがobjAのグループと衝突可能か
	if ((objB->GetCollisionLayerMask() & (1 << groupA)) == 0)
		return false;

	return true;
}

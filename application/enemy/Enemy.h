#pragma once
#include "component/IEnemyComponent.h"
#include "MagMath.h"
#include "base/EnemyBase.h"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <vector>
#include <string>
#include <functional>

using namespace MagMath;

// 前方宣言
class Object3dSetup;
class Player;

/**
 * @brief エネミーコンテナクラス
 *
 * すべてのコンポーネントを管理し、敵の統合ライフサイクルを制御します。
 * EnemyBase から継承して、既存システムとの互換性を保ちます。
 */
class Enemy : public EnemyBase {
public:
	Enemy();
	~Enemy();

	// ========== ライフサイクル ==========

	/**
	 * @brief 敵の初期化
	 * @param enemyTypeId 敵タイプID（"fighter", "scout" など）
	 * @param position スポーン位置
	 */
	void Initialize(const std::string& enemyTypeId, const Vector3& position);

	/**
	 * @brief 互換性用Initialize（旧システム向け）
	 */
	void Initialize(void* object3dSetup, const std::string& modelPath, const Vector3& position) {
		// 旧システムの Initialize をサポート
		// object3dSetup と modelPath は内部的に処理
		Initialize("old_enemy_type", position);
	}

	/**
	 * @brief 毎フレーム更新
	 */
	void Update(float deltaTime);

	/**
	 * @brief 描画
	 */
	void Draw();

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui();

	// ========== コンポーネント管理 ==========

	/**
	 * @brief コンポーネントを追加
	 * @tparam T コンポーネント型
	 * @return 追加されたコンポーネントへのポインタ
	 */
	template<typename T>
	T* AddComponent() {
		std::type_index idx(typeid(T));
		if (components_.find(idx) != components_.end()) {
			return static_cast<T*>(components_[idx].get());
		}
		auto component = std::make_unique<T>();
		T* ptr = component.get();
		components_[idx] = std::move(component);
		ptr->SetOwner(this);
		return ptr;
	}

	/**
	 * @brief コンポーネントを取得
	 * @tparam T コンポーネント型
	 * @return コンポーネントへのポインタ（存在しない場合はnullptr）
	 */
	template<typename T>
	T* GetComponent() const {
		auto it = components_.find(std::type_index(typeid(T)));
		if (it != components_.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	/**
	 * @brief コンポーネントが存在するか確認
	 */
	template<typename T>
	bool HasComponent() const {
		return GetComponent<T>() != nullptr;
	}

	/**
	 * @brief コンポーネントを削除
	 */
	template<typename T>
	void RemoveComponent() {
		components_.erase(std::type_index(typeid(T)));
	}

	// ========== 敵識別情報 ==========

	void SetEnemyTypeId(const std::string& typeId) { enemyTypeId_ = typeId; }
	std::string GetEnemyTypeId() const { return enemyTypeId_; }

	void SetEnemyId(int id) { enemyId_ = id; }
	int GetEnemyId() const { return enemyId_; }

	// ========== 位置・移動情報 ==========

	Vector3 GetPosition() const;
	void SetPosition(const Vector3& pos);

	Vector3 GetVelocity() const;
	void SetVelocity(const Vector3& vel);

	float GetRadius() const;

	// ========== HP・生存状態 ==========

	int GetCurrentHP() const;
	int GetMaxHP() const;
	bool IsAlive() const;
	void TakeDamage(int damage);

	// ========== グループ・部隊管理 ==========

	void SetGroupId(int groupId) { groupId_ = groupId; }
	int GetGroupId() const { return groupId_; }

	void SetGroupRole(int role) { groupRole_ = role; }
	int GetGroupRole() const { return groupRole_; }

	// ========== プレイヤー参照 ==========

	void SetPlayer(Player* player) { player_ = player; }
	Player* GetPlayer() const { return player_; }

	// ========== 編隊制御互換メソッド ==========

	/**
	 * @brief 編隊目標位置を設定（編隊制御用）
	 */
	void SetFormationTargetPosition(const Vector3& targetPos) {
		formationTargetPos_ = targetPos;
	}

	/**
	 * @brief 編隊追尾フラグを設定（編隊制御用）
	 */
	void SetFormationFollowing(bool following) {
		isFollowingFormation_ = following;
	}

	/**
	 * @brief 編隊目標位置を取得
	 */
	Vector3 GetFormationTargetPosition() const { return formationTargetPos_; }

	/**
	 * @brief 編隊追尾中かどうか
	 */
	bool IsFollowingFormation() const { return isFollowingFormation_; }

	/**
	 * @brief パーティクルシステムを設定
	 */
	void SetParticleSystem(void* particleSystem) {
		particleSystem_ = particleSystem;
	}

	/**
	 * @brief パーティクルシステムを設定（旧システム互換性）
	 */
	void SetParticleSystem(void* particle, void* particleSetup) {
		particleSystem_ = particle;
		// particleSetup は内部的に保存
	}

	/**
	 * @brief 敵撃破時のコールバックを設定
	 */
	void SetDefeatCallback(std::function<void(Enemy*)> callback) {
		defeatCallback_ = callback;
	}

	/**
	 * @brief 敵撃破時のコールバックを設定（シンプル版）
	 */
	void SetDefeatCallback(std::function<void()> callback) {
		if (callback) {
			defeatCallback_ = [callback](Enemy*) { callback(); };
		}
	}

private:
	// コンポーネント管理
	std::unordered_map<std::type_index, std::unique_ptr<IEnemyComponent>> components_;

	// 敵識別情報
	std::string enemyTypeId_;
	int enemyId_;

	// 部隊情報
	int groupId_ = -1;        // -1 = 独立敵
	int groupRole_ = 0;       // 0=Leader, 1=Support, 2=Flanker

	// 編隊制御情報
	Vector3 formationTargetPos_ = Vector3(0, 0, 0);
	bool isFollowingFormation_ = false;

	// 外部参照
	Player* player_ = nullptr;
	void* particleSystem_ = nullptr;
	std::function<void(Enemy*)> defeatCallback_;

	// std::type_index のハッシュ関数
	struct TypeIndexHash {
		std::size_t operator()(const std::type_index& t) const {
			return t.hash_code();
		}
	};
};

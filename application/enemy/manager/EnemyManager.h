#pragma once
#include "EnemyBase.h"
#include "EnemyHandle.h"
#include "stage/EnemyStageSystem.h"
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
#include <memory>
#include <unordered_map>
#include <vector>


class EnemyBullet;
class Player;
namespace MagEngine {
	class Object3dSetup;
	class Particle;
	class ParticleSetup;
	class RenderWorld;
	class TrailEffectManager;
}

/// @brief 敵生成時にFactoryへ渡す実行時依存
/// @note 各ポインタはScene側またはEngineサービスが所有する非所有参照。EnemyManagerより先に破棄してはならない。
struct EnemySpawnContext {
	MagEngine::Object3dSetup *object3dSetup = nullptr;
	MagEngine::Particle *particle = nullptr;
	MagEngine::ParticleSetup *particleSetup = nullptr;
	MagEngine::TrailEffectManager *trailEffectManager = nullptr;
	Player *player = nullptr;
	Vector3 position{};
	std::string modelName = "jet.obj";
};

/// @brief Stage定義から個別の敵インスタンスを生成するFactory
/// @note 生成結果の所有権は呼び出し側へ移る。生成後の登録と寿命管理はEnemyManagerが担う。
class EnemyFactory {
public:
	/// @brief 定義と実行時依存から敵を生成
	/// @return 所有権を持つ敵。生成できない定義の場合はnullptr。
	std::unique_ptr<EnemyBase> Create(const EnemySpawnDefinition &definition, const EnemySpawnContext &context);
};

/// @brief ステージ中の敵と敵弾の生成・更新・破棄を一元管理するクラス
/// @details Enemyの所有権はこのクラスが持つ。外部へ返すEnemyBaseポインタはフレームを跨いで保持してはならない。
class EnemyManager {
public:
	/// @brief 敵生成で共有するEngine側サービスを設定
	/// @note 各引数は非所有参照。CreateEnemy()を呼ぶ前に設定する必要がある。
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::Particle *particle,
					MagEngine::ParticleSetup *particleSetup,
					MagEngine::TrailEffectManager *trailEffectManager);

	/// @brief 敵の追跡・攻撃対象となるPlayerを設定
	/// @param player Sceneが所有するPlayerへの非所有参照。未設定時の敵挙動は生成定義に依存する。
	void SetPlayer(Player *player) {
		player_ = player;
	}

	/// @brief 全敵を更新し、無効化済みの敵を回収
	/// @note 毎フレーム実行する。回収後は以前に取得したEnemyBaseポインタが無効になる。
	void Update(float deltaTime);
	/// @brief 現フレームの敵描画対象をRenderWorldへ登録
	/// @note Transform更新後に呼び出す。renderWorldは呼び出し側が所有する。
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);
	void DrawImGui();
	void Clear();

	/// @brief 敵を生成して管理下へ登録
	/// @return 管理中の敵への非所有ポインタ。Clear()、Update()による回収後は使用不可。
	EnemyBase *CreateEnemy(const EnemySpawnDefinition &definition, const Vector3 &position, const std::string &modelName = "jet.obj");
	[[nodiscard]] EnemyBase *ResolveEnemy(EnemyHandle handle);
	[[nodiscard]] const EnemyBase *ResolveEnemy(EnemyHandle handle) const;
	[[nodiscard]] bool IsEnemyValid(EnemyHandle handle) const;
	[[nodiscard]] bool IsEnemyTargetable(EnemyHandle handle) const;

	size_t GetActiveEnemyCount() const;
	size_t GetAliveEnemyCount() const {
		return GetActiveEnemyCount();
	}

	int GetDefeatedCount() const {
		return defeatedCount_;
	}

	const std::vector<std::unique_ptr<EnemyBase>> &GetEnemies() const {
		return enemies_;
	}

	void CollectEnemyBullets(std::vector<EnemyBullet *> &result) const;

private:
	void RemoveInactiveEnemies();

	std::vector<std::unique_ptr<EnemyBase>> enemies_; // Enemyの所有権を保持し、破棄順をEnemyManagerに集約する。
	std::unordered_map<std::uint64_t, EnemyBase *> enemyLookup_; // Handle解決専用の非所有索引。Enemy回収時に同時に削除する。
	std::uint64_t nextEnemyHandleValue_ = 1;
	EnemyFactory enemyFactory_;
	int defeatedCount_ = 0;
	float gameTime_ = 0.0f;

	MagEngine::Object3dSetup *object3dSetup_ = nullptr; // Engine側が所有する3D生成サービスへの非所有参照。
	MagEngine::Particle *particle_ = nullptr; // Engine側が所有するパーティクルサービスへの非所有参照。
	MagEngine::ParticleSetup *particleSetup_ = nullptr; // Engine側が所有するパーティクル設定への非所有参照。
	MagEngine::TrailEffectManager *trailEffectManager_ = nullptr; // Engine側が所有する軌跡エフェクト管理への非所有参照。
	Player *player_ = nullptr; // Sceneが所有する追跡対象への非所有参照。
};

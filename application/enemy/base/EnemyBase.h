#pragma once
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
using Transform = MagMath::Transform;
#include "BaseObject.h"
#include "Object3d.h"
#include "Particle.h"
#include "ParticleSetup.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// 前方宣言
class Object3dSetup;
class Player;
class PlayerBullet;
class PlayerMissile;
namespace MagEngine {
	class RenderWorld;
}

///=============================================================================
///						EnemyBase クラス（敵の基底クラス）
/**
 * @brief すべての敵キャラクターの基底クラス
 *
 * 責務：
 * - 敵の基本情報（HP、位置、速度等）の管理
 * - プレイヤーおよび弾丸との衝突判定処理
 * - パーティクル爆発エフェクトの管理
 * - 敵の消滅処理と状態管理
 * - 敵固有の行動パターンは派生クラスで実装
 */
class EnemyBase : public BaseObject {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	virtual ~EnemyBase() = default;

	/// \brief 基本初期化（派生クラスから呼び出す）
	virtual void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position);

	/// \brief パーティクルシステムの設定
	void SetParticleSystem(MagEngine::Particle *particle,
						   MagEngine::ParticleSetup *particleSetup);

	/// \brief 更新（派生クラスでオーバーライド可能）
	virtual void Update();
	virtual void Update(float deltaTime);

	/// \brief 3D不透明描画対象の登録
	virtual void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief ImGui描画（派生クラスでオーバーライド推奨）
	virtual void DrawImGui();

	///--------------------------------------------------------------
	///							入出力関数
	/// \brief 生存フラグの取得
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief Wave進行上の離脱として非アクティブ化
	void MarkExited();

	/// \brief 位置の取得
	Vector3 GetPosition() const {
		return transform_.translate;
	}

	/// \brief 当たり判定の半径を取得
	float GetRadius() const {
		return radius_;
	}

	/// \brief 現在のHPを取得
	int GetCurrentHP() const {
		return currentHP_;
	}

	/// \brief 最大HPを取得
	int GetMaxHP() const {
		return maxHP_;
	}

	/// \brief ダメージを受ける
	virtual void TakeDamage(int damage, std::function<void()> onDefeatCallback = nullptr);

	/// \brief 撃破コールバックを設定（初期化時に使用）
	void SetDefeatCallback(std::function<void()> callback) {
		onDefeatCallback_ = callback;
	}

	/// \brief ヒットリアクション中かどうか
	bool IsInHitReaction() const {
		return isHitReacting_;
	}

	/// \brief プレイヤー参照を設定
	void SetPlayer(Player *player) {
		player_ = player;
	}

	/// \brief 編隊内の目標位置を設定
	void SetFormationTarget(const Vector3 &targetPosition);

	/// \brief 編隊追従を解除
	void ClearFormationTarget();

	/// \brief 編隊追従の有効状態を設定
	void SetFormationFollowEnabled(bool enabled);

	/// \brief 編隊追従中かどうか
	bool IsFormationFollowEnabled() const {
		return formationFollowEnabled_;
	}

	/// \brief 編隊スロット番号を設定
	void SetFormationSlotIndex(uint32_t index) {
		formationSlotIndex_ = index;
	}

	/// \brief 編隊スロット番号を取得
	uint32_t GetFormationSlotIndex() const {
		return formationSlotIndex_;
	}

	/// \brief 編隊追従速度を設定
	void SetFormationFollowSpeed(float speed) {
		formationFollowSpeed_ = speed;
	}

	/// \brief 編隊追従の鋭さを設定
	void SetFormationFollowSharpness(float sharpness) {
		formationFollowSharpness_ = sharpness;
	}

	/// \brief 編隊側から攻撃可能タイミングを設定
	void SetFormationAttackEnabled(bool enabled) {
		formationAttackEnabled_ = enabled;
	}

	/// \brief 編隊側の攻撃許可状態を取得
	bool IsFormationAttackEnabled() const {
		return formationAttackEnabled_;
	}

	/// \brief 編隊目標へ滑らかに追従
	void UpdateFormationFollow(float deltaTime);

	/// \brief 衝突処理関数（BaseObjectの純粋仮想関数を実装）
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

protected:
	/// \brief ヒットリアクションの開始
	void StartHitReaction();

	/// \brief 破壊状態への移行
	void StartDestroy();

	/// \brief ヒット時のパーティクル生成
	virtual void CreateHitParticle();

	/// \brief 破壊時のパーティクル生成
	virtual void CreateDestroyParticle();

	/// \brief ヒットリアクションの更新
	void UpdateHitReaction(float deltaTime);

	/// \brief 破壊演出の更新
	bool UpdateDestroy(float deltaTime); // 破壊完了したらtrueを返す

	/// \brief 目標座標へのイージング移動（currentVelocity_ を smooth して translate に適用）
	void MoveToward(const Vector3 &target, float speed, float smoothing, float deltaTime);

	/// \brief ２点間の距離を返す
	float GetDistanceTo(const Vector3 &pos) const;

	///--------------------------------------------------------------
	///							メンバ変数
protected:
	//========================================
	// 3Dオブジェクト
	std::unique_ptr<MagEngine::Object3d> obj_;

	//========================================
	// 移動・位置関連
	Transform transform_;
	Vector3 currentVelocity_;

	//========================================
	// 編隊追従関連
	// 理由：EnemyGroup は敵を所有しないため、共通APIで目標だけを渡し寿命管理へ介入しない。
	Vector3 formationTargetPosition_;
	Vector3 formationFollowVelocity_;
	bool formationFollowEnabled_;
	bool formationAttackEnabled_;
	uint32_t formationSlotIndex_;
	float formationFollowSpeed_;
	float formationFollowSharpness_;

	//========================================
	// 基本パラメータ
	float speed_;
	float radius_;
	int currentHP_;
	int maxHP_;

	//========================================
	// 生存時間管理
	float lifeTimer_;
	float maxLifeTime_;
	bool isAlive_;

	//========================================
	// パーティクル関連
	MagEngine::Particle *particle_;
	MagEngine::ParticleSetup *particleSetup_;
	bool particleCreated_;

	//========================================
	// 破壊演出関連
	enum class DestroyState {
		Alive,
		Destroying,
		Dead
	};
	DestroyState destroyState_;
	float destroyTimer_;
	float destroyDuration_;

	//========================================
	// ヒットリアクション関連（簡略化）
	bool isHitReacting_;
	float hitReactionTimer_;
	float hitReactionDuration_;
	Vector3 hitStartPosition_;
	bool isInvincible_;

	//========================================
	// プレイヤー参照
	Player *player_;

	//========================================
	// 撃破コールバック
	std::function<void()> onDefeatCallback_;
};

#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "BaseObject.h"
#include "Object3d.h"
#include <functional>
#include <memory>
#include <string>
#include "Particle.h"
#include "ParticleSetup.h"

// 前方宣言
class Object3dSetup;
class Player;
class PlayerBullet;
class PlayerMissile;

///=============================================================================
///						EnemyBase クラス（敵の基底クラス）
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

	/// \brief 描画
	virtual void Draw();

	/// \brief ImGui描画（派生クラスでオーバーライド推奨）
	virtual void DrawImGui();

	///--------------------------------------------------------------
	///							入出力関数
	/// \brief 生存フラグの取得
	bool IsAlive() const {
		return isAlive_;
	}

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
	void UpdateHitReaction();

	/// \brief 破壊演出の更新
	bool UpdateDestroy(); // 破壊完了したらtrueを返す

	///--------------------------------------------------------------
	///							メンバ変数
protected:
	//========================================
	// 3Dオブジェクト
	std::unique_ptr<MagEngine::Object3d> obj_;

	//========================================
	// 移動・位置関連
	Transform transform_;

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
	// ヒットリアクション関連
	bool isHitReacting_;
	float hitReactionTimer_;
	float hitReactionDuration_;
	int hitFlashCount_;
	Vector3 originalScale_;
	Vector3 hitScale_;
	bool shouldRenderThisFrame_;
	Vector3 knockbackVelocity_;
	float shakeAmplitude_;
	float shakeFrequency_;
	Vector3 hitStartPosition_;
	bool isInvincible_;

	//========================================
	// プレイヤー参照
	Player *player_;

	//========================================
	// 撃破コールバック
	std::function<void()> onDefeatCallback_;
};

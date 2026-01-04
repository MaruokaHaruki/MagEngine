/*********************************************************************
 * \file   Enemy.h
 *
 * \author Harukichimaru
 * \date   June 2025
 * \note
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Object3d.h"
#include "Vector3.h"
#include <memory>
#include <string>

// 前方宣言
class Object3dSetup;
class Particle;
class ParticleSetup;

///=============================================================================
///						Enemyクラス
class Enemy : public BaseObject {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position);

	/// \brief パーティクルシステムの設定
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							入出力関数
	/// \brief 生存フラグの取得
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief 位置の取得
	Vector3 GetPosition() const;

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
	void TakeDamage(int damage);

	/// \brief 衝突処理関数（BaseObjectの純粋仮想関数を実装）
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	/// \brief ヒットリアクションの開始
	void StartHitReaction();

	/// \brief ヒットリアクション中かどうか
	bool IsInHitReaction() const {
		return isHitReacting_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 3Dオブジェクト
	std::unique_ptr<Object3d> obj_;

	//========================================
	// 移動・位置関連（メイン管理）
	Transform transform_; // メインのトランスフォーム（位置情報の一括管理）

	//========================================
	// スピード関連
	float speed_; // 移動速度

	//========================================
	// 生存時間管理
	float lifeTimer_;	// 生存時間タイマー
	float maxLifeTime_; // 最大生存時間

	//========================================
	// 状態管理
	bool isAlive_;	// 生存フラグ
	float radius_;	// 当たり判定用の半径
	int currentHP_; // 現在のHP
	int maxHP_;		// 最大HP

	//========================================
	// パーティクル関連
	Particle *particle_;		   // パーティクルシステム
	ParticleSetup *particleSetup_; // パーティクル設定
	bool particleCreated_;		   // パーティクル生成フラグ

	//========================================
	// 破壊演出関連
	enum class DestroyState {
		Alive,		// 生存中
		Destroying, // 破壊中（パーティクル再生中）
		Dead		// 完全に消滅
	};
	DestroyState destroyState_; // 破壊状態
	float destroyTimer_;		// 破壊演出タイマー
	float destroyDuration_;		// 破壊演出の持続時間

	//========================================
	// ヒットリアクション関連
	bool isHitReacting_;		 // ヒットリアクション中フラグ
	float hitReactionTimer_;	 // ヒットリアクションタイマー
	float hitReactionDuration_;	 // ヒットリアクション持続時間
	int hitFlashCount_;			 // 点滅カウント
	Vector3 originalScale_;		 // 元のスケール
	Vector3 hitScale_;			 // ヒット時のスケール
	bool shouldRenderThisFrame_; // このフレームで描画するか
	Vector3 knockbackVelocity_;	 // ノックバック速度
	float shakeAmplitude_;		 // 揺れの振幅
	float shakeFrequency_;		 // 揺れの周波数
	Vector3 hitStartPosition_;	 // ヒット開始時の位置（復帰用）
};

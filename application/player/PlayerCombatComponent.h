/*********************************************************************
 * \file   PlayerCombatComponent.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note   プレイヤー戦闘コンポーネント - 射撃、ミサイル、弾幕管理
 *********************************************************************/
#pragma once
#include "MagMath.h"
#include "Object3d.h"
#include "PlayerBullet.h"
#include "PlayerMissile.h"
#include <memory>
#include <vector>

using namespace MagMath;

//========================================
// 前方宣言
class Object3dSetup;
class EnemyManager;
class EnemyBase; // Enemy から EnemyBase に変更

///=============================================================================
///						クラス定義
class PlayerCombatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 初期化
	/// @param object3dSetup object3dのセットアップ情報
	void Initialize(MagEngine::Object3dSetup *object3dSetup);

	/// @brief 更新
	void Update(float deltaTime);

	/// @brief 弾発射処理
	void ShootBullet(const Vector3 &position, const Vector3 &direction);

	/// @brief ミサイル発射処理
	void ShootMissile(const Vector3 &position, const Vector3 &direction, EnemyBase *targetEnemy = nullptr);

	/// @brief 描画
	void Draw();

	/// @brief ImGui描画
	void DrawImGui();

	/// @brief 弾の更新
	void UpdateBullets();

	/// @brief ミサイルの更新
	void UpdateMissiles();

	/// @brief 弾の描画
	void DrawBullets();

	/// @brief ミサイルの描画
	void DrawMissiles();

	/// @brief 弾が撃てるか判定
	bool CanShootBullet() const;

	/// @brief ミサイルが撃てるか判定
	bool CanShootMissile() const;

	//========================================
	// EnemyManager設定（ミサイル用）
	/// @brief 敵マネージャーの設定
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}

	//========================================
	// ゲッター
	/// @brief 弾とミサイルの取得
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return bullets_;
	}
	/// @brief ミサイルの取得
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return missiles_;
	}
	/// @brief 弾の発射方向を取得
	Vector3 GetBulletFireDirection() const {
		return bulletFireDirection_;
	}

	//========================================
	// セッター（ImGui用）
	/// @brief 最大発射クールタイムの設定
	void SetMaxShootCoolTime(float time) {
		maxShootCoolTime_ = time;
	}
	/// @brief 最大ミサイルクールタイムの設定
	void SetMaxMissileCoolTime(float time) {
		maxMissileCoolTime_ = time;
	}

	//========================================
	// プライベートメンバ
private:
	//========================================
	// コンポーネント参照
	MagEngine::Object3dSetup *object3dSetup_;
	EnemyManager *enemyManager_;

	//========================================
	// 弾・ミサイル
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	std::vector<std::unique_ptr<PlayerMissile>> missiles_;

	//========================================
	// クールタイム管理
	float shootCoolTime_;
	float maxShootCoolTime_;
	float missileCoolTime_;
	float maxMissileCoolTime_;

	//========================================
	// 弾発射方向（HUD用）
	Vector3 bulletFireDirection_;
};
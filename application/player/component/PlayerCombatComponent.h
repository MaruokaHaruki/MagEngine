#pragma once
#include "PlayerBullet.h"
#include "PlayerMissile.h"
#include "PlayerLockedOnComponent.h"
#include "Vector3.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// 前方宣言
class Object3dSetup;
class EnemyManager;
class EnemyBase; // Enemy から EnemyBase に変更
namespace MagEngine {
	class TrailEffectManager;
	class LineManager;
	class RenderWorld;
}

///=============================================================================
///						戦闘管理コンポーネント
/// @brief プレイヤー弾・ミサイルの生成、更新、描画登録を管理する。
/// @details 投射物はこのクラスが所有する。EnemyManagerと描画セットアップは外部所有の依存として扱うため、
///          Initialize()およびSetEnemyManager()の後も投射物の更新・描画が終わるまで有効でなければならない。
class PlayerCombatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 外部サービスと投射物の初期状態を設定する
	/// @param object3dSetup 弾モデルの生成に使う非所有参照。発射前に有効である必要がある。
	/// @param trailEffectManager トレイルを使用しない場合はnullptrを許容する非所有参照。
	/// @param lineManager デバッグ線を使用しない場合はnullptrを許容する非所有参照。
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::TrailEffectManager *trailEffectManager = nullptr,
					MagEngine::LineManager *lineManager = nullptr);
	/// @brief クールタイムとミサイル回復を更新する
	/// @note 投射物そのものの更新はUpdateBullets()/UpdateMissiles()へ分け、Scene側で更新順を制御する。
	void Update(float deltaTime);

	///--------------------------------------------------------------
	///                        射撃処理
	/// @brief 通常弾を生成する
	/// @param position 発射時のワールド座標
	/// @param direction 発射基準となる方向
	/// @note 弾アシストが有効かつEnemyManager設定済みの場合のみ、発射方向を補正する。
	void ShootBullet(const Vector3 &position, const Vector3 &direction);
	/// @brief 指定ターゲットへ追尾するミサイルを生成する
	/// @param position 発射時のワールド座標
	/// @param direction 発射基準となる方向
	/// @param target 追尾対象。nullptrの場合はターゲットなしで発射する。
	void ShootMissile(const Vector3 &position, const Vector3 &direction, EnemyBase *target); // Enemy* から EnemyBase* に変更

	/// @brief マルチロックオンで複数敵に同時発射
	/// @param position ミサイル発射位置
	/// @param direction 発射基準方向
	/// @param targets ロックオン対象の非所有ハンドル一覧。無効な要素は発射対象から除外する。
	/// @note 残弾数まで発射し、実際に生成できた数だけ残弾を消費する。
	void ShootMultipleMissiles(const Vector3 &position, const Vector3 &direction,
							   const std::vector<EnemyBase *> &targets);

	/// @brief 所有する通常弾を更新し、寿命切れまたは削除済みの弾を除去する
	/// @param deltaTime 前フレームからの経過時間（秒）
	void UpdateBullets(float deltaTime);
	/// @brief 所有するミサイルを更新し、寿命切れまたは削除済みのミサイルを除去する
	/// @param deltaTime 前フレームからの経過時間（秒）
	void UpdateMissiles(float deltaTime);

	///--------------------------------------------------------------
	///                        描画
	/// @brief 所有する生存中の投射物をフレーム描画対象へ登録する
	/// @note UpdateBullets()/UpdateMissiles()後の状態を登録するため、Sceneの描画登録フェーズで呼び出す。
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	///--------------------------------------------------------------
	///                        ゲッター
	/// @brief 所有する通常弾一覧を取得する
	/// @return 生存管理中の通常弾一覧への参照
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return bullets_;
	}
	/// @brief 所有するミサイル一覧を取得する
	/// @return 生存管理中のミサイル一覧への参照
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return missiles_;
	}
	/// @brief 次の通常弾を発射可能になるまでの残り時間を取得する
	/// @return 発射クールタイムの残り秒数
	float GetShootCoolTime() const {
		return shootCoolTime_;
	}
	/// @brief 通常弾を発射可能かを判定する
	/// @return クールタイムが終了している場合はtrue、それ以外はfalse
	bool CanShootBullet() const {
		return shootCoolTime_ <= 0.0f;
	}
	/// @brief ミサイルを発射可能かを判定する
	/// @return ミサイル残弾が1以上の場合はtrue、それ以外はfalse
	bool CanShootMissile() const {
		return missileAmmo_ > 0;
	}
	/// @brief 現在のミサイル残弾数を取得する
	/// @return 発射可能なミサイル数
	int GetMissileAmmo() const {
		return missileAmmo_;
	}
	/// @brief ミサイル残弾の上限を取得する
	/// @return 最大ミサイル残弾数
	int GetMaxMissileAmmo() const {
		return maxMissileAmmo_;
	}
	/// @brief 次のミサイル回復までの経過時間を取得する
	/// @return ミサイル回復タイマーの経過秒数
	float GetMissileRecoveryTimer() const {
		return missileRecoveryTimer_;
	}
	/// @brief ミサイル1発を回復するまでの設定時間を取得する
	/// @return ミサイル回復に必要な秒数
	float GetMissileRecoveryTime() const {
		return maxMissileRecoveryTime_;
	}
	/// @brief HUD表示に用いる直近の通常弾発射方向を取得する
	/// @return 最終的に発射へ使用した方向ベクトル
	Vector3 GetBulletFireDirection() const {
		return bulletFireDirection_;
	}

	///--------------------------------------------------------------
	///                        セッター
	/// @brief ミサイルの探索・追尾に用いる敵管理を設定する
	/// @note 所有権は取得しない。弾アシストと生成済みミサイルが参照する間は有効でなければならない。
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
	}
	void SetMaxShootCoolTime(float coolTime) {
		maxShootCoolTime_ = coolTime;
	}
	void SetMaxMissileAmmo(int ammo) {
		maxMissileAmmo_ = ammo;
		missileAmmo_ = std::min(missileAmmo_, maxMissileAmmo_);
	}
	void SetMissileRecoveryTime(float recoveryTime) {
		maxMissileRecoveryTime_ = recoveryTime;
	}
	void SetBulletModelPath(const std::string &modelPath) {
		bulletModelPath_ = modelPath;
	}
	void SetMissileModelPath(const std::string &modelPath) {
		missileModelPath_ = modelPath;
	}
	void SetTrailEffectManager(MagEngine::TrailEffectManager *trailEffectManager) {
		trailEffectManager_ = trailEffectManager;
	}

	///--------------------------------------------------------------
	///                        弾アシスト機能設定
	/// @brief 弾アシストの有効化
	void SetBulletAssistEnabled(bool enabled) {
		isBulletAssistEnabled_ = enabled;
	}

	/// @brief 弾アシストが有効か取得
	bool IsBulletAssistEnabled() const {
		return isBulletAssistEnabled_;
	}

	/// @brief 弾アシスト視野角を設定（度数法）
	void SetBulletAssistFOV(float fov) {
		bulletAssistFOV_ = fov;
	}

	/// @brief 弾アシスト視野角を取得
	float GetBulletAssistFOV() const {
		return bulletAssistFOV_;
	}

	/// @brief 弾アシスト範囲を設定（メートル）
	void SetBulletAssistRange(float range) {
		bulletAssistRange_ = range;
	}

	/// @brief 弾アシスト範囲を取得
	float GetBulletAssistRange() const {
		return bulletAssistRange_;
	}

	/// @brief 弾アシスト強度を設定（0.0～1.0）
	void SetBulletAssistStrength(float strength) {
		bulletAssistStrength_ = (strength < 0.0f) ? 0.0f : (strength > 1.0f) ? 1.0f : strength;
	}

	/// @brief 弾アシスト強度を取得
	float GetBulletAssistStrength() const {
		return bulletAssistStrength_;
	}

private:
	///--------------------------------------------------------------
	///                        メンバ変数
	MagEngine::Object3dSetup *object3dSetup_;           // 弾モデル生成に使う外部所有のセットアップ
	MagEngine::TrailEffectManager *trailEffectManager_; // トレイル生成に使う外部所有のサービス。nullptrを許容する。
	MagEngine::LineManager *lineManager_;               // ミサイルのデバッグ線用。nullptrを許容する。
	EnemyManager *enemyManager_;                        // 弾アシストとミサイル追尾に使う外部所有の敵管理

	std::vector<std::unique_ptr<PlayerBullet>> bullets_;   // 生存期間をこのコンポーネントが所有する通常弾
	std::vector<std::unique_ptr<PlayerMissile>> missiles_; // 生存期間をこのコンポーネントが所有するミサイル

	float shootCoolTime_;		   // 現在のクールタイム
	float maxShootCoolTime_;	   // 最大クールタイム
	int missileAmmo_;			   // 現在のミサイル残弾
	int maxMissileAmmo_;		   // 最大ミサイル残弾数
	float missileRecoveryTimer_;   // ミサイル回復タイマー
	float maxMissileRecoveryTime_; // ミサイル回復時間（3秒で1発）

	Vector3 bulletFireDirection_; // 弾の発射方向（HUD用）

	std::string bulletModelPath_;  // 弾のモデルパス
	std::string missileModelPath_; // ミサイルのモデルパス

	///--------------------------------------------------------------
	///                        弾アシスト機能設定
	bool isBulletAssistEnabled_ = true;	// 弾アシストが有効か
	float bulletAssistFOV_ = 26.0f;		// 弾アシスト視野角（度数法）
	float bulletAssistRange_ = 75.0f;	// 弾アシスト検出範囲（メートル）
	float bulletAssistStrength_ = 0.60f; // アシスト強度（0.0～1.0）
};

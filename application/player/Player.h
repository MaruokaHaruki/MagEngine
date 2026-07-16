/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \brief  プレイヤーキャラクターの総合管理クラス
 *
 * 責務：
 * - 各コンポーネント（移動・HP・射撃・ロックオン・敗北演出）の統合管理
 * - 入力処理の委譲
 * - オブジェクト管理
 *
 * 各責務は以下のコンポーネントに分離：
 * - PlayerMovementComponent: 移動・バレルロール・ブースト
 * - PlayerCombatComponent: 弾・ミサイル発射
 * - PlayerHealthComponent: HP・ダメージ
 * - PlayerLockedOnComponent: ロックオン機能
 * - PlayerDefeatComponent: 敗北演出
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "MagMath.h"
using Vector3 = MagMath::Vector3;
using Transform = MagMath::Transform;
#include "Object3d.h"
#include "ParticleEmitter.h"
#include "PlayerConstants.h"
#include "ResourcePaths.h"
#include "component/PlayerCombatComponent.h"
#include "component/PlayerDefeatComponent.h"
#include "component/PlayerHealthComponent.h"
#include "component/PlayerJustAvoidanceComponent.h"
#include "component/PlayerLockedOnComponent.h"
#include "component/PlayerMovementComponent.h"
#include "GameOverAnimation.h"
#include <memory>
#include <string>
#include <vector>

//========================================
// 前方宣言
class Object3dSetup;
class EnemyManager;
class EnemyBase; // Enemy から EnemyBase に変更
class EnemyBullet;
namespace MagEngine {
	class Input;
	class LineManager;
	class TrailEffectManager;
	class RenderWorld;
}

///=============================================================================
/// @brief 武装設定構造体
/// @details プレイヤーの弾とミサイルの全設定を一元管理
/// @note 実行時に動的に変更可能で、ゲームバランス調整に使用される
struct WeaponConfig {
	//========================================
	//          弾（銃）の設定
	//========================================
	/// @brief 弾のモデルパス
	/// @note ResourcePath::Model::BULLET はデフォルト値
	std::string bulletModelPath = ResourcePath::Model::BULLET;
	
	/// @brief 弾のテクスチャパス
	/// @details 未使用時は空文字列。設定されない場合はデフォルトテクスチャを使用
	std::string bulletTexturePath = ResourcePath::Texture::BULLET_DEFAULT;
	
	/// @brief 弾の飛行速度（units/秒）
	/// @details 大きいほど弾が素早く移動する。値が小さいと敵が回避しやすくなる
	/// @note PlayerCombatComponent::ShootBullet() で使用
	float bulletSpeed = PlayerConstants::Weapon::BULLET_SPEED;
	
	/// @brief 弾の生存時間（秒）
	/// @details この時間を超えた弾は自動的に削除される
	/// @warning ステージサイズが大きい場合、この値を大きくすると処理負荷が増える
	float bulletMaxLifeTime = PlayerConstants::Weapon::BULLET_LIFETIME;
	
	/// @brief 弾の当たり判定半径（units）
	/// @details 敵の当たり判定と交差判定を行う際に使用
	/// @note 値が大きいほど当たりやすくなるが、見た目と乖離する可能性がある
	float bulletRadius = PlayerConstants::Weapon::BULLET_RADIUS;
	
	/// @brief 連射クールタイム（秒）
	/// @details この時間が経過するまで次の弾を発射できない
	/// @note 0より大きい値を設定することで連射制限を実装
	float shootCoolTime = PlayerConstants::Weapon::SHOOT_COOLDOWN;

	//========================================
	//          ミサイルの設定
	//========================================
	/// @brief ミサイルのモデルパス
	/// @note ResourcePath::Model::MISSILE はデフォルト値
	std::string missileModelPath = ResourcePath::Model::MISSILE;
	
	/// @brief ミサイルのテクスチャパス
	/// @details 未使用時は空文字列。設定されない場合はデフォルトテクスチャを使用
	std::string missileTexturePath = ResourcePath::Texture::MISSILE_DEFAULT;
	
	/// @brief ミサイルの飛行速度（units/秒）
	/// @details 弾より遅く設計。敵への追尾が可能になるバランスポイント
	/// @note 敵の移動速度を考慮して設定する必要がある
	float missileSpeed = PlayerConstants::Weapon::MISSILE_SPEED;
	
	/// @brief ミサイルの最大旋回速度（度/秒）
	/// @details 敵を追尾する際の回転速度。大きいほど素早く敵を追う
	/// @warning 値が大きすぎるとミサイルが発射位置に戻って衝突する可能性がある
	float missileMaxTurnRate = PlayerConstants::Weapon::MISSILE_TURN_RATE;
	
	/// @brief ミサイルの生存時間（秒）
	/// @details この時間内にターゲットに命中しない場合、自動的に削除される
	/// @note 十分な時間がないと、敵が画面外に移動した時に消滅してしまう
	float missileMaxLifeTime = PlayerConstants::Weapon::MISSILE_LIFETIME;
	
	/// @brief ミサイル最大残弾数
	/// @details この数値に達するまでミサイルの発射ができない（回復待ち）
	/// @note ゲーム難易度調整の重要なパラメータ
	int missileMaxAmmo = PlayerConstants::Weapon::MISSILE_MAX_AMMO;
	
	/// @brief ミサイル1発の回復時間（秒）
	/// @details missileMaxAmmoに達するまで、この時間ごとに1発ずつ回復する
	/// @note 総回復時間 = missileMaxAmmo * missileRecoveryTime
	float missileRecoveryTime = PlayerConstants::Weapon::MISSILE_RECOVERY_TIME;

	//========================================
	//          拡張用
	//========================================
	// マシンガンなど他の武装タイプはここに追加予定
};

///=============================================================================
///						クラス定義
class Player : public BaseObject {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 初期化
	/// @param object3dSetup object3dのセットアップ情報
	/// @param modelPath モデルパス
	/// @details プレイヤーのすべてのコンポーネント、オブジェクト、ゲージを初期化
	/// @note Initialize() の後は必ず SetEnemyManager() を呼び出すこと
	void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, MagEngine::Input &input, MagEngine::LineManager &lineManager);
	
	/// @brief 毎フレーム更新
	/// @details 移動、射撃、各種コンポーネントの状態を更新
	/// @note Update() は必ず毎フレーム呼び出す必要がある
	void Update(float unscaledDeltaTime, float gameplayDeltaTime);
	
	/// @brief ImGui用デバッグパネル描画
	/// @details HPゲージ、ブーストゲージ、移動情報、ロックオン情報などを表示
	/// @warning ImGui パネルの表示はゲームプレイに影響しない（デバッグのみ）
	void DrawImGui();

	/// @brief 3D不透明描画対象の登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	//========================================
	// EnemyManager設定（ミサイル・ロックオン用）
	/// @brief 敵マネージャーの設定
	/// @param enemyManager ゲーム内すべての敵を管理するマネージャー
	/// @details ミサイルの自動追尾と ロックオン機能に必須
	/// @note Initialize() 直後に必ず呼び出す必要がある。呼び出さないとミサイルが発射できない
	void SetEnemyManager(EnemyManager *enemyManager) {
		enemyManager_ = enemyManager;
		combatComponent_.SetEnemyManager(enemyManager);
		lockedOnComponent_.SetEnemyManager(enemyManager);
	}
	
	/// @brief 敵マネージャーの取得
	/// @return 現在設定されている EnemyManager へのポインタ
	/// @details HUD描画時にロックオン情報を取得する際に使用
	EnemyManager *GetEnemyManager() const {
		return enemyManager_;
	}

	//========================================
	// TrailEffectManager設定（弾・ミサイルトレイル用）
	/// @brief トレイルエフェクトマネージャーの設定
	/// @param trailEffectManager 弾・ミサイルの軌跡エフェクトを管理
	/// @details 設定することで、弾とミサイルの軌跡がレンダリングされるようになる
	/// @note 設定しない場合、軌跡エフェクトは表示されない（ゲーム機能上の問題はない）
	void SetTrailEffectManager(MagEngine::TrailEffectManager *trailEffectManager) {
		combatComponent_.SetTrailEffectManager(trailEffectManager);
	}

	//========================================
	// ロックオン機能（PlayerLockedOnComponentに委譲）
	/// @brief ロックオン対象が存在するか確認
	/// @return ロックオン中なら true、無ければ false
	/// @details HUD のロックオンマーカー表示判定に使用
	bool HasLockOnTarget() const {
		return lockedOnComponent_.HasLockOnTarget();
	}
	
	/// @brief プライマリロックオン対象を取得
	/// @return メインロックオン対象の敵。ロック中でなければ nullptr
	/// @details 最初にロックされた敵（優先度が最も高い）を返す
	EnemyHandle GetLockOnTargetHandle() const {
		return lockedOnComponent_.GetPrimaryTargetHandle();
	}
	
	/// @brief 全ロックオン対象を取得
	/// @return ロック中のすべての敵のポインタのベクトル
	/// @details マルチロック機能で同時に複数敵をロック可能
	/// @note 最大数は PlayerLockedOnComponent で定義される
	const std::vector<EnemyHandle> &GetAllLockOnTargetHandles() const {
		return lockedOnComponent_.GetAllTargetHandles();
	}
	
	/// @brief ロックオン対象の数を取得
	/// @return 現在ロック中の敵の数
	/// @details 0 なら誰もロックされていない状態
	size_t GetLockOnTargetCount() const {
		return lockedOnComponent_.GetTargetCount();
	}
	
	/// @brief ミサイル長押しロックオンモード中か確認
	/// @return ロックオンモード中なら true
	/// @details ミサイルボタン長押し中の状態を取得
	/// @note ロックオン中のみミサイルを発射できる
	bool IsMissileLockOnMode() const {
		return isInLockOnMode_;
	}
	
	/// @brief 現在照準内にいるロック候補を取得
	/// @return 照準円内で最も近い敵。候補がなければ nullptr
	/// @details 照準カーソル近くの敵を取得（ロック判定の前段階）
	EnemyHandle GetAimingLockOnTargetHandle() const {
		return lockedOnComponent_.GetAimingTargetHandle();
	}
	
	/// @brief ロックオン範囲をセット
	/// @param range ロックオン有効距離（units）
	/// @details この範囲内の敵のみロックオン対象となる
	/// @note ゲーム難易度調整に使用
	void SetLockOnRange(float range) {
		lockedOnComponent_.SetLockOnRange(range);
	}
	
	/// @brief ロックオン範囲をゲット
	/// @return 現在設定されているロックオン有効距離
	float GetLockOnRange() const {
		return lockedOnComponent_.GetLockOnRange();
	}
	
	/// @brief ロックオン視野角をゲット
	/// @return ロックオンの視野角（度数法）
	/// @details 狭いほどロックしにくく、広いほどロックしやすい
	float GetLockOnFOV() const {
		return lockedOnComponent_.GetLockOnFOV();
	}

	///--------------------------------------------------------------
	///                        敗北演出（PlayerDefeatComponentに委譲）
	/// @brief 敗北状態か確認
	/// @return 敗北済みなら true、生存中なら false
	/// @details HP が0になると敗北フラグが立つ
	/// @note 敗北中はダメージを受けない（二重敗北を防ぐ）
	bool IsDefeated() const {
		return defeatComponent_.IsDefeated();
	}
	
	/// @brief 敗北演出完了判定
	/// @return 敗北演出が完全に終了したら true
	/// @details 敗北演出の所要時間は PlayerConstants::Defeat::ANIMATION_DURATION で定義
	/// @note ゲームオーバー画面への遷移タイミングに使用
	bool IsDefeatAnimationComplete() const {
		return defeatComponent_.IsDefeatAnimationComplete();
	}
	
	/// @brief 敗北演出開始
	/// @details ノーズダイブと落下のアニメーションを開始する
	/// @warning 通常は TakeDamage() が HPを0にした時に自動的に呼ばれる
	void StartDefeatAnimation() {
		defeatComponent_.StartDefeatAnimation();
	}

	///--------------------------------------------------------------
	///                        ジャスト回避（PlayerJustAvoidanceComponentに委譲）
	/// @brief ジャスト回避ウィンドウ内かどうか
	/// @return ジャスト判定の有効時間帯なら true
	/// @details 敵弾がプレイヤーに近づく直前の一瞬のタイミング
	/// @note UI で視覚的フィードバックを提供するのに使用
	bool IsInJustAvoidanceWindow() const {
		return justAvoidanceComponent_.IsInJustAvoidanceWindow();
	}
	
	/// @brief ジャスト回避ウィンドウの残り時間を取得
	/// @return ウィンドウ内の残り時間（秒）
	/// @details 0.0 になるとウィンドウが閉じる
	/// @note タイミング表示バーの実装に使用
	float GetJustAvoidanceWindowTimeRemaining() const {
		return justAvoidanceComponent_.GetJustAvoidanceWindowTimeRemaining();
	}

	/// @brief ジャスト回避ウィンドウの最大時間を取得
	/// @return 判定ウィンドウの最大時間（秒）
	float GetJustAvoidanceWindowSize() const {
		return justAvoidanceComponent_.GetJustAvoidanceWindowSize();
	}
	
	/// @brief 直近のジャスト回避成功率を取得
	/// @return 0.0～1.0（1.0が完璧なタイミング）
	/// @details スロー強度や報酬の計算に使用
	/// @note 失敗した場合も 0.0 を返す（判定ウィンドウ外など）
	float GetJustAvoidanceSuccessRate() const {
		return justAvoidanceComponent_.GetLastSuccessRate();
	}
	
	/// @brief スロー演出中かどうか
	/// @return スロー中なら true
	/// @details ジャスト回避成功時に発動し、敵弾やプレイヤー操作が遅くなる
	bool IsSlowActive() const {
		return justAvoidanceComponent_.IsSlowActive();
	}
	
	/// @brief 現在のスロー強度を取得
	/// @return 0.0～1.0（1.0で最強スロー）
	/// @details 成功度によってスロー効果が段階的に変わる
	float GetSlowStrength() const {
		return justAvoidanceComponent_.GetSlowStrength();
	}
	
	/// @brief 機体強化バフ中かどうか
	/// @return バフ中なら true
	/// @details ジャスト回避成功時に攻撃力と移動速度が上昇
	bool IsPowerUpActive() const {
		return justAvoidanceComponent_.IsPowerUpActive();
	}
	
	/// @brief 現在の攻撃力倍率を取得
	/// @return 1.0 を超える値（バフなしは 1.0）
	/// @details ミサイルの威力計算に使用
	float GetAttackMultiplier() const {
		return justAvoidanceComponent_.GetAttackMultiplier();
	}
	
	/// @brief 現在の移動速度倍率を取得
	/// @return 1.0 を超える値（バフなしは 1.0）
	/// @details 移動速度の計算に使用
	float GetSpeedMultiplier() const {
		return justAvoidanceComponent_.GetSpeedMultiplier();
	}

	/// @brief ジャスト回避コンポーネントを取得
	/// @return PlayerJustAvoidanceComponent へのポインタ
	/// @details 外部からスロー効果を適用する際に使用
	/// @warning 通常は使用不要。内部的な用途に限定
	PlayerJustAvoidanceComponent *GetJustAvoidanceComponent() {
		return &justAvoidanceComponent_;
	}

	/// @brief ジャスト回避が今フレーム成功したか
	/// @return このフレームで成功したなら true（1フレームのみ有効）
	/// @details 演出トリガーや音声再生の判定に使用
	/// @note 次のフレームで自動的に false にリセットされる
	bool IsJustAvoidanceSuccessThisFrame() const {
		return justAvoidanceSuccessThisFrame_;
	}

	/// @brief 最後のジャスト回避成功率を取得
	/// @return 前回のジャスト回避の成功度（0.0～1.0）
	/// @details リプレイ時にスロー効果を再現する際に使用
	float GetLastJustAvoidanceSuccessRate() const {
		return lastJustAvoidanceSuccessRate_;
	}
	
	/// @brief ジャスト回避ウィンドウサイズを設定
	/// @param windowSize ウィンドウサイズ（秒）
	/// @details 大きいほどジャスト回避がしやすくなる
	/// @note ゲーム難易度調整用
	void SetJustAvoidanceWindowSize(float windowSize) {
		justAvoidanceComponent_.SetJustAvoidanceWindowSize(windowSize);
	}
	
	/// @brief 敵弾検出範囲を設定
	/// @param radius 検出半径（units）
	/// @details この範囲内の敵弾がプレイヤーに接近したら判定開始
	void SetJustAvoidanceDetectionRadius(float radius) {
		justAvoidanceComponent_.SetDetectionRadius(radius);
	}
	
	/// @brief スロー演出の持続時間を設定
	/// @param duration スロー効果の継続時間（秒）
	void SetSlowDuration(float duration) {
		justAvoidanceComponent_.SetSlowDuration(duration);
	}
	
	/// @brief スロー強度を設定
	/// @param strength スロー強度（0.0～1.0）
	/// @details 1.0 に近いほど時間が遅くなる
	void SetSlowStrength(float strength) {
		justAvoidanceComponent_.SetSlowStrength(strength);
	}
	
	/// @brief 機体強化バフの持続時間を設定
	/// @param duration バフの継続時間（秒）
	void SetPowerUpDuration(float duration) {
		justAvoidanceComponent_.SetPowerUpDuration(duration);
	}
	
	/// @brief 攻撃力倍率を設定
	/// @param multiplier 攻撃力倍率（デフォルト 1.0）
	/// @details 1.5 なら攻撃力が1.5倍になる
	void SetAttackMultiplier(float multiplier) {
		justAvoidanceComponent_.SetAttackMultiplier(multiplier);
	}
	
	/// @brief 移動速度倍率を設定
	/// @param multiplier 速度倍率（デフォルト 1.0）
	void SetSpeedMultiplier(float multiplier) {
		justAvoidanceComponent_.SetSpeedMultiplier(multiplier);
	}

	///--------------------------------------------------------------
	///                        ゲッター（基本情報）
	/// @brief プレイヤーの現在位置を取得
	/// @return ワールド座標での位置ベクトル
	/// @details カメラ操作やUI表示に使用
	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}
	
	/// @brief Object3d の取得
	/// @return プレイヤーの3Dオブジェクト
	/// @details 外部での直接操作は想定していない（内部管理用）
	MagEngine::Object3d *GetObject3d() const {
		return obj_.get();
	}
	
	/// @brief 発射済みの弾を取得
	/// @return 生存中のプレイヤー弾のリスト
	/// @details 敵との当たり判定や描画に使用
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return combatComponent_.GetBullets();
	}
	
	/// @brief ミサイルを取得
	/// @return 生存中のプレイヤーミサイルのリスト
	/// @details 敵への追尾判定と描画に使用
	const std::vector<std::unique_ptr<PlayerMissile>> &GetMissiles() const {
		return combatComponent_.GetMissiles();
	}

	//========================================
	// ブースト・バレルロール関連ゲッター（HUD用）
	/// @brief ブーストゲージの取得
	/// @return 現在のブーストゲージ値（0.0～maxBoostGauge_）
	/// @details HUD のブーストバーの表示に使用
	/// @note ジャスト回避のボーナス後、即座に反映される
	float GetBoostGauge() const {
		return movementComponent_.GetBoostGauge();
	}
	
	/// @brief 最大ブーストゲージを取得
	/// @return ブーストゲージの最大値
	/// @details ゲージの表示スケール計算に使用
	float GetMaxBoostGauge() const {
		return movementComponent_.GetMaxBoostGauge();
	}
	
	/// @brief バレルロール中か
	/// @return ロール中なら true
	/// @details HUD のアクティビティ表示に使用
	bool IsBarrelRolling() const {
		return movementComponent_.IsBarrelRolling();
	}
	
	/// @brief バレルロール進行度を取得
	/// @return 0.0～1.0（1.0で完了）
	/// @details UI のプログレスバー表示に使用
	float GetBarrelRollProgress() const {
		return movementComponent_.GetBarrelRollProgress();
	}
	
	/// @brief ブースト中か
	/// @return ブースト中なら true
	/// @details 映像効果や音声の再生判定に使用
	bool IsBoosting() const {
		return movementComponent_.IsBoosting();
	}

	//========================================
	// Transform関連のゲッター（GameClearAnimation用）
	/// @brief Transform を取得
	/// @return プレイヤーの Transform へのポインタ
	/// @details ゲームクリア演出で位置や回転を操作する際に使用
	/// @note オブジェクトがない場合は nullptr を返す
	MagMath::Transform *GetTransform() const {
		return obj_ ? obj_->GetTransform() : nullptr;
	}

	//========================================
	// HP関連
	/// @brief 現在の HP を取得
	/// @return 現在のHP値
	/// @details HUD の HP バー表示に使用
	int GetCurrentHP() const {
		return healthComponent_.GetCurrentHP();
	}
	
	/// @brief 最大 HP を取得
	/// @return 最大HP値
	/// @details HP 比率の計算に使用
	int GetMaxHP() const {
		return healthComponent_.GetMaxHP();
	}
	
	/// @brief HP の割合を取得
	/// @return 0.0～1.0（1.0が満タン）
	/// @details HUD の HP バー表示に使用
	float GetHPRatio() const {
		return healthComponent_.GetHPRatio();
	}
	
	/// @brief 生存判定
	/// @return HP > 0 なら true
	/// @details ゲーム状態の判定に使用
	bool IsAlive() const {
		return healthComponent_.IsAlive();
	}
	
	/// @brief ダメージを受ける
	/// @param damage ダメージ量
	/// @details 敵の攻撃や衝突で呼び出される
	/// @note 敗北中は呼び出されても無視される
	void TakeDamage(int damage);
	
	/// @brief HP を回復する
	/// @param healAmount 回復量
	/// @details アイテムの使用時などに呼び出される
	/// @note 最大HP を超えて回復することはない
	void Heal(int healAmount);

	///--------------------------------------------------------------
	///                        衝突処理
	/// @brief 衝突開始時の処理
	/// @param other 衝突したオブジェクト
	/// @details 敵弾に衝突したときダメージを受ける
	/// @note 敗北中の衝突は無視される（二重ダメージ防止）
	void OnCollisionEnter(BaseObject *other) override;
	
	/// @brief 衝突継続中の処理
	/// @param other 衝突し続けているオブジェクト
	/// @details 現在は処理なし（将来の拡張用）
	void OnCollisionStay(BaseObject *other) override;
	
	/// @brief 衝突終了時の処理
	/// @param other 衝突が終わったオブジェクト
	/// @details 現在は処理なし（将来の拡張用）
	void OnCollisionExit(BaseObject *other) override;

	//========================================
	// 射撃・弾発射方向（HUD用）
	/// @brief 弾の発射方向を取得
	/// @return 正規化された発射方向ベクトル
	/// @details アシスト対象があれば偏向後の方向、無ければプレイヤーの前方方向
	Vector3 GetBulletFireDirection() const {
		return combatComponent_.GetBulletFireDirection();
	}
	
	/// @brief 実際の前方ベクトルを取得
	/// @return プレイヤーの機体方向ベクトル
	/// @details 画面上の矢印や照準描画に使用
	/// @note Euler角から前方ベクトルを算出
	Vector3 GetForwardVector() const {
		if (auto *transform = GetTransformSafe()) {
			return Vector3{
				sinf(transform->rotate.y) * cosf(transform->rotate.x),
				-sinf(transform->rotate.x),
				cosf(transform->rotate.y) * cosf(transform->rotate.x)};
		}
		return {0.0f, 0.0f, 1.0f};
	}

	//========================================
	// 武装設定（一元管理）
	/// @brief 武装設定構造体を読み込み専用で取得
	/// @return WeaponConfig への const 参照
	/// @details すべての武装パラメータにアクセス可能
	/// @note 変更する場合は GetWeaponConfigRef() を使用
	const WeaponConfig &GetWeaponConfig() const {
		return weaponConfig_;
	}

	/// @brief 武装設定構造体を取得（変更用）
	/// @return WeaponConfig への非 const 参照
	/// @details 複数のパラメータを一度に変更する場合に使用
	/// @warning 変更後は ApplyWeaponConfigToCombatComponent() を呼び出す必要がある
	WeaponConfig &GetWeaponConfigRef() {
		return weaponConfig_;
	}

	/// @brief 全ての武装設定をデフォルトに初期化
	/// @details PlayerConstants で定義されたデフォルト値に戻す
	/// @note 内部的に ApplyWeaponConfigToCombatComponent() も呼び出される
	void ResetWeaponConfig() {
		weaponConfig_ = WeaponConfig();
		ApplyWeaponConfigToCombatComponent();
	}

	/// @brief 武装設定をコンポーネントに適用
	/// @details WeaponConfig の値を各コンポーネントに反映させる
	/// @warning GetWeaponConfigRef() で複数項目を変更した場合、最後にこれを呼ぶ
	void ApplyWeaponConfigToCombatComponent() {
		combatComponent_.SetBulletModelPath(weaponConfig_.bulletModelPath);
		combatComponent_.SetMissileModelPath(weaponConfig_.missileModelPath);
		combatComponent_.SetMaxShootCoolTime(weaponConfig_.shootCoolTime);
		combatComponent_.SetMaxMissileAmmo(weaponConfig_.missileMaxAmmo);
		combatComponent_.SetMissileRecoveryTime(weaponConfig_.missileRecoveryTime);
	}

	//---- 弾（銃）の設定 ----
	/// @brief 弾のモデルパスを設定
	/// @param modelPath モデルファイルへの相対パス
	/// @details 次に弾を発射したときから新しいモデルが使用される
	/// @warning 不正なパスを指定するとアサート
	void SetBulletModelPath(const std::string &modelPath) {
		weaponConfig_.bulletModelPath = modelPath;
		combatComponent_.SetBulletModelPath(modelPath);
	}
	
	/// @brief 弾のモデルパスを取得
	/// @return 現在設定されているモデルパス
	const std::string &GetBulletModelPath() const {
		return weaponConfig_.bulletModelPath;
	}

	/// @brief 弾の飛行速度を設定
	/// @param speed 速度（units/秒）
	/// @details 大きいほど敵が回避しにくくなる。ただし画面外に素早く出ることもある
	/// @note bulletMaxLifeTime と組み合わせて有効距離を調整
	void SetBulletSpeed(float speed) {
		weaponConfig_.bulletSpeed = speed;
	}
	
	/// @brief 弾の飛行速度を取得
	/// @return 現在の弾速（units/秒）
	float GetBulletSpeed() const {
		return weaponConfig_.bulletSpeed;
	}

	/// @brief 弾の生存時間を設定
	/// @param lifeTime 生存時間（秒）
	/// @details この時間を超えた弾は自動削除される
	/// @note bulletSpeed × bulletMaxLifeTime = 弾が移動可能な最大距離
	void SetBulletMaxLifeTime(float lifeTime) {
		weaponConfig_.bulletMaxLifeTime = lifeTime;
	}
	
	/// @brief 弾の生存時間を取得
	/// @return 現在の生存時間制限（秒）
	float GetBulletMaxLifeTime() const {
		return weaponConfig_.bulletMaxLifeTime;
	}

	/// @brief 弾の当たり判定半径を設定
	/// @param radius 当たり判定球の半径（units）
	/// @details 敵の体積判定と交差判定。大きいほど当たりやすい
	/// @warning 見た目と乖離すると違和感が生じる
	void SetBulletRadius(float radius) {
		weaponConfig_.bulletRadius = radius;
	}
	
	/// @brief 弾の当たり判定半径を取得
	/// @return 現在の当たり判定半径（units）
	float GetBulletRadius() const {
		return weaponConfig_.bulletRadius;
	}

	/// @brief 連射クールタイムを設定
	/// @param coolTime 連射間隔（秒）
	/// @details この時間が経過するまで次の弾を発射できない
	/// @note 小さい値ほど高速連射。0.0 は瞬間発射（非推奨）
	void SetShootCoolTime(float coolTime) {
		weaponConfig_.shootCoolTime = coolTime;
		combatComponent_.SetMaxShootCoolTime(coolTime);
	}
	
	/// @brief 連射クールタイムを取得
	/// @return 現在の連射間隔（秒）
	float GetShootCoolTime() const {
		return weaponConfig_.shootCoolTime;
	}

	//---- ミサイルの設定 ----
	/// @brief ミサイルのモデルパスを設定
	/// @param modelPath モデルファイルへの相対パス
	/// @details 次に発射するミサイルから新しいモデルが使用される
	void SetMissileModelPath(const std::string &modelPath) {
		weaponConfig_.missileModelPath = modelPath;
		combatComponent_.SetMissileModelPath(modelPath);
	}
	
	/// @brief ミサイルのモデルパスを取得
	/// @return 現在設定されているミサイルモデルパス
	const std::string &GetMissileModelPath() const {
		return weaponConfig_.missileModelPath;
	}

	/// @brief ミサイルの飛行速度を設定
	/// @param speed 速度（units/秒）
	/// @details 弾より遅く設計。敵追尾のバランスポイント
	/// @note missileMaxTurnRate と組み合わせて追尾精度を調整
	void SetMissileSpeed(float speed) {
		weaponConfig_.missileSpeed = speed;
	}
	
	/// @brief ミサイルの飛行速度を取得
	/// @return 現在のミサイル速度（units/秒）
	float GetMissileSpeed() const {
		return weaponConfig_.missileSpeed;
	}

	/// @brief ミサイルの最大旋回速度を設定
	/// @param turnRate 旋回速度（度/秒）
	/// @details 大きいほど敵を素早く追う。小さすぎると敵を見失う
	/// @note missileSpeed が大きすぎると旋回に付いていけない
	void SetMissileMaxTurnRate(float turnRate) {
		weaponConfig_.missileMaxTurnRate = turnRate;
	}
	
	/// @brief ミサイルの最大旋回速度を取得
	/// @return 現在の旋回速度（度/秒）
	float GetMissileMaxTurnRate() const {
		return weaponConfig_.missileMaxTurnRate;
	}

	/// @brief ミサイルの生存時間を設定
	/// @param lifeTime 生存時間（秒）
	/// @details この時間内にターゲットに命中しなければ自動削除
	/// @warning 短すぎるとステージが広いときに敵が遠すぎて消滅する
	void SetMissileMaxLifeTime(float lifeTime) {
		weaponConfig_.missileMaxLifeTime = lifeTime;
	}
	
	/// @brief ミサイルの生存時間を取得
	/// @return 現在の生存時間制限（秒）
	float GetMissileMaxLifeTime() const {
		return weaponConfig_.missileMaxLifeTime;
	}

	/// @brief ミサイル1発の回復時間を設定
	/// @param recoveryTime 回復時間（秒）
	/// @details 1発のミサイル回復にかかる時間。小さいほど高速補充
	/// @note 総回復時間 = missileMaxAmmo × missileRecoveryTime
	void SetMissileRecoveryTime(float recoveryTime) {
		weaponConfig_.missileRecoveryTime = recoveryTime;
		combatComponent_.SetMissileRecoveryTime(recoveryTime);
	}
	
	/// @brief ミサイル1発の回復時間を取得
	/// @return 現在の回復時間（秒）
	float GetMissileRecoveryTime() const {
		return weaponConfig_.missileRecoveryTime;
	}

	/// @brief ミサイル最大残弾数を設定
	/// @param maxAmmo 最大残弾数
	/// @details 同時に保持できるミサイル数の上限
	/// @note 敵の同時ロック数を制限することでゲーム難易度を調整
	void SetMissileMaxAmmo(int maxAmmo) {
		weaponConfig_.missileMaxAmmo = maxAmmo;
		combatComponent_.SetMaxMissileAmmo(maxAmmo);
	}
	
	/// @brief ミサイル最大残弾数を取得
	/// @return 現在設定されている最大残弾数
	int GetMissileMaxAmmo() const {
		return weaponConfig_.missileMaxAmmo;
	}

	/// @brief ミサイル現在残弾数を取得
	/// @return 今この瞬間に発射可能な残弾数
	/// @details リアルタイムで更新される値
	int GetMissileAmmo() const {
		return combatComponent_.GetMissileAmmo();
	}

	/// @brief 次のミサイルが補充されるまでの経過時間を取得
	/// @return 0.0～GetMissileRecoveryTime() の経過時間
	float GetMissileRecoveryTimer() const {
		return combatComponent_.GetMissileRecoveryTimer();
	}

	///--------------------------------------------------------------
	///                        弾アシスト機能設定
	/// @brief 弾アシスト機能を有効/無効に設定
	/// @param enabled true で有効、false で無効
	/// @details 有効時、敵に対して自動的に照準が補正される
	/// @note 難易度調整やアクセシビリティ機能として使用
	void SetBulletAssistEnabled(bool enabled) {
		combatComponent_.SetBulletAssistEnabled(enabled);
	}

	/// @brief 弾アシストが有効か確認
	/// @return 有効なら true
	bool IsBulletAssistEnabled() const {
		return combatComponent_.IsBulletAssistEnabled();
	}

	/// @brief 弾アシスト視野角を設定
	/// @param fov 視野角（度数法）
	/// @details この角度内の敵のみアシスト対象。狭いほど難しくなる
	/// @note FOV = 30度 で前方30度の扇状範囲が対象
	void SetBulletAssistFOV(float fov) {
		combatComponent_.SetBulletAssistFOV(fov);
	}

	/// @brief 弾アシスト視野角を取得
	/// @return 現在の視野角（度数法）
	float GetBulletAssistFOV() const {
		return combatComponent_.GetBulletAssistFOV();
	}

	/// @brief 弾アシスト有効距離を設定
	/// @param range アシスト有効距離（units）
	/// @details この距離内の敵のみアシスト対象
	/// @note 遠すぎると常に敵が視野外になり、アシスト無効になる
	void SetBulletAssistRange(float range) {
		combatComponent_.SetBulletAssistRange(range);
	}

	/// @brief 弾アシスト有効距離を取得
	/// @return 現在の有効距離（units）
	float GetBulletAssistRange() const {
		return combatComponent_.GetBulletAssistRange();
	}

	/// @brief 弾アシスト強度を設定
	/// @param strength アシスト強度（0.0～1.0）
	/// @details 1.0 で完全自動狙い、0.0 で無補正。0.5 が弱いアシスト
	/// @note スキル上達を促すため、段階的に低下させるのが効果的
	void SetBulletAssistStrength(float strength) {
		combatComponent_.SetBulletAssistStrength(strength);
	}

	/// @brief 弾アシスト強度を取得
	/// @return 現在のアシスト強度（0.0～1.0）
	float GetBulletAssistStrength() const {
		return combatComponent_.GetBulletAssistStrength();
	}

	///--------------------------------------------------------------
	///                        内部処理（private）
private:
	/// @brief 移動更新処理
	/// @param deltaTime フレーム経過時間（秒）
	/// @details キー入力に基づいてプレイヤーを移動、画面外移動を制限
	/// @note ブースト中の速度上昇も含まれる
	void UpdateMovement(float deltaTime);
	
	/// @brief バレルロール・ブースト更新
	/// @param deltaTime フレーム経過時間（秒）
	/// @details 回避ロールとブースト機能の状態遷移と値の更新
	/// @note ブースト中は移動速度が上昇し、ゲージが消費される
	void UpdateBarrelRollAndBoost(float deltaTime);
	/// @brief スロー倍率を反映したゲームプレイ用コンポーネントを更新
	/// @details 移動、射撃、弾の更新を既存の依存順序で実行する
	/// @note 入力結果を同じフレームの射撃へ反映するため、呼び出し順序を変更しない
	void UpdateGameplayComponents(float deltaTime);
	/// @brief 敗北演出と3Dオブジェクトの状態を更新
	/// @details 敗北中は敗北演出、それ以外はBaseObjectの通常更新に位置更新を委譲する
	/// @note transformはUpdate()内でnull確認済みのobj_から取得した一時的な参照である
	void UpdateDefeatAndObject(MagMath::Transform *transform, float deltaTime);
	
	/// @brief 射撃処理
	/// @details ボタン入力に応じて弾またはミサイルを発射
	/// @note ロックオンモード中はミサイル発射のみ可能
	void ProcessShooting(float deltaTime);
	/// @brief 通常弾の入力判定と発射を処理
	/// @param playerPosition 発射位置
	/// @param shootDirection 発射方向
	/// @note ミサイルのロックオン状態に影響させないため、通常弾処理を分離する
	void ProcessBulletShooting(const MagMath::Vector3 &playerPosition, const MagMath::Vector3 &shootDirection);
	/// @brief ミサイルの押下、ロックオン、解放時の発射を処理
	/// @param playerPosition ロックオン判定の基準位置
	/// @param shootDirection ロックオン判定の基準方向
	/// @note prevMissileButtonPressed_を更新する唯一の処理として、毎フレーム一度だけ呼び出す
	void ProcessMissileShooting(const MagMath::Vector3 &playerPosition, const MagMath::Vector3 &shootDirection, float deltaTime);
	
	/// @brief Transform 安全取得
	/// @return obj_ が存在すれば Transform へのポインタ、無ければ nullptr
	/// @details null チェックを含むアクセス
	/// @note GetTransform() は無条件で transform にアクセスする場合の代替
	MagMath::Transform *GetTransformSafe() const;

	///--------------------------------------------------------------
	///                        メンバ変数
	//========================================
	// コア
	/// @brief プレイヤーの3Dオブジェクト
	/// @details モデル、位置、回転などの管理
	/// @note unique_ptr で自動管理
	std::unique_ptr<MagEngine::Object3d> obj_;
	
	/// @brief Object3d のセットアップ情報
	/// @details Initialize() で受け取ったポインタを保持
	/// @note 画像リソースの作成など、オブジェクト生成時に必要
	MagEngine::Object3dSetup *object3dSetup_;
	MagEngine::Input *input_;
	MagEngine::LineManager *lineManager_;

	//========================================
	// コンポーネント群
	/// @brief HP 管理コンポーネント
	/// @details ダメージ処理、回復処理、死亡判定を管理
	PlayerHealthComponent healthComponent_;
	
	/// @brief 射撃・ミサイル管理コンポーネント
	/// @details 弾の生成・更新・削除、ミサイルの自動追尾を管理
	/// @note 敵との当たり判定情報も提供
	PlayerCombatComponent combatComponent_;
	
	/// @brief 移動・バレルロール管理コンポーネント
	/// @details プレイヤーの移動速度、バレルロール状態、ブースト機能を管理
	PlayerMovementComponent movementComponent_;
	
	/// @brief ジャスト回避管理コンポーネント
	/// @details 敵弾検知、スロー演出、機体強化バフを管理
	/// @note スロー効果の適用は外部に委譲
	PlayerJustAvoidanceComponent justAvoidanceComponent_;
	
	/// @brief ロックオン管理コンポーネント
	/// @details 敵の検出、ロック対象の選択、照準の更新を管理
	PlayerLockedOnComponent lockedOnComponent_;
	
	/// @brief 敗北演出コンポーネント
	/// @details ノーズダイブと落下のアニメーションを管理
	PlayerDefeatComponent defeatComponent_;
	
	/// @brief ゲームオーバー演出
	/// @details 敗北後の画面遷移演出を管理
	GameOverAnimation gameOverAnimation_;

	//========================================
	// システム参照
	/// @brief 敵マネージャーへのポインタ
	/// @details ロックオンとミサイル追尾で全敵情報にアクセス
	/// @note nullptr の場合、ロックオンとミサイル機能は機能しない
	EnemyManager *enemyManager_;

	//========================================
	// ミサイルボタン長押し管理
	/// @brief ミサイルボタンが押されている累積時間（秒）
	/// @details ロックオンウィンドウを判定するための計時値
	/// @note 毎フレーム加算され、ボタン解放時にリセット
	float missileButtonHeldTime_;
	
	/// @brief ロックオンモード中フラグ
	/// @details true のとき、ミサイルボタンを押してもロックオン状態が継続
	/// @note ロックオン完了後、ボタン解放で false に戻る
	bool isInLockOnMode_;
	
	/// @brief 前フレームのミサイルボタン状態
	/// @details ボタン入力の立ち上がり検出に使用
	/// @note 状態遷移判定のため、毎フレーム更新される
	bool prevMissileButtonPressed_;

	//========================================
	// ジャスト回避演出管理
	/// @brief 今フレームでジャスト回避が成功したか
	/// @details true のとき、演出やサウンドをトリガーする（1フレームのみ有効）
	/// @note 次フレームで自動的に false にリセットされる
	bool justAvoidanceSuccessThisFrame_ = false;
	
	/// @brief 最後のジャスト回避成功率
	/// @details 0.0～1.0（1.0 で完璧なタイミング、0.0 で失敗）
	/// @note リプレイ時にスロー効果を再現する際に使用
	float lastJustAvoidanceSuccessRate_ = 0.0f;

	//========================================
	// 武装設定（一元管理）
	/// @brief 武装全設定を一元管理する構造体
	/// @details 弾とミサイルのすべてのパラメータを保持
	/// @note ゲーム難易度調整時に全武装設定を一括変更する際に使用
	WeaponConfig weaponConfig_;

	//========================================
	// Friend クラス
	friend class FollowCamera;
};

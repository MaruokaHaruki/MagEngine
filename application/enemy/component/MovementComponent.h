/*********************************************************************
 * \file   MovementComponent.h
 * \brief  移動制御・速度管理コンポーネント
 *
 * \author Harukichimaru
 * \date   April 2026
 *********************************************************************/
#pragma once
#include "IEnemyComponent.h"
#include "../../../engine/math/MagMath.h"
using namespace MagMath;

/**
 * @brief 敵の移動制御を担当するコンポーネント
 *
 * 責務：
 * - 目標位置への移動ロジック
 * - 速度・加速度の管理
 * - ターン速度（回転速度）の管理
 * - 周回パターン（オービット）の実装
 */
class MovementComponent : public IEnemyComponent {
public:
	virtual ~MovementComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 *   - baseSpeed (float): 基本移動速度
	 *   - acceleration (float): 加速度
	 *   - turnSpeed (float): ターン速度（度/秒）
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	void Initialize(const ComponentConfig& config, Enemy* owner) override;

	/**
	 * @brief 毎フレーム更新（速度減衰、移動更新）
	 */
	void Update(float deltaTime) override;

	/**
	 * @brief ImGui デバッグ表示
	 */
	void DrawImGui() override;

	/**
	 * @brief コンポーネント名取得
	 */
	std::string GetComponentName() const override { return "MovementComponent"; }

	//========================================
	// 入出力インターフェース
	//========================================

	/**
	 * @brief 指定位置へ移動（直線移動）
	 * @param target 目標位置
	 * @param speedMultiplier 速度乗数（デフォルト 1.0）
	 */
	void MoveTo(const Vector3& target, float speedMultiplier = 1.0f);

	/**
	 * @brief 周回移動（オービット）
	 * @param center 周回中心
	 * @param radius 周回半径
	 * @param direction 周回方向（1.0 = 右回り、-1.0 = 左回り）
	 */
	void Orbit(const Vector3& center, float radius, float direction = 1.0f);

	/**
	 * @brief 現在の速度を取得
	 */
	Vector3 GetVelocity() const { return velocity_; }

	/**
	 * @brief 速度を設定
	 */
	void SetVelocity(const Vector3& vel) { velocity_ = vel; }

	/**
	 * @brief 基本移動速度を取得
	 */
	float GetBaseSpeed() const { return baseSpeed_; }

	/**
	 * @brief 基本移動速度を設定
	 */
	void SetBaseSpeed(float speed) { baseSpeed_ = speed; }

	/**
	 * @brief 加速度を設定
	 */
	void SetAcceleration(float accel) { acceleration_ = accel; }

	/**
	 * @brief 移動を停止
	 */
	void StopMovement() { velocity_ = Vector3(0, 0, 0); }

	/**
	 * @brief オービット状態かどうか
	 */
	bool IsOrbiting() const { return isOrbiting_; }

private:
	//========================================
	// メンバ変数
	//========================================
	Vector3 velocity_;           ///< 現在の速度
	float baseSpeed_;            ///< 基本移動速度
	float acceleration_;         ///< 加速度
	float turnSpeed_;            ///< ターン速度（度/秒）
	float velocityDamping_;      ///< 速度減衰係数

	// 直線移動関連
	Vector3 moveTarget_;         ///< 移動目標位置
	bool isMoving_;              ///< 移動中フラグ

	// 周回移動関連
	Vector3 orbitCenter_;        ///< 周回中心
	float orbitRadius_;          ///< 周回半径
	float orbitDirection_;       ///< 周回方向
	bool isOrbiting_;            ///< 周回中フラグ
	float orbitAngle_;           ///< 現在の周回角度
};

/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#pragma once
#include "Input.h" // Input処理のために追加
#include "Object3d.h"
#include "PlayerBullet.h"
#include <memory> // For std::unique_ptr
#include <string> // For std::string
#include <vector> // For std::vector

// Forward declarations
class Object3d;
class Object3dSetup;

class Player {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// @brief ImGui描画
	void DrawImGui();

	/// \brief 弾の描画
	void DrawBullets();

	/// \brief 弾のリストを取得
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return bullets_;
	}

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief プレイヤーの動作関係処理（入力処理、移動、回転を統合）
	void UpdateMovement();

	/// \brief 入力に基づいて目標速度と目標回転を設定
	void ProcessMovementInput(bool pressW, bool pressS, bool pressA, bool pressD);

	/// \brief 現在の速度を目標速度に向けて更新
	void UpdateVelocity();

	/// \brief 位置を速度に基づいて更新
	void UpdatePosition();

	/// \brief 回転（傾き）を更新
	void UpdateRotation();

	/// \brief 弾の発射処理
	void ProcessShooting();

	/// \brief 弾の更新・削除処理
	void UpdateBullets();

	///--------------------------------------------------------------
	///							入出力関数
public:
	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}

public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	std::unique_ptr<Object3d> obj_; // プレイヤーの3Dオブジェクト

	// 移動関連
	Vector3 currentVelocity_;
	Vector3 targetVelocity_;
	float moveSpeed_;
	float acceleration_; // 加速/減速の速さ (補間係数として使用)

	// 回転（傾き）関連
	Vector3 targetRotationEuler_; // 目標の傾き（オイラー角）
	float rollSpeed_;			  // ロール（Z軸回転）の速さ
	float pitchSpeed_;			  // ピッチ（X軸回転）の速さ
	float rotationSmoothing_;	  // 傾きの滑らかさ (補間係数として使用)
	float maxRollAngle_;		  // 最大ロール角
	float maxPitchAngle_;		  // 最大ピッチ角

	// 弾関連
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	Object3dSetup *object3dSetup_; // 弾の初期化用
	float shootCoolTime_;		   // 射撃のクールタイム
	float maxShootCoolTime_;	   // 最大クールタイム
};

/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#pragma once
#include "Object3d.h"
#include <memory> // For std::unique_ptr
#include <string> // For std::string

// #include "Input.h" // Inputクラスの直接呼び出しを避けるため削除

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
	void Update(float deltaTime, bool pressW, bool pressS, bool pressA, bool pressD);

	/// \brief 描画
	void Draw();

	/// @brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
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
};

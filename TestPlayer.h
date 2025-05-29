/*********************************************************************
 * \file   TestPlayer.h
 * \brief
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#pragma once
#include "Vector2.h"
#include <chrono>

class TestPlayer {

	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize();

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief デルタタイムの計算
	void CalculateDeltaTime();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief 目標FPSの設定
	void SetTargetFPS(float fps) {
		targetFPS_ = fps;
	}

	/// \brief 現在のFPSの取得
	float GetCurrentFPS() const {
		return currentFPS_;
	}

	/// \brief 位置の設定
	void SetPosition(const Vector2 &position) {
		position_ = position;
	}

	/// \brief 位置の取得
	Vector2 GetPosition() const {
		return position_;
	}

	/// \brief 地面に接触しているかの取得
	bool IsGrounded() const {
		return isGrounded_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	Vector2 position_; // プレイヤーの位置
	Vector2 velocity_; // プレイヤーの速度
	Vector2 size_;	   // ボックスのサイズ
	float speed_;	   // 移動速度

	// ジャンプ・重力関連
	float jumpPower_;		 // ジャンプ力
	float gravity_;			 // 重力の強さ
	float verticalVelocity_; // 垂直方向の速度
	bool isGrounded_;		 // 地面に接触しているか
	float groundLevel_;		 // 地面のレベル（Y座標）

	// FPS関連
	float targetFPS_;											   // 目標FPS
	float deltaTime_;											   // デルタタイム
	std::chrono::high_resolution_clock::time_point lastFrameTime_; // 前フレームの時間
	float updateAccumulator_;									   // 更新間隔制御用アキュムレータ

	// デバッグ用
	float currentFPS_; // 現在のFPS
	int frameCount_;   // フレームカウント
	float fpsTimer_;   // FPS計算用タイマー
};

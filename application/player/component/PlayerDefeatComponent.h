/*********************************************************************
 * \file   PlayerDefeatComponent.h
 * \brief  プレイヤー敗北演出コンポーネント
 *
 * \author Harukichimaru
 * \date   February 2026
 * \note   敗北時のアニメーション演出・落下処理・回転処理を管理
 *
 * 単一責任：敗北演出のアニメーション制御
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Vector3.h"

///=============================================================================
///                プレイヤー敗北演出管理コンポーネント
class PlayerDefeatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 初期化
	void Initialize();

	/// @brief 敗北演出開始
	void StartDefeatAnimation();

	/// @brief 敗北演出更新
	/// @param transform 更新対象になるTransform
	/// @param deltaTime フレーム時間
	void Update(MagMath::Transform *transform, float deltaTime);

	///--------------------------------------------------------------
	///                        状態取得
	/// @brief 敗北フラグ取得
	bool IsDefeated() const {
		return isDefeated_;
	}

	/// @brief 敗北演出完了判定
	bool IsDefeatAnimationComplete() const {
		return defeatAnimationComplete_;
	}

	/// @brief 敗北演出進行度取得（0.0 ～ 1.0）
	float GetAnimationProgress() const {
		return animationProgress_;
	}

	///--------------------------------------------------------------
	///                        設定変更
	/// @brief 敗北演出の時間を設定
	void SetAnimationDuration(float duration) {
		animationDuration_ = duration;
	}

	/// @brief 落下加速度を設定（重力値）
	void SetGravity(float gravity) {
		gravity_ = gravity;
	}

	/// @brief 終了判定の高さを設定（この高さより下がったら完了）
	void SetDeadHeight(float height) {
		deadHeight_ = height;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	/// @brief 敗北演出のアニメーション更新
	void UpdateAnimation(MagMath::Transform *transform, float deltaTime);

	/// @brief 疑似ランダム値生成（シード値から）
	float GeneratePseudoRandom(unsigned int seed, int multiplier, int divisor);

	///--------------------------------------------------------------
	///                        メンバ変数
	bool isDefeated_;			   // 敗北フラグ
	bool defeatAnimationComplete_; // 敗北演出完了フラグ
	float defeatAnimationTime_;	   // 敗北演出経過時間
	float animationDuration_;	   // 敗北演出全体時間

	// 物理パラメータ
	Vector3 defeatVelocity_;	  // 敗北時の速度（横移動・落下）
	Vector3 defeatRotationSpeed_; // 敗北時の回転速度
	Vector3 localRotation_;		  // ローカル座標系での回転（世界座標の傾きを防ぐ）
	float gravity_;				  // 重力加速度
	float deadHeight_;			  // 終了判定高さ

	// 演出進行度
	float animationProgress_; // 0.0 ～ 1.0
};

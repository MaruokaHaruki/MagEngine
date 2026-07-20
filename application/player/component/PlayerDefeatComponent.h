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
using Vector3 = MagMath::Vector3;
#include "Vector3.h"
#include <random>

///=============================================================================
///                プレイヤー敗北演出管理コンポーネント
class PlayerDefeatComponent {
public:
	///--------------------------------------------------------------
	///                        メンバ関数
	/// @brief 敗北演出の状態、物理パラメータ、進行度を初期化する
	void Initialize();

	/// @brief 敗北フラグを立て、落下・回転演出を開始する
	/// @note すでに敗北中の場合は状態を上書きしない。
	void StartDefeatAnimation();

	/// @brief 敗北中の位置・姿勢と演出完了状態を更新する
	/// @param transform 落下・回転を反映するTransform。nullptrの場合は更新しない。
	/// @param deltaTime 前フレームからの経過時間（秒）
	void Update(MagMath::Transform *transform, float deltaTime);

	///--------------------------------------------------------------
	///                        状態取得
	/// @brief 敗北演出が開始済みかを取得する
	/// @return 敗北中または演出済みの場合はtrue、それ以外はfalse
	bool IsDefeated() const {
		return isDefeated_;
	}

	/// @brief 敗北演出の終了条件を満たしたかを取得する
	/// @return 演出完了の場合はtrue、それ以外はfalse
	bool IsDefeatAnimationComplete() const {
		return defeatAnimationComplete_;
	}

	/// @brief 敗北演出の時間進行度を取得する
	/// @return 0.0から1.0にクランプされた進行度
	float GetAnimationProgress() const {
		return animationProgress_;
	}

	///--------------------------------------------------------------
	///                        設定変更
	/// @brief 敗北演出の時間上限を設定する
	/// @param duration 演出の総時間（秒）
	void SetAnimationDuration(float duration) {
		animationDuration_ = duration;
	}

	/// @brief 落下に使用する重力加速度を設定する
	/// @param gravity 下向き加速に使用する値
	void SetGravity(float gravity) {
		gravity_ = gravity;
	}

	/// @brief 演出完了と判定するワールド高さを設定する
	/// @param height TransformのY座標がこの値以下になると演出を完了する
	void SetDeadHeight(float height) {
		deadHeight_ = height;
	}

private:
	///--------------------------------------------------------------
	///                        内部処理
	/// @brief 時間進行に応じて落下・回転をTransformへ反映する
	/// @param transform 更新対象のTransform
	/// @param deltaTime 前フレームからの経過時間（秒）
	void UpdateAnimation(MagMath::Transform *transform, float deltaTime);

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
	// 敗北開始ごとの揺らぎを保持し、rand()の共有状態による他システムへの副作用を避ける。
	// 乱数生成器はメンバとして所有し、演出開始のたびに再初期化しない。
	std::mt19937 randomEngine_{std::random_device{}()};
};

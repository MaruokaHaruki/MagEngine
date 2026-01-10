#pragma once
#include "MagMath.h"
using namespace MagMath;
#include <string>

// Forward declarations
class Camera;
class Player;

class FollowCamera {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(const std::string &cameraName);

	/// \brief 更新
	void Update();

	/// \brief ターゲットの設定
	void SetTarget(Player *target);

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/// \brief カメラの位置と回転を計算
	void UpdateCameraTransform();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief オフセットの設定
	void SetOffset(const Vector3 &offset) {
		offset_ = offset;
	}

	/// \brief 追従の滑らかさを設定
	void SetSmoothness(float positionSmoothness, float rotationSmoothness) {
		positionSmoothness_ = positionSmoothness;
		rotationSmoothness_ = rotationSmoothness;
	}

	/// \brief 固定位置モードの設定
	void SetFixedPositionMode(bool enable) {
		isFixedPositionMode_ = enable;
	}

	/// \brief 固定位置の設定
	void SetFixedPosition(const Vector3 &position) {
		fixedPosition_ = position;
		isFixedPositionMode_ = true;
	}

	/// \brief 現在の位置を固定位置として設定
	void SetCurrentPositionAsFixed() {
		fixedPosition_ = currentPosition_;
		isFixedPositionMode_ = true;
	}

	/// \brief カメラの傾き追従を有効化/無効化
	void SetEnableRollFollow(bool enable) {
		enableRollFollow_ = enable;
	}

	/// \brief 使用中のカメラを取得
	Camera *GetCamera() const {
		return camera_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	Camera *camera_;		 // カメラマネージャから取得したカメラ
	Player *target_;		 // 追従対象のプレイヤー
	std::string cameraName_; // 使用するカメラ名

	// 追従パラメータ
	Vector3 offset_;		   // プレイヤーからのオフセット
	float positionSmoothness_; // 位置の滑らかさ (0.0f-1.0f)
	float rotationSmoothness_; // 回転の滑らかさ (0.0f-1.0f)

	// 墜落時の追従パラメータ
	float crashRotationSmoothness_; // 墜落時の回転の滑らかさ
	bool limitCrashRotation_;		// 墜落時の回転制限フラグ

	// 固定位置モード
	bool isFixedPositionMode_; // 固定位置モードのフラグ
	Vector3 fixedPosition_;	   // 固定位置

	// カメラの傾き追従
	bool enableRollFollow_; // カメラの傾き（ロール）追従を有効化するか

	// 内部状態
	Vector3 currentPosition_; // 現在のカメラ位置
	Vector3 currentRotation_; // 現在のカメラ回転
	Vector3 targetPosition_;  // 目標カメラ位置
	Vector3 targetRotation_;  // 目標カメラ回転
};

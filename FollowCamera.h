#pragma once
#include "Vector3.h"
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

	// 内部状態
	Vector3 currentPosition_; // 現在のカメラ位置
	Vector3 currentRotation_; // 現在のカメラ回転
	Vector3 targetPosition_;  // 目標カメラ位置
	Vector3 targetRotation_;  // 目標カメラ回転
};

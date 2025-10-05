#pragma once
#include "Vector3.h"
#include <string>

//========================================
// 前方宣言
class Camera;
class Player;

class TitleCamera {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(const std::string &cameraName);

	/// \brief 更新
	void Update();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:

	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	Camera *camera_;		 // カメラマネージャから取得したカメラ
	Player *target_;		 // 追従対象のプレイヤー
	std::string cameraName_; // 使用するカメラ名
};

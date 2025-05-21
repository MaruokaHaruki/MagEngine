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

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	std::unique_ptr<Object3d> obj_; // プレイヤーの3Dオブジェクト
};

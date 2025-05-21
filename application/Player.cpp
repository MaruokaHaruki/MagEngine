/*********************************************************************
 * \file   Player.cpp
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#include "Player.h"
#include "ImguiSetup.h"
#include "Input.h"
#include "ModelManager.h" // SetModelのために必要に応じて
#include "Object3d.h"	  // Object3dクラスの定義をインクルード

///=============================================================================
///						初期化
void Player::Initialize(Object3dSetup *object3dSetup, const std::string &modelPath) {
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize(object3dSetup);
	obj_->SetModel(modelPath);
}

///=============================================================================
///						更新
void Player::Update() {
	if (obj_) {
		obj_->Update();
	}
}

///=============================================================================
///                     描画
void Player::Draw() {
	if (obj_) {
		obj_->Draw();
	}
}

///=============================================================================
///						ImGui描画
void Player::DrawImGui() {
	if (obj_) {
		// 必要に応じて obj_->DrawImGui(); を呼び出す
		// Object3dクラスにDrawImGuiメンバーがある場合
	}
}

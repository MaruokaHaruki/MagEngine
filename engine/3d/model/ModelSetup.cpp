/*********************************************************************
 * \file   ModelSetup.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "ModelSetup.h"
#include "TextureManager.h"

///=============================================================================
///						初期化
void ModelSetup::Initialize(DirectXCore *dxCore) {
	//========================================
	// 引数で受け取ったDXCoreをセット
	dxCore_ = dxCore;
}

///=============================================================================
///						環境マップテクスチャの設定
void ModelSetup::SetEnvironmentTexture(const std::string &texturePath) {
	//========================================
	// テクスチャパスを設定
	environmentTexturePath_ = texturePath;

	//========================================
	// テクスチャの読み込み
	if (!texturePath.empty()) {
		TextureManager::GetInstance()->LoadTexture(texturePath);
	}
}
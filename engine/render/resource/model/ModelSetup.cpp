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
 ///                        namespace MagEngine
namespace MagEngine {
///=============================================================================
///						初期化
	void ModelSetup::Initialize(DirectXCore *dxCore, TextureManager &textureManager) {
		//========================================
		// 引数で受け取ったDXCoreをセット
		dxCore_ = dxCore;
		// モデルはテクスチャを所有せず、Framework所有の管理器を参照する
		textureManager_ = &textureManager;
	}

	///=============================================================================
	///						環境マップテクスチャの設定
	void ModelSetup::SetEnvironmentTexture(const std::string &texturePath) {
		//========================================
		// テクスチャパスを設定
		environmentTexturePath_ = texturePath;

		//========================================
		// テクスチャの読み込み
		if(!texturePath.empty()) {
			textureManager_->LoadTexture(texturePath);
		}
	}
}

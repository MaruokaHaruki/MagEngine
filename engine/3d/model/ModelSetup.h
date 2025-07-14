/*********************************************************************
 * \file   ModelSetup.h
 * \brief  モデル共通部クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "DirectXCore.h"
#include <string>

///=============================================================================
///						モデル共通部クラス
class ModelSetup {

	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(DirectXCore *dxCore);

	/// \brief 環境マップテクスチャの設定
	void SetEnvironmentTexture(const std::string &texturePath);

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	/**----------------------------------------------------------------------------
	 * \brief  GetDXManager DirectXCore取得
	 * \return
	 * \note
	 */
	DirectXCore *GetDXManager() const {
		return dxCore_;
	}

	/**----------------------------------------------------------------------------
	 * \brief  GetEnvironmentTexture 環境マップテクスチャパス取得
	 * \return
	 * \note
	 */
	const std::string &GetEnvironmentTexture() const {
		return environmentTexturePath_;
	}

	/**----------------------------------------------------------------------------
	 * \brief  HasEnvironmentTexture 環境マップテクスチャが設定されているか
	 * \return
	 * \note
	 */
	bool HasEnvironmentTexture() const {
		return !environmentTexturePath_.empty();
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//---------------------------------------
	// DirectXCoreポインタ
	DirectXCore *dxCore_ = nullptr;

	//---------------------------------------
	// 環境マップテクスチャパス
	std::string environmentTexturePath_;
};

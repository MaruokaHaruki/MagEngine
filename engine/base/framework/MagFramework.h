/*********************************************************************
 * \file   MagFramework .h
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#pragma once
//========================================
// Framework
#include "Camera.h"
#include "DirectXCore.h"
#include "ImguiSetup.h"
#include "Input.h"
#include "SrvSetup.h"
#include "WinApp.h"
// Manager
#include "CameraManager.h"
#include "DebugTextManager.h"
#include "LightManager.h"
#include "LineManager.h"
#include "MAudioG.h"
#include "ModelManager.h"
#include "SceneFactory.h"
#include "SceneManager.h"
#include "TextureManager.h"
// Setup
#include "Object3dSetup.h"
#include "ParticleSetup.h"
#include "SkyboxSetup.h"
#include "SpriteSetup.h"

///=============================================================================
///						FrameWorkクラス
class MagFramework {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 仮想デストラクタ
	virtual ~MagFramework() = default;

	/// \brief メインループ
	void Run();

	/// \brief 初期化
	virtual void Initialize();

	/// \brief 更新
	virtual void Update();

	/// \brief 描画
	virtual void Draw() = 0;

	/// \brief 終了処理
	virtual void Finalize();

	///--------------------------------------------------------------
	///						 静的メンバ関数
public:
	/// @brief レンダーテクスチャ前処理
	void RenderPreDraw();

	/// @biref レンダーテクスチャ後処理
	void RenderPostDraw();

	/// @brief フレームワーク共通前処理
	void PreDraw();

	/// @brief フレームワーク共通後処理
	void PostDraw();

	/// @brief ImGuiの更新前処理
	void ImGuiPreDraw();

	/// @brief ImGuiの更新後処理
	void ImGuiPostDraw();

	/// @brief オブジェクト2D共通描画設定
	void Object2DCommonDraw();

	/// @brief パーティクル共通描画設定
	void ParticleCommonDraw();

	/// @brief オブジェクト3D共通描画設定
	void Object3DCommonDraw();

	/// @brief Skybox共通描画設定
	void SkyboxCommonDraw();

	///--------------------------------------------------------------
	///							入出力関数
public:
	/// \brief 終了リクエストの取得
	virtual bool IsEndRequest() const {
		return isEndRequest_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
protected:
	//========================================
	// ゲーム終了フラグ
	bool isEndRequest_ = false;
	//========================================
	// ウィンドウクラス
	std::unique_ptr<WinApp> win_;
	//========================================
	// ダイレクトX
	std::unique_ptr<DirectXCore> dxCore_;
	//========================================
	// ImGui
	std::unique_ptr<ImguiSetup> imguiSetup_;
	//========================================
	// SrvSetup
	std::unique_ptr<SrvSetup> srvSetup_;
	//========================================
	// 共通部
	// スプライトセットアップ
	std::unique_ptr<SpriteSetup> spriteSetup_;
	// パーティクルセットアップ
	std::unique_ptr<ParticleSetup> particleSetup_;
	// 3Dオブジェクセットアップ
	std::unique_ptr<Object3dSetup> object3dSetup_;
	// モデルセットアップ
	std::unique_ptr<ModelSetup> modelSetup_;
	// Skyboxセットアップ
	std::unique_ptr<SkyboxSetup> skyboxSetup_;
	//========================================
	// マネージャ
	// シーンマネージャ
	std::unique_ptr<SceneManager> sceneManager_;
	// シーンファクトリー
	std::unique_ptr<SceneFactory> sceneFactory_;
	// ライトマネージャ
	std::unique_ptr<LightManager> lightManager_;
};

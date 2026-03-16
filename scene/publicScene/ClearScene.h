/*********************************************************************
 * \file   ClearScene.h
 * \brief  クリアシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "BaseScene.h"

// Forward declaration
class SceneContext;

///=============================================================================
///                         クリアシーンクラス
class ClearScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(SceneContext *context) override;

	/// \brief 終了処理
	void Finalize() override;

	/// \brief 更新
	void Update() override;

	/// @brie 2D描画
	void Object2DDraw() override;

	/// \brief 3D描画
	void Object3DDraw() override;

	/// \brief パーティクル描画
	void ParticleDraw() override;

	/// \brief Skybox描画
	void SkyboxDraw() override;

	/// \brief Cloud描画
	void CloudDraw() override;

	/// \brief TrailEffect描画
	void TrailEffectDraw() override;

	/// \brief ImGui描画
	void ImGuiDraw() override;

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	///--------------------------------------------------------------
	///							メンバ変数
private:
};

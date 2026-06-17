/*********************************************************************
 * \file   SceneManager.h
 * \brief  シーン管理クラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップを統一管理
 *         NOTE: シーンの遷移と生成を管理する
 *********************************************************************/
#pragma once
#include "AbstractSceneFactory.h"
#include "BaseScene.h"
#include "EngineContext.h"
#include "SceneContext.h"
#include "TrailEffectManager.h"
#include <memory>

///=============================================================================
///                         シーンマネージャ
/// NOTE: SceneContextを内部で管理し、シーンに統合して渡す
class SceneManager {
	///----------------------------------s----------------------------
	///                            メンバ関数
public:
	/// \brief 初期化
	void Initialize(const MagEngine::EngineContext &engineContext);

	/// @brief 終了処理
	void Finalize();

	/// @brief 更新処理
	void Update();

	/// @brief Object2D描画
	void Object2DDraw();

	/// @brief Object3D描画
	void Object3DDraw();

	/// @brief Particle描画
	void ParticleDraw();

	/// @brief Skybox描画
	void SkyboxDraw();

	/// @brief Cloud描画
	void CloudDraw();

	/// @brief TrailEffect描画
	void TrailEffectDraw();

	/// @brief ImGui描画
	void ImGuiDraw();

	/// @brief シーンファクトリーのSetter
	void SetSceneFactory(AbstractSceneFactory *sceneFactory) {
		sceneFactory_ = sceneFactory;
	}

	///--------------------------------------------------------------
	///                            メンバ変数
private:
	//========================================
	// シーンファクトリーポインタ
	AbstractSceneFactory *sceneFactory_ = nullptr;

	//========================================
	// シーンコンテキスト - NOTE: セットアップ類をここで統一管理
	SceneContext sceneContext_;

	// EngineContext - NOTE: 実体はMagFrameworkが保持し、SceneManagerは非所有参照だけをSceneへ渡す
	const MagEngine::EngineContext *engineContext_ = nullptr;

	//========================================
	// 今のシーン
	std::unique_ptr<BaseScene> nowScene_;
	// 次のシーン
	std::unique_ptr<BaseScene> nextScene_;

	//========================================
	// 現在のシーン番号　
	int currentSceneNo_ = 0;
	// 前のシーン番号
	int prevSceneNo_ = -1;

};

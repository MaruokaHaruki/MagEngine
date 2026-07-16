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
#include <string>

namespace MagEngine {
	class RenderWorld;
}

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
	void Update(const FrameTime &frameTime);

	/// @brief 描画対象の登録
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// @brief シーンファクトリーのSetter
	void SetSceneFactory(AbstractSceneFactory *sceneFactory) {
		sceneFactory_ = sceneFactory;
	}

	/// \brief 直近のScene初期化失敗理由を取得
	const std::string &GetLastSceneInitializationError() const {
		return lastSceneInitializationError_;
	}

	///--------------------------------------------------------------
	///                            メンバ変数
private:
	bool CreateAndInitializeCandidateScene(int sceneNo, std::unique_ptr<BaseScene> &candidateScene, std::string &errorMessage);
	void DiscardCandidateScene(std::unique_ptr<BaseScene> &candidateScene);
	void HandleSceneChangeFailure(int requestedSceneNo, const std::string &errorMessage);
	static const char *GetSceneName(int sceneNo);

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

	// NOTE: Releaseでも失敗原因を追跡できるよう、次の遷移成功まで保持する。
	std::string lastSceneInitializationError_;

};

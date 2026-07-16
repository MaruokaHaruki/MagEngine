/*********************************************************************
 * \file   IScene.h
 * \brief`
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#pragma once

#include "engine/base/time/FrameTime.h"
#include <string_view>

class SceneContext;
namespace MagEngine {
	struct EngineContext;
	class RenderWorld;
}

// シーンの種類
enum SCENE {
	DEBUG,
	TITLE,
	GAMEPLAY,
	CLEAR
};

///=============================================================================
///                         インターフェースシーン
/// NOTE: このクラスはすべてのシーンの基底クラス
///       派生Scene固有の依存を持たせないことでinclude波及を抑える
class BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
	// NOTE: 継承先で実装される関数。抽象クラスなので純粋仮想関数とする。
public:
	/// \brief 初期化
	/// \param engineContext Sceneが使用するEngineサービス
	/// \param sceneContext Scene管理用コンテキスト
	virtual void Initialize(const MagEngine::EngineContext &engineContext, SceneContext &sceneContext) = 0;

	/// \brief 終了処理
	virtual void Finalize() = 0;

	/// \brief 更新
	virtual void Update(const FrameTime &frameTime) = 0;

	/// \brief 描画対象の登録
	virtual void RegisterRenderables(MagEngine::RenderWorld &renderWorld) = 0;

	/// \brief Scene切替失敗時の復帰通知
	/// \note 遷移元Sceneだけが、自身の演出や入力状態を安全な状態へ戻すために使用する。
	virtual void OnSceneChangeFailed(int requestedSceneNo, std::string_view errorMessage) {
		(void)requestedSceneNo;
		(void)errorMessage;
	}

#ifdef _DEBUG
	/// \brief Scene固有Editor Panelの再登録
	/// \note Scene切替の確定後にのみ登録し、候補Sceneの破棄後へラムダを残さない。
	virtual void RegisterEditorPanels() {}
#endif

	/**----------------------------------------------------------------------------
	 * \brief  ~BaseScene 抽象クラスのデストラクタ
	 * NOTE: 仮想デストラクタを用意することで、継承先のクラスのデストラクタが呼ばれるようにする。
	 */
	virtual ~BaseScene() = default;

	/**----------------------------------------------------------------------------
	 * \brief  GetSceneNo シーン番号を取得する
	 * \note   NOTE: SceneManagerで次のシーン番号を管理する
	 * \return 次のシーン番号
	 */
	int GetSceneNo() const {
		return nextSceneNo_;
	}

	/**----------------------------------------------------------------------------
	 * \brief  SetSceneNo 次のシーン番号を設定する
	 * \param  sceneNo 次のシーン番号
	 * \note   NOTE: シーンから次のシーン番号を設定するために使用
	 * \return 設定されたシーン番号
	 */
	int SetSceneNo(int sceneNo) {
		return nextSceneNo_ = sceneNo;
	}

	///--------------------------------------------------------------
	///                            メンバ変数
protected:
	// NOTE: 各シーンインスタンスは次のシーン番号を保持する
	//       これによりSceneManagerが複雑な管理をしなくて済む
	int nextSceneNo_ = -1;
};

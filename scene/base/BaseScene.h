/*********************************************************************
 * \file   IScene.h
 * \brief`
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#pragma once
//========================================
// NOTE: SceneContextを使用してセットアップ類をまとめている
//       これにより各シーンのInitialize引数を削減
#include "SceneContext.h"

//========================================
// 3D系
// Object3d
#include "Object3d.h"
#include "Object3dSetup.h"
// Particle
#include "Particle.h"
#include "ParticleEmitter.h"
#include "ParticleSetup.h"
// Skybox
#include "Skybox.h"
#include "SkyboxSetup.h"
// Cloud
#include "Cloud.h"
#include "CloudSetup.h"
// TrailEffect
#include "TrailEffectManager.h"
#include "TrailEffectSetup.h"
// ========================================
// 2D系
#include "Sprite.h"
#include "SpriteSetup.h"
// ========================================
// その他
// オーディオ
#include "MAudioG.h"
// カメラ
#include "Camera.h"
// カメラマネージャ
#include "CameraManager.h"
// ラインマネージャ
#include "LineManager.h"
// データローダー
#include "LevelDataLoader.h"
// Input
#include "Input.h"

// ========================================
// デバック関係
#include "DebugTextManager.h"

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
///       SceneContextを通じて共通リソースにアクセスする
class BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
	// NOTE: 継承先で実装される関数。抽象クラスなので純粋仮想関数とする。
public:
	/// \brief 初期化 - NOTE: 引数をSceneContextの1つにまとめられた
	/// \param context シーンが使用するすべてのセットアップを含むコンテキスト
	virtual void Initialize(SceneContext *context) = 0;

	/// \brief 終了処理
	virtual void Finalize() = 0;

	/// \brief 更新
	virtual void Update() = 0;

	/// \brief 2D描画
	virtual void Object2DDraw() = 0;

	/// \brief 3D描画
	virtual void Object3DDraw() = 0;

	/// \brief パーティクル描画
	virtual void ParticleDraw() = 0;

	/// \brief Skybox描画
	virtual void SkyboxDraw() = 0;

	/// \brief ImGui描画
	virtual void ImGuiDraw() = 0;

	/// \brief Cloud描画
	virtual void CloudDraw() = 0;

	/// \brief TrailEffect描画
	virtual void TrailEffectDraw() = 0;

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

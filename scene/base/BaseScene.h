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
///						インターフェースシーン
class BaseScene {
	///--------------------------------------------------------------
	///							メンバ関数
	// NOTE:継承先で実装される関数。抽象クラスなので純粋仮想関数とする。
public:
	/// \brief 初期化
	virtual void Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup, SkyboxSetup *skyboxSetup) = 0;

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

	/**----------------------------------------------------------------------------
	 * \brief  ~IScene 抽象クラスのデストラクタ
	 * NOTE: 仮想デストラクタを用意することで、継承先のクラスのデストラクタが呼ばれるようにする。
	 */
	virtual ~BaseScene() = default;

	/**----------------------------------------------------------------------------
	 * \brief  GetSceneNo シーン番号を取得する
	 * \return
	 */
	int GetSceneNo() {
		return sceneNo;
	}

	/**----------------------------------------------------------------------------
	 * \brief  SetSceneNo
	 * \param  sceneNo
	 * \return
	 */
	int SetSceneNo(int nextNo) {
		return sceneNo = nextNo;
	}

	///--------------------------------------------------------------
	///							メンバ変数
protected:
	// シーン番号を保存する変数
	static int sceneNo;
};

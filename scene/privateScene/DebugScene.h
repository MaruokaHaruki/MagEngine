/*********************************************************************
 * \file   DebugScene.h
 * \brief  デバッグシーンクラス
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note   NOTE: SceneContextを使用してセットアップの依存関係を削減
 *********************************************************************/
#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "BaseScene.h"
//========================================
// Game
#include "Cloud.h"
#include "TrailEffectManager.h"

// Forward declaration
class SceneContext;

class DebugScene : public BaseScene {
	///--------------------------------------------------------------
	///                            メンバ関数
public:
	/// \brief 初期化 - NOTE: 引数がSceneContext*の1つに削減
	void Initialize(SceneContext *context) override;
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
	//========================================
	// オーディオ
	MagEngine::MAudioG *audio_;

	//========================================
	// スプライト

	//========================================
	// 3dオブジェクト
	// モンスターボール
	std::unique_ptr<MagEngine::Object3d> objMonsterBall_;
	// 地面
	std::unique_ptr<MagEngine::Object3d> objTerrain_;

	//========================================
	// レベルデータローダー
	std::unique_ptr<MagEngine::LevelDataLoader> levelDataLoader_;
	// レベルデータから作成されたObject3Dリスト
	std::vector<std::unique_ptr<MagEngine::Object3d>> levelObjects_;
	// Object3dSetupの保存（再読み込み時に使用）
	MagEngine::Object3dSetup *object3dSetup_;

	//========================================
	// パーティクル

	// パーティクルエミッター

	///--------------------------------------------------------------
	///						 アプリケーション固有
	Transform transform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	//========================================
	// Skybox用デバッグフラグ
	bool showSkyboxDebug_ = true;
	float skyboxScale_ = 100.0f;

	//========================================
	// ボールテスト

	//========================================
	// Cloud
	std::unique_ptr<MagEngine::Cloud> cloud_;

	//========================================
	// TrailEffect
	MagEngine::TrailEffectManager *trailEffectManager_ = nullptr;

	//========================================
	// TrailEffect テスト用パラメータ
	float trailLoopTimer_ = 0.0f;				// ループアニメーション用タイマー
	float trailLoopRadius_ = 2.0f;				// ループの半径
	float trailLoopHeight_ = 2.0f;				// ループの高さ
	float trailLoopSpeed_ = 2.0f;				// ループの速度（rad/sec）
	Vector3 trailLoopCenter_{0.0f, 0.0f, 0.0f}; // ループの中心

	//========================================
	// 雲の穴開けテスト用パラメータ（円錐形状）
	float bulletHoleStartRadius_ = 4.0f;			   // 弾痕の開始半径（入口）
	float bulletHoleEndRadius_ = 0.2f;				   // 弾痕の終了半径（出口）
	float bulletHoleConeLength_ = 1000.0f;			   // 円錐の長さ
	float bulletHoleLifeTime_ = 15.0f;				   // 弾痕の持続時間
	Vector3 manualBulletOrigin_{0.0f, 180.0f, 300.0f}; // マニュアル追加用の原点
	Vector3 manualBulletDirection_{0.0f, 0.0f, 1.0f};  // マニュアル追加用の方向
};

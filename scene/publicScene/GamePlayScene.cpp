/*********************************************************************
 * \file   GamePlayScene.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "GamePlayScene.h"
#include "CameraManager.h"
#include "Enemy.h"
#include "Input.h" // Inputクラスを使用するためにインクルード
#include "LineManager.h"
#include "ModelManager.h"
#include "Player.h"

///=============================================================================
///						初期化
void GamePlayScene::Initialize(SpriteSetup *spriteSetup, Object3dSetup *object3dSetup, ParticleSetup *particleSetup) {
	// 適当に引数を使用
	// 引数を使用しない場合は警告を出さないようにする
	spriteSetup;
	object3dSetup;
	particleSetup;
	//========================================
	// カメラ設定
	CameraManager::GetInstance()->GetCamera("DefaultCamera")->SetTransform({{1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.3f, -8.0f}});

	//========================================
	// DebugTextManagerにカメラを設定
	DebugTextManager::GetInstance()->SetCamera(CameraManager::GetInstance()->GetCamera("DefaultCamera"));

	//========================================
	// モデルの読み込み
	ModelManager::GetInstance()->LoadMedel("jet.obj");	   // モデルは事前にロードしておく
	ModelManager::GetInstance()->LoadMedel("skydome.obj"); // 地面のモデルもロード

	//========================================
	// スプライトクラス(Game)

	//========================================
	// 地面

	//========================================
	// スカイドーム
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(object3dSetup, "skydome.obj");

	//========================================
	// プレイヤー
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dSetup, "jet.obj"); // PlayerにObject3dSetupとモデル名を渡す

	//========================================
	// パーティクルクラス
	particle_ = std::make_unique<Particle>();
	// パーティクルの初期化
	particle_->Initialize(particleSetup);
	particle_->SetCustomTextureSize({10.0f, 10.0f});

	// 通常のパーティクルグループの作成（Board形状）
	particle_->CreateParticleGroup("Particle", "sandWind.png", ParticleShape::Board);

	// 爆発エフェクト用の複数の形状を作成
	// 1. メインの爆発エフェクト（Board形状 - 火花）
	particle_->CreateParticleGroup("ExplosionSparks", "sandWind.png", ParticleShape::Board);

	// 2. リング形状の衝撃波
	particle_->CreateParticleGroup("ExplosionRing", "sandWind.png", ParticleShape::Ring);

	// 3. シリンダー形状の煙柱
	particle_->CreateParticleGroup("ExplosionSmoke", "sandWind.png", ParticleShape::Cylinder);

	// パーティクルエミッター（通常用）
	particleEmitter_ =
		std::make_unique<ParticleEmitter>(particle_.get(),
										  "Particle",
										  Transform{{0.1f, 0.1f, 0.1f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
										  4,
										  0.5f,
										  true);

	//========================================
	// 敵
	enemy_ = std::make_unique<Enemy>();
	enemy_->Initialize(object3dSetup, "jet.obj", {0.0f, 0.0f, 10.0f}); // 固定位置に配置
	// 敵にパーティクルシステムを設定
	enemy_->SetParticleSystem(particle_.get(), particleSetup);

	//========================================
	// 当たり判定
	collisionManager_ = std::make_unique<CollisionManager>();
	collisionManager_->Initialize();

	// 敵の位置にデバッグテキストを配置（固定位置）
	DebugTextManager::GetInstance()->AddText3D("Enemy", {5.0f, 1.0f, 5.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
}

///=============================================================================
///							終了処理
void GamePlayScene::Finalize() {
}

///=============================================================================
///							更新
void GamePlayScene::Update() {
	//========================================
	// プレイヤー
	if (player_) {
		player_->Update();

		// プレイヤーの位置にデバッグテキストを配置
		Vector3 playerPos = player_->GetPosition();
		playerPos.y += 2.0f; // プレイヤーの少し上に表示
		DebugTextManager::GetInstance()->AddText3D("Player", playerPos, {0.0f, 1.0f, 0.0f, 1.0f});

		// グリッドは自動でアニメーションするため、手動オフセットは不要
		// LineManager::GetInstance()->SetGridAnimation(true); // 初期化時に設定済み
	}

	//========================================
	// 敵の更新
	if (enemy_) {
		enemy_->Update();
	}

	//========================================
	// パーティクルの更新
	if (particle_) {
		particle_->Update();
	}
	if (particleEmitter_) {
		particleEmitter_->Update();
	}

	//========================================
	// スカイドーム
	if (skydome_) {
		skydome_->Update();
	}

	//---------------------------------------
	//  当たり判定
	//  情報のリセット
	collisionManager_->Reset();

	//  プレイヤーの当たり判定を追加
	if (player_) {
		collisionManager_->AddCollider(player_.get());
	}

	//  敵の当たり判定を追加（生存中のみ）
	if (enemy_ && enemy_->IsAlive()) {
		collisionManager_->AddCollider(enemy_.get());
	}

	//  プレイヤーの弾の当たり判定を追加
	if (player_) {
		const auto &bullets = player_->GetBullets();
		for (const auto &bullet : bullets) {
			if (bullet->IsAlive()) {
				collisionManager_->AddCollider(bullet.get());
			}
		}
	}

	//  当たり判定の更新
	collisionManager_->Update();
}

///=============================================================================
///                        スプライト描画
void GamePlayScene::Object2DDraw() {
}

///=============================================================================
///                        3D描画
void GamePlayScene::Object3DDraw() {
	//========================================
	// 地面

	//=========================================
	// スカイドーム
	if (skydome_) {
		skydome_->Draw();
	}

	//========================================
	// プレイヤー
	if (player_) {
		player_->Draw();
		// プレイヤーの弾も描画
		player_->DrawBullets();
	}

	//========================================
	// 敵
	if (enemy_) {
		enemy_->Draw();
	}

	//========================================
	// 当たり判定
	collisionManager_->Draw();
}

///=============================================================================
///
void GamePlayScene::ParticleDraw() {
	//========================================
	// パーティクル描画
	if (particle_) {
		particle_->Draw();
	}
	if (particleEmitter_) {
		particleEmitter_->Draw();
	}
}

///=============================================================================
///						ImGui描画
void GamePlayScene::ImGuiDraw() {
#ifdef _DEBUG
	ImGui::Begin("DebugScene");
	ImGui::Text("Hello, GamePlayScene!");
	ImGui::End();
	//========================================
	// プレイヤー
	if (player_) {
		player_->DrawImGui();
	}

	//========================================
	// 敵
	if (enemy_) {
		enemy_->DrawImGui();
	}

	//========================================
	// 当たり判定
	collisionManager_->DrawImGui();
#endif // _DEBUG
}

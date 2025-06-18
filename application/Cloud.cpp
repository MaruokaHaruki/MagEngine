#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include <algorithm>
// ここまで
#include "Cloud.h"
#include "ImguiSetup.h"
#include <cmath>
#include <numbers>
#include <random>

///=============================================================================
///                        初期化
void Cloud::Initialize(Particle *particle, ParticleSetup *particleSetup) {
	particle_ = particle;
	particleSetup_ = particleSetup;

	// 乱数エンジンの初期化
	randomEngine_.seed(randomDevice_());

	// 雲パーティクルグループの作成
	particle_->CreateParticleGroup("CloudSystem", "sandWind.png", ParticleShape::Board);

	// パーティクルシステムの設定を強化
	particle_->SetBillboard(true);
	particle_->SetCustomTextureSize({15.0f, 15.0f}); // サイズを小さく（密度重視）

	// 雲エミッターの初期化
	cloudEmitters_.clear();
	cloudParams_.clear();

	// 広範囲に雲のじゅうたんを初期配置
	CreateCloudCarpet();
}

///=============================================================================
///                        雲のじゅうたん作成
void Cloud::CreateCloudCarpet() {
	// グリッド状の基本配置（原点周辺に集中）
	int gridSize = 10;		  // グリッドサイズを縮小（10x10）
	float gridSpacing = 8.0f; // グリッド間隔を縮小
	float baseZ = 30.0f;	  // Z+側の開始位置を原点近くに

	for (int x = -gridSize; x <= gridSize; ++x) {
		for (int z = 0; z <= gridSize; ++z) { // Z+側の範囲を縮小
			// 基本位置（原点周辺）
			Vector3 gridPos = {
				x * gridSpacing,
				params_.cloudHeight,
				baseZ - z * gridSpacing};

			// ランダムなオフセットを追加してグリッド感を薄める
			std::uniform_real_distribution<float> offsetDist(-gridSpacing * 0.3f, gridSpacing * 0.3f);
			std::uniform_real_distribution<float> heightDist(-params_.heightVariation * 0.5f, params_.heightVariation * 0.5f);

			gridPos.x += offsetDist(randomEngine_);
			gridPos.y += heightDist(randomEngine_);
			gridPos.z += offsetDist(randomEngine_);

			CreateCloudEmitter(gridPos);
		}
	}

	// 追加のランダム散布（原点周辺に密集）
	for (int i = 0; i < params_.cloudCount; ++i) { // 生成数を調整
		std::uniform_real_distribution<float> xDist(-params_.spawnRadius, params_.spawnRadius);
		std::uniform_real_distribution<float> yDist(params_.cloudHeight - params_.heightVariation * 0.7f,
													params_.cloudHeight + params_.heightVariation * 0.7f);
		std::uniform_real_distribution<float> zDist(5.0f, params_.spawnRadius); // Z+側に限定

		Vector3 randomPos = {
			xDist(randomEngine_),
			yDist(randomEngine_),
			zDist(randomEngine_)};

		CreateCloudEmitter(randomPos);
	}
}

///=============================================================================
///                        更新
void Cloud::Update(const Vector3 &playerPosition) {
	spawnTimer_ += 1.0f / 60.0f;

	// 雲エミッターの更新
	for (auto &emitter : cloudEmitters_) {
		if (emitter) {
			emitter->Update();
		}
	}

	// 雲パラメータの更新
	UpdateCloudPositions(playerPosition);

	// より頻繁に新しい雲を生成
	if (spawnTimer_ >= spawnInterval_) {
		CheckAndSpawnClouds(playerPosition);
		spawnTimer_ = 0.0f;
	}

	// 古い雲の削除チェック
	RemoveDistantClouds(playerPosition);
}

///=============================================================================
///                        描画
void Cloud::Draw() {
	// パーティクルシステムが描画を担当するため、ここでは特に何もしない
}

///=============================================================================
///                        雲エミッターの作成
void Cloud::CreateCloudEmitter(const Vector3 &basePosition) {
	// ランダムな位置の計算（原点周辺に集中）
	std::uniform_real_distribution<float> radiusDist(1.0f, 5.0f); // 分散範囲を縮小
	std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> heightDist(-2.0f, 2.0f); // 高さの分散を縮小

	float radius = radiusDist(randomEngine_);
	float angle = angleDist(randomEngine_);
	float heightOffset = heightDist(randomEngine_);

	Vector3 spawnPosition = {
		basePosition.x + radius * std::cos(angle),
		basePosition.y + heightOffset,
		basePosition.z + radius * std::sin(angle)};

	// エミッターの作成（高頻度で生成）
	Transform emitterTransform;
	emitterTransform.translate = spawnPosition;
	emitterTransform.rotate = {0.0f, 0.0f, 0.0f};
	emitterTransform.scale = {1.0f, 1.0f, 1.0f};

	auto emitter = std::make_unique<ParticleEmitter>(
		particle_, "CloudSystem", emitterTransform, 25, 0.02f, true); // パーティクル数を調整、頻度を上げる

	// 雲の設定
	ConfigureCloudEmitter(emitter.get());

	// 雲パラメータの追加（寿命を短く）
	CloudParams cloudParam;
	cloudParam.position = spawnPosition;
	cloudParam.velocity = CalculateWindVelocity();
	cloudParam.lifeTime = 25.0f; // 寿命を大幅短縮（80→25秒）
	cloudParam.isActive = true;

	cloudEmitters_.push_back(std::move(emitter));
	cloudParams_.push_back(cloudParam);
}

///=============================================================================
///                        雲エミッターの設定
void Cloud::ConfigureCloudEmitter(ParticleEmitter *emitter) {
	// パーティクルの基本設定（密度重視）
	emitter->SetCustomTextureSize({8.0f, 8.0f}); // さらに小さく
	emitter->SetBillboard(true);

	// 移動範囲の設定（原点周辺に集中）
	emitter->SetTranslateRange({-8.0f, -2.0f, -8.0f}, {8.0f, 2.0f, 8.0f});

	// 初速度の設定（風の影響を強化）
	Vector3 windVel = CalculateWindVelocity();
	emitter->SetVelocityRange(
		{windVel.x - 1.5f, windVel.y - 0.3f, windVel.z - 1.5f},
		{windVel.x + 1.5f, windVel.y + 0.3f, windVel.z + 1.5f});

	// 色の設定（より薄く、密度で補う）
	emitter->SetColorRange(
		{0.85f, 0.92f, 1.0f, 0.3f}, // 薄い白
		{1.0f, 1.0f, 1.0f, 0.6f}	// 少し濃い白
	);

	// 寿命の設定（短めに）
	emitter->SetLifetimeRange(3.0f, 8.0f); // パーティクル寿命を短縮

	// スケールの設定（小さめで数重視）
	emitter->SetInitialScaleRange({0.6f, 0.6f, 0.6f}, {1.5f, 1.5f, 1.5f});
	emitter->SetEndScaleRange({1.5f, 1.5f, 1.5f}, {3.0f, 3.0f, 3.0f});

	// 回転の設定
	emitter->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.1f});
	emitter->SetEndRotationRange({0.0f, 0.0f, -0.1f}, {0.0f, 0.0f, 0.1f});

	// 重力の設定（ほぼ無重力）
	emitter->SetGravity({0.0f, -0.001f, 0.0f});

	// フェードの設定
	emitter->SetFadeInOut(0.1f, 0.8f);
}

///=============================================================================
///                        風の速度計算
Vector3 Cloud::CalculateWindVelocity() {
	if (!params_.enableWind) {
		return {0.0f, 0.0f, 0.0f};
	}

	// 風速を大幅に強化
	Vector3 windVelocity = {
		params_.cloudSpeed * 2.0f * std::cos(params_.windDirection),
		0.0f,
		params_.cloudSpeed * 2.0f * std::sin(params_.windDirection)};

	return windVelocity;
}

///=============================================================================
///                        雲位置の更新
void Cloud::UpdateCloudPositions(const Vector3 &playerPosition) {
	Vector3 windVel = CalculateWindVelocity();

	for (size_t i = 0; i < cloudEmitters_.size() && i < cloudParams_.size(); ++i) {
		if (cloudParams_[i].isActive) {
			// 風による移動（より速く）
			cloudParams_[i].position.x += windVel.x * (1.0f / 60.0f);
			cloudParams_[i].position.z += windVel.z * (1.0f / 60.0f);

			// エミッターの位置を更新
			cloudEmitters_[i]->SetTranslate(cloudParams_[i].position);

			// 寿命の減少
			cloudParams_[i].lifeTime -= 1.0f / 60.0f;
			if (cloudParams_[i].lifeTime <= 0.0f) {
				cloudParams_[i].isActive = false;
			}
		}
	}
}

///=============================================================================
///                        新しい雲の生成チェック
void Cloud::CheckAndSpawnClouds(const Vector3 &playerPosition) {
	// アクティブな雲の数をカウント
	int activeClouds = 0;
	for (const auto &param : cloudParams_) {
		if (param.isActive) {
			activeClouds++;
		}
	}

	// 大量の雲を常に維持（目標数を8倍に）
	int targetCloudCount = params_.cloudCount * 8; // 目標数を調整

	// 雲が少しでも足りなくなったら即座に補充
	int shortage = targetCloudCount - activeClouds;
	if (shortage > 0) {
		// 不足分に応じて生成（原点周辺に集中）
		int spawnCount = std::max(20, shortage / 8); // 生成数を調整

		for (int i = 0; i < spawnCount; ++i) {
			// 原点周辺の風上側（Z+方向）に生成
			std::uniform_real_distribution<float> xDist(-params_.spawnRadius * 0.8f, params_.spawnRadius * 0.8f);
			std::uniform_real_distribution<float> yDist(params_.cloudHeight - params_.heightVariation * 0.6f,
														params_.cloudHeight + params_.heightVariation * 0.6f);
			std::uniform_real_distribution<float> zDist(params_.spawnRadius * 0.3f, params_.spawnRadius * 1.2f); // 原点近くのZ+側

			Vector3 spawnBase = {
				xDist(randomEngine_),
				yDist(randomEngine_),
				zDist(randomEngine_)};

			CreateCloudEmitter(spawnBase);
		}
	}

	// 追加で常時生成（途切れ防止）
	for (int i = 0; i < 3; ++i) { // 毎回3個追加生成
		std::uniform_real_distribution<float> xDist(-params_.spawnRadius * 0.6f, params_.spawnRadius * 0.6f);
		std::uniform_real_distribution<float> yDist(params_.cloudHeight - params_.heightVariation * 0.5f,
													params_.cloudHeight + params_.heightVariation * 0.5f);
		std::uniform_real_distribution<float> zDist(params_.spawnRadius * 0.4f, params_.spawnRadius * 1.0f);

		Vector3 continuousSpawn = {
			xDist(randomEngine_),
			yDist(randomEngine_),
			zDist(randomEngine_)};

		CreateCloudEmitter(continuousSpawn);
	}
}

///=============================================================================
///                        遠い雲の削除
void Cloud::RemoveDistantClouds(const Vector3 &playerPosition) {
	// 原点からの距離で削除判定
	for (auto it = cloudEmitters_.begin(); it != cloudEmitters_.end();) {
		size_t index = std::distance(cloudEmitters_.begin(), it);

		if (index < cloudParams_.size()) {
			// 原点からの距離を計算
			Vector3 distance = {
				cloudParams_[index].position.x,
				0.0f,
				cloudParams_[index].position.z};
			float distanceLength = std::sqrt(distance.x * distance.x + distance.z * distance.z);

			// Z座標が大きくマイナスになった雲も削除（風下側に流れ切った雲）
			bool tooFarDownwind = cloudParams_[index].position.z < -params_.respawnDistance * 0.7f; // 削除距離を短縮
			bool tooFarFromOrigin = distanceLength > params_.respawnDistance;

			// 遠すぎるか非アクティブな雲を削除
			if (tooFarDownwind || tooFarFromOrigin || !cloudParams_[index].isActive) {
				it = cloudEmitters_.erase(it);
				cloudParams_.erase(cloudParams_.begin() + index);
			} else {
				++it;
			}
		} else {
			++it;
		}
	}
}

///=============================================================================
///                        ImGui描画
void Cloud::DrawImGui() {
#ifdef _DEBUG
	if (ImGui::TreeNode("Cloud System")) {
		ImGui::SliderInt("Cloud Count", &params_.cloudCount, 100, 800);			 // 最大数を調整
		ImGui::SliderFloat("Spawn Radius", &params_.spawnRadius, 20.0f, 100.0f); // 生成半径を原点周辺に制限
		ImGui::SliderFloat("Cloud Speed", &params_.cloudSpeed, 2.0f, 20.0f);
		ImGui::SliderFloat("Cloud Height", &params_.cloudHeight, 15.0f, 50.0f);
		ImGui::SliderFloat("Height Variation", &params_.heightVariation, 5.0f, 30.0f);
		ImGui::SliderFloat("Wind Direction", &params_.windDirection, 0.0f, 2.0f * std::numbers::pi_v<float>);
		ImGui::SliderFloat("Respawn Distance", &params_.respawnDistance, 100.0f, 300.0f); // 削除距離を短縮
		ImGui::Checkbox("Enable Wind", &params_.enableWind);

		ImGui::Text("Active Clouds: %zu", cloudEmitters_.size());
		ImGui::Text("Active Cloud Params: %zu", cloudParams_.size());
		ImGui::Text("Target Cloud Count: %d", params_.cloudCount * 8);
		ImGui::Text("Wind Direction: Z+ to Z- (%.2f rad)", params_.windDirection);
		ImGui::Text("Spawn Interval: %.3f seconds", spawnInterval_);

		// 各雲の状態を表示
		for (size_t i = 0; i < cloudParams_.size() && i < 3; ++i) {
			ImGui::Text("Cloud %zu: Active=%s, Life=%.1f, Pos=(%.1f,%.1f,%.1f)", i,
						cloudParams_[i].isActive ? "Yes" : "No",
						cloudParams_[i].lifeTime,
						cloudParams_[i].position.x,
						cloudParams_[i].position.y,
						cloudParams_[i].position.z);
		}

		// 雲のじゅうたんを再生成するボタン
		if (ImGui::Button("Recreate Cloud Carpet")) {
			cloudEmitters_.clear();
			cloudParams_.clear();
			CreateCloudCarpet();
		}

		// 強制的に原点周辺に雲を生成するボタン
		if (ImGui::Button("Spawn Origin Clouds")) {
			for (int i = 0; i < 100; ++i) { // 100個を原点周辺に生成
				std::uniform_real_distribution<float> xDist(-params_.spawnRadius, params_.spawnRadius);
				std::uniform_real_distribution<float> yDist(params_.cloudHeight - params_.heightVariation,
															params_.cloudHeight + params_.heightVariation);
				std::uniform_real_distribution<float> zDist(5.0f, params_.spawnRadius);

				Vector3 pos = {
					xDist(randomEngine_),
					yDist(randomEngine_),
					zDist(randomEngine_)};
				CreateCloudEmitter(pos);
			}
		}

		ImGui::TreePop();
	}
#endif
}

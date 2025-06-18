#include "Cloud.h"
#include "ImguiSetup.h"
#include <cmath>
#include <random>

void Cloud::Initialize(Particle *particle, const Vector3 &basePosition) {
	particle_ = particle;
	lastPlayerPosition_ = basePosition;

	// 雲の設定パラメータ初期化（雲らしい値に調整）
	cloudDensity_ = 1.5f;
	cloudSpeed_ = 1.0f;
	minAltitude_ = -8.0f;
	maxAltitude_ = -3.0f;
	emissionRadius_ = 25.0f;
	followSmoothing_ = 0.02f;

	// アフターバーナー効果関連の初期化
	afterburnerMode_ = false;
	afterburnerIntensity_ = 1.0f;
	speedEffectMultiplier_ = 1.0f;
	cloudFlowAmount_ = 1.0f;

	// 雲エミッターを作成
	CreateCloudEmitters();
}

void Cloud::CreateCloudEmitters() {
	cloudEmitters_.clear();

	// アフターバーナーモード時はエミッター数を大幅に増加
	float densityMultiplier = afterburnerMode_ ? (3.0f * afterburnerIntensity_) : 1.0f;
	int emitterCount = static_cast<int>(10 * cloudDensity_ * densityMultiplier * cloudFlowAmount_);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
	std::uniform_real_distribution<float> radiusDist(8.0f, emissionRadius_);
	std::uniform_real_distribution<float> altitudeDist(minAltitude_, maxAltitude_);

	for (int i = 0; i < emitterCount; ++i) {
		// ランダムな円形配置
		float angle = angleDist(gen);
		float radius = radiusDist(gen);
		float altitude = altitudeDist(gen);

		Vector3 emitterOffset = {
			radius * std::cos(angle),
			altitude,
			radius * std::sin(angle)};

		Vector3 emitterPos = {
			lastPlayerPosition_.x + emitterOffset.x,
			lastPlayerPosition_.y + emitterOffset.y,
			lastPlayerPosition_.z + emitterOffset.z};

		Transform emitterTransform = {
			{2.0f, 2.0f, 2.0f},
			{0.0f, 0.0f, 0.0f},
			emitterPos};

		// アフターバーナーモード時は生成間隔を短縮、生成数を増加
		int particleCount = afterburnerMode_ ? static_cast<int>(4 * afterburnerIntensity_) : 2;
		float emissionInterval = afterburnerMode_ ? (0.1f / afterburnerIntensity_) : 0.8f;

		auto cloudEmitter = std::make_unique<ParticleEmitter>(
			particle_,
			"CloudParticles",
			emitterTransform,
			particleCount,
			emissionInterval,
			true);

		// 速度効果を考慮した設定
		float effectiveSpeed = cloudSpeed_ * speedEffectMultiplier_ * cloudFlowAmount_;
		float velocityMultiplier = afterburnerMode_ ? (2.0f * afterburnerIntensity_) : 1.0f;

		cloudEmitter->SetBillboard(true);
		cloudEmitter->SetVelocityRange(
			{-effectiveSpeed * velocityMultiplier, -0.1f, -effectiveSpeed * velocityMultiplier * 2.0f}, // Z方向により強い流れ
			{effectiveSpeed * velocityMultiplier, 0.1f, -effectiveSpeed * velocityMultiplier * 0.5f});
		cloudEmitter->SetTranslateRange({-2.0f, -0.5f, -2.0f}, {2.0f, 0.5f, 2.0f});

		// アフターバーナー時は色を少し変える
		if (afterburnerMode_) {
			cloudEmitter->SetColorRange({0.9f, 0.9f, 0.95f, 0.4f}, {1.0f, 0.98f, 0.95f, 0.8f}); // 少し黄色がかった白
		} else {
			cloudEmitter->SetColorRange({0.95f, 0.95f, 1.0f, 0.3f}, {1.0f, 1.0f, 1.0f, 0.7f});
		}

		// アフターバーナー時は生存時間を短縮して流れ感を演出
		float lifetimeMultiplier = afterburnerMode_ ? 0.7f : 1.0f;
		cloudEmitter->SetLifetimeRange(8.0f * lifetimeMultiplier, 15.0f * lifetimeMultiplier);

		cloudEmitter->SetInitialScaleRange({3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f});
		cloudEmitter->SetEndScaleRange({8.0f, 8.0f, 8.0f}, {12.0f, 12.0f, 12.0f});
		cloudEmitter->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
		cloudEmitter->SetEndRotationRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 6.28f});
		cloudEmitter->SetGravity({0.0f, 0.0f, 0.0f});
		cloudEmitter->SetFadeInOut(0.3f, 0.6f);

		cloudEmitters_.push_back(std::move(cloudEmitter));
	}
}

void Cloud::CreateAfterburnerEmitters() {
	// 既存のエミッターに追加でアフターバーナー専用エミッターを作成
	int afterburnerEmitterCount = static_cast<int>(5 * afterburnerIntensity_);
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);

	for (int i = 0; i < afterburnerEmitterCount; ++i) {
		// プレイヤーの直後に配置
		float angle = angleDist(gen);
		Vector3 emitterPos = {
			lastPlayerPosition_.x + std::cos(angle) * 3.0f,
			lastPlayerPosition_.y - 2.0f,
			lastPlayerPosition_.z - 5.0f - i * 2.0f}; // 後方に一列配置

		Transform emitterTransform = {
			{1.5f, 1.5f, 1.5f},
			{0.0f, 0.0f, 0.0f},
			emitterPos};

		auto afterburnerEmitter = std::make_unique<ParticleEmitter>(
			particle_,
			"CloudParticles",
			emitterTransform,
			static_cast<int>(6 * afterburnerIntensity_),
			0.05f, // 非常に高頻度
			true);

		afterburnerEmitter->SetBillboard(true);
		afterburnerEmitter->SetVelocityRange(
			{-2.0f, -1.0f, -15.0f * afterburnerIntensity_}, // 強い後方流れ
			{2.0f, 1.0f, -8.0f * afterburnerIntensity_});
		afterburnerEmitter->SetTranslateRange({-1.0f, -0.3f, -1.0f}, {1.0f, 0.3f, 1.0f});
		afterburnerEmitter->SetColorRange({0.85f, 0.85f, 0.9f, 0.6f}, {0.95f, 0.95f, 1.0f, 0.9f});
		afterburnerEmitter->SetLifetimeRange(2.0f, 4.0f); // 短い生存時間
		afterburnerEmitter->SetInitialScaleRange({2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f});
		afterburnerEmitter->SetEndScaleRange({1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}); // 縮小
		afterburnerEmitter->SetGravity({0.0f, 0.0f, 0.0f});
		afterburnerEmitter->SetFadeInOut(0.1f, 0.8f);

		cloudEmitters_.push_back(std::move(afterburnerEmitter));
	}
}

void Cloud::Update(const Vector3 &playerPosition) {
	// プレイヤー位置への追従（アフターバーナー時はより敏感に）
	float followRate = afterburnerMode_ ? (followSmoothing_ * 2.0f) : followSmoothing_;
	lastPlayerPosition_.x += (playerPosition.x - lastPlayerPosition_.x) * followRate;
	lastPlayerPosition_.y += (playerPosition.y - lastPlayerPosition_.y) * followRate;
	lastPlayerPosition_.z += (playerPosition.z - lastPlayerPosition_.z) * followRate;

	// エミッター位置の更新
	UpdateEmitterPositions(lastPlayerPosition_);

	// 速度効果の更新
	UpdateSpeedEffect();

	// 各エミッターの更新
	for (auto &emitter : cloudEmitters_) {
		emitter->Update();
	}
}

void Cloud::UpdateSpeedEffect() {
	// 速度効果に応じてパーティクルの流れを調整
	float effectiveSpeed = cloudSpeed_ * speedEffectMultiplier_ * cloudFlowAmount_;
	float velocityMultiplier = afterburnerMode_ ? (2.0f * afterburnerIntensity_) : 1.0f;

	for (auto &emitter : cloudEmitters_) {
		emitter->SetVelocityRange(
			{-effectiveSpeed * velocityMultiplier, -0.1f, -effectiveSpeed * velocityMultiplier * 2.0f},
			{effectiveSpeed * velocityMultiplier, 0.1f, -effectiveSpeed * velocityMultiplier * 0.5f});
	}
}

void Cloud::UpdateEmitterPositions(const Vector3 &playerPosition) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> offsetDist(-1.0f, 1.0f); // 小さな変動

	for (size_t i = 0; i < cloudEmitters_.size(); ++i) {
		// より自然な配置（等間隔 + ランダム要素）
		float baseAngle = (static_cast<float>(i) / cloudEmitters_.size()) * 6.28318f;
		float randomAngleOffset = offsetDist(gen) * 0.5f; // 小さな角度の変動
		float angle = baseAngle + randomAngleOffset;

		float baseRadius = emissionRadius_ * (0.6f + offsetDist(gen) * 0.3f); // 半径に変動

		Vector3 newPosition = {
			playerPosition.x + baseRadius * std::cos(angle),
			playerPosition.y + (minAltitude_ + maxAltitude_) * 0.5f + offsetDist(gen) * 0.5f,
			playerPosition.z + baseRadius * std::sin(angle)};

		cloudEmitters_[i]->SetTranslate(newPosition);
	}
}

void Cloud::Draw() {
	// エミッター自体は描画しない（パーティクルのみ）
	for (auto &emitter : cloudEmitters_) {
		emitter->Draw();
	}
}

void Cloud::DrawImGui() {
	ImGui::Begin("Cloud System");
	
	// 基本設定
	ImGui::Text("=== Basic Settings ===");
	ImGui::SliderFloat("Cloud Density", &cloudDensity_, 0.1f, 3.0f);
	ImGui::SliderFloat("Cloud Speed", &cloudSpeed_, 0.1f, 5.0f);
	ImGui::SliderFloat("Cloud Flow Amount", &cloudFlowAmount_, 0.1f, 5.0f);
	
	// アフターバーナー効果
	ImGui::Separator();
	ImGui::Text("=== Afterburner Effect ===");
	if (ImGui::Checkbox("Enable Afterburner Mode", &afterburnerMode_)) {
		CreateCloudEmitters(); // モード変更時にエミッターを再生成
	}
	if (afterburnerMode_) {
		ImGui::SliderFloat("Afterburner Intensity", &afterburnerIntensity_, 0.5f, 3.0f);
		if (ImGui::Button("Add Afterburner Emitters")) {
			CreateAfterburnerEmitters();
		}
	}
	
	// 速度演出
	ImGui::Separator();
	ImGui::Text("=== Speed Effect ===");
	ImGui::SliderFloat("Speed Effect Multiplier", &speedEffectMultiplier_, 0.5f, 5.0f);
	
	// 高度設定
	ImGui::Separator();
	ImGui::Text("=== Altitude Settings ===");
	ImGui::SliderFloat("Min Altitude", &minAltitude_, -20.0f, 0.0f);
	ImGui::SliderFloat("Max Altitude", &maxAltitude_, -15.0f, 5.0f);
	ImGui::SliderFloat("Emission Radius", &emissionRadius_, 10.0f, 50.0f);
	ImGui::SliderFloat("Follow Smoothing", &followSmoothing_, 0.001f, 0.1f);
	
	ImGui::Text("Active Emitters: %zu", cloudEmitters_.size());

	if (ImGui::Button("Recreate All Cloud Emitters")) {
		CreateCloudEmitters();
	}

	// プリセット設定
	ImGui::Separator();
	ImGui::Text("=== Presets ===");
	if (ImGui::Button("Normal Flight")) {
		afterburnerMode_ = false;
		cloudDensity_ = 1.0f;
		cloudFlowAmount_ = 1.0f;
		speedEffectMultiplier_ = 1.0f;
		CreateCloudEmitters();
	}
	ImGui::SameLine();
	if (ImGui::Button("High Speed")) {
		afterburnerMode_ = true;
		afterburnerIntensity_ = 1.5f;
		cloudDensity_ = 2.0f;
		cloudFlowAmount_ = 3.0f;
		speedEffectMultiplier_ = 2.5f;
		CreateCloudEmitters();
	}
	ImGui::SameLine();
	if (ImGui::Button("Climax Mode")) {
		afterburnerMode_ = true;
		afterburnerIntensity_ = 2.5f;
		cloudDensity_ = 3.0f;
		cloudFlowAmount_ = 5.0f;
		speedEffectMultiplier_ = 4.0f;
		CreateCloudEmitters();
		CreateAfterburnerEmitters();
	}

	ImGui::End();
}

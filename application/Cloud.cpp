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

	// 霧パーティクルグループの作成
	particle_->CreateParticleGroup("FogSmoke", "sandWind.png", ParticleShape::Board);

	// 霧効果のセットアップ
	SetupFogEffect();

	// エミッターの作成
	CreateEmitters();
}

///=============================================================================
///                        更新
void Cloud::Update(const Vector3 &playerPosition) {
	if (!isActive_)
		return;

	// プレイヤー位置を基準に霧の中心を調整
	fogCenter_ = playerPosition;

	// 全エミッターの更新
	for (auto &emitter : fogEmitters_) {
		if (emitter) {
			emitter->Update();
		}
	}

	// デバッグ可視化の描画
	if (showDebugVisualization_) {
		DrawDebugVisualization();
	}
}

///=============================================================================
///                        描画
void Cloud::Draw() {
	// パーティクル描画は自動的に行われるため、ここでは特に何もしない
}

///=============================================================================
///                        ImGui描画
void Cloud::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Cloud System");

	if (ImGui::CollapsingHeader("Fog Parameters")) {
		ImGui::Checkbox("Active", &isActive_);
		ImGui::SliderFloat3("Fog Center", &fogCenter_.x, -100.0f, 100.0f);
		ImGui::SliderFloat3("Fog Size", &fogSize_.x, 1.0f, 100.0f);
		ImGui::SliderFloat3("Wind Direction", &windDirection_.x, -5.0f, 5.0f);
		ImGui::SliderFloat("Wind Strength", &windStrength_, 0.0f, 10.0f);
		ImGui::SliderInt("Emitter Count", &emitterCount_, 1, 128);
		ImGui::SliderFloat("Frequency", &emitterFrequency_, 0.01f, 1.0f);
		ImGui::SliderInt("Particles Per Emitter", &particlesPerEmitter_, 1, 32);
		ImGui::SliderFloat("Fog Density", &fogDensity_, 0.1f, 1.0f);

		if (ImGui::Button("Recreate Emitters")) {
			fogEmitters_.clear();
			SetupFogEffect();
			CreateEmitters();
		}

		// デバッグ表示設定を追加
		ImGui::Separator();
		ImGui::Text("Debug Visualization");
		ImGui::Checkbox("Show Debug Area", &showDebugVisualization_);
		ImGui::ColorEdit4("Area Color", &areaColor_.x);
		ImGui::ColorEdit4("Wind Color", &windColor_.x);
		ImGui::SliderFloat("Wind Arrow Length", &windArrowLength_, 1.0f, 20.0f);
	}

	ImGui::Text("Active Emitters: %zu", fogEmitters_.size());

	ImGui::End();
#endif
}

///=============================================================================
///                        エミッターの作成
void Cloud::CreateEmitters() {
	fogEmitters_.clear();

	// 指定された数のエミッターを範囲内にランダム配置
	std::uniform_real_distribution<float> distX(-fogSize_.x * 0.5f, fogSize_.x * 0.5f);
	std::uniform_real_distribution<float> distY(0.0f, fogSize_.y);
	std::uniform_real_distribution<float> distZ(-fogSize_.z * 0.5f, fogSize_.z * 0.5f);

	for (int i = 0; i < emitterCount_; ++i) {
		// エミッターの位置をランダムに決定
		Vector3 emitterPosition = {
			fogCenter_.x + distX(randomEngine_),
			fogCenter_.y + distY(randomEngine_),
			fogCenter_.z + distZ(randomEngine_)};

		Transform emitterTransform = {};
		emitterTransform.translate = emitterPosition;
		emitterTransform.rotate = {0.0f, 0.0f, 0.0f};
		emitterTransform.scale = {1.0f, 1.0f, 1.0f};

		// エミッターを作成（継続発生するようにrepeat=true）
		auto emitter = std::make_unique<ParticleEmitter>(
			particle_,
			"FogSmoke",
			emitterTransform,
			particlesPerEmitter_,
			emitterFrequency_,
			true // repeat = true で継続発生
		);

		fogEmitters_.push_back(std::move(emitter));
	}
}

///=============================================================================
///                        霧効果のセットアップ
void Cloud::SetupFogEffect() {
	if (!particle_)
		return;

	// 霧らしい動きと見た目を設定
	particle_->SetBillboard(true);

	// 移動範囲（エミッター周辺の小さな範囲）
	particle_->SetTranslateRange({-1.0f, -0.5f, -1.0f}, {1.0f, 0.5f, 1.0f});

	// 初速度（風の方向に影響される）
	Vector3 windVel = {
		windDirection_.x * windStrength_ * 0.5f,
		windDirection_.y * windStrength_ * 0.2f + 0.1f, // 少し上向きに
		windDirection_.z * windStrength_ * 0.5f};
	particle_->SetVelocityRange(
		{windVel.x - 0.3f, windVel.y - 0.1f, windVel.z - 0.3f},
		{windVel.x + 0.3f, windVel.y + 0.2f, windVel.z + 0.3f});

	// 色（グレー系の煙色）
	Vector4 smokeColorMin = {0.6f, 0.6f, 0.6f, 0.2f * fogDensity_};
	Vector4 smokeColorMax = {0.9f, 0.9f, 0.9f, 0.5f * fogDensity_};
	particle_->SetColorRange(smokeColorMin, smokeColorMax);

	// 生存時間（長めに設定して霧らしく）
	particle_->SetLifetimeRange(3.0f, 8.0f);

	// スケール（開始時は小さく、終了時は大きく）
	particle_->SetInitialScaleRange({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f});
	particle_->SetEndScaleRange({3.0f, 3.0f, 3.0f}, {5.0f, 5.0f, 5.0f});

	// 回転（ゆっくりと回転）
	particle_->SetInitialRotationRange({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	particle_->SetEndRotationRange({-0.2f, -0.2f, -0.2f}, {0.2f, 0.2f, 0.2f});

	// 重力（風の抵抗を表現）
	Vector3 gravity = {
		windDirection_.x * windStrength_ * 0.1f,
		-0.5f, // 軽い下向きの力
		windDirection_.z * windStrength_ * 0.1f};
	particle_->SetGravity(gravity);

	// フェードイン・アウト（自然な出現と消失）
	particle_->SetFadeInOut(0.2f, 0.6f);
}

///=============================================================================
///                        デバッグ描画（霧のエリアと風向きの可視化）
void Cloud::DrawDebugVisualization() {
	LineManager *lineManager = LineManager::GetInstance();
	if (!lineManager)
		return;

	// 霧のエリア（境界ボックス）を描画
	lineManager->DrawBox(fogCenter_, fogSize_, areaColor_, 2.0f);

	// エミッター位置にマーカーを描画（小さな点）
	for (const auto &emitter : fogEmitters_) {
		if (emitter) {
			// エミッター位置を取得（ParticleEmitterに位置取得機能が必要）
			// 今回は霧の範囲内にランダム配置されているので、小さなマーカーで代用
			// lineManager->DrawSphere(emitterPos, 0.2f, {0.0f, 1.0f, 0.0f, 0.5f}, 8, 1.0f);
		}
	}

	// 風向きの矢印を描画（霧の中心から風の方向へ）
	Vector3 windEnd = {
		fogCenter_.x + windDirection_.x * windArrowLength_,
		fogCenter_.y + windDirection_.y * windArrowLength_,
		fogCenter_.z + windDirection_.z * windArrowLength_};

	// 風向き矢印（太めに描画）
	lineManager->DrawArrow(fogCenter_, windEnd, windColor_, 0.15f, 3.0f);

	// 風の強さを表す追加の矢印（複数描画で強さを表現）
	int strengthIndicators = static_cast<int>(windStrength_ / 2.0f) + 1;
	for (int i = 0; i < strengthIndicators && i < 5; ++i) {
		float offset = (i + 1) * 2.0f;
		Vector3 offsetStart = {
			fogCenter_.x + windDirection_.x * offset,
			fogCenter_.y + windDirection_.y * offset + i * 0.5f,
			fogCenter_.z + windDirection_.z * offset};
		Vector3 offsetEnd = {
			offsetStart.x + windDirection_.x * (windArrowLength_ * 0.6f),
			offsetStart.y + windDirection_.y * (windArrowLength_ * 0.6f),
			offsetStart.z + windDirection_.z * (windArrowLength_ * 0.6f)};

		Vector4 fadedColor = windColor_;
		fadedColor.w *= (1.0f - i * 0.2f); // 段々薄くなる
		lineManager->DrawArrow(offsetStart, offsetEnd, fadedColor, 0.1f, 2.0f);
	}

	// 霧の密度を表すグリッド（オプション）
	if (fogDensity_ > 0.5f) {
		// 霧が濃い場合のみ内部グリッドを表示
		float gridStep = fogSize_.x / 8.0f;
		Vector4 gridColor = areaColor_;
		gridColor.w *= 0.3f; // 薄くする

		// 縦のグリッドライン
		for (int i = -3; i <= 3; ++i) {
			Vector3 lineStart = {
				fogCenter_.x + i * gridStep,
				fogCenter_.y - fogSize_.y * 0.5f,
				fogCenter_.z - fogSize_.z * 0.5f};
			Vector3 lineEnd = {
				fogCenter_.x + i * gridStep,
				fogCenter_.y + fogSize_.y * 0.5f,
				fogCenter_.z + fogSize_.z * 0.5f};
			lineManager->DrawLine(lineStart, lineEnd, gridColor, 1.0f);
		}
	}
}

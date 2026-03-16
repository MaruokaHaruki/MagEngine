/*********************************************************************
 * \file   TrailEffect.cpp
 * \brief  トレイルエフェクト描画・統合管理クラス実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#include "TrailEffect.h"
#include "Logger.h"
#include "TrailEffectSetup.h"
#include "externals/imgui/imgui.h"
#include <stdexcept>

using namespace Logger;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void TrailEffect::Initialize(TrailEffectSetup *setup, const TrailEffectPreset &preset) {
		//========================================
		// 引数チェック
		if (!setup) {
			throw std::runtime_error("TrailEffect: TrailEffectSetup is null.");
		}

		//========================================
		// 引数から設定を受け取る
		setup_ = setup;
		currentPreset_ = preset;

		//========================================
		// Emitterの初期化
		emitter_.Initialize(setup_);

		//========================================
		// プリセット内容をEmitterに適用
		ApplyPreset(currentPreset_);

		Log("TrailEffect initialized with preset: " + preset.name, Logger::LogLevel::Info);
	}

	///=============================================================================
	///						更新処理
	void TrailEffect::Update(float deltaTime) {
		//========================================
		// 無効化されている場合は更新をスキップ
		if (!enabled_) {
			return;
		}

		//========================================
		// Emitterを更新
		emitter_.Update(deltaTime);
	}

	///=============================================================================
	///						描画
	void TrailEffect::Draw() {
		//========================================
		// 無効化されている場合は描画をスキップ
		if (!enabled_) {
			return;
		}

		//========================================
		// Emitterを描画
		emitter_.Draw();
	}

	///=============================================================================
	///						ImGui描画
	void TrailEffect::DrawImGui() {
		ImGui::Begin(("TrailEffect: " + currentPreset_.name).c_str());

		//========================================
		// 基本設定
		ImGui::Checkbox("Enabled", &enabled_);
		ImGui::Separator();

		//========================================
		// ビジュアルパラメータ
		ImGui::ColorEdit3("Color", &currentPreset_.color.x);
		ImGui::SliderFloat("Opacity##Trail", &currentPreset_.opacity, 0.0f, 1.0f);
		ImGui::SliderFloat("Width##Trail", &currentPreset_.width, 0.1f, 10.0f);

		//========================================
		// グラデーション色（新機能）
		ImGui::Separator();
		ImGui::Text("Color Gradient");
		ImGui::ColorEdit3("Start Color##Trail", &currentPreset_.startColor.x);
		ImGui::ColorEdit3("End Color##Trail", &currentPreset_.endColor.x);
		ImGui::Separator();

		//========================================
		// 物理パラメータ
		ImGui::SliderFloat("VelocityDamping##Trail", &currentPreset_.velocityDamping, 0.0f, 1.0f);
		ImGui::SliderFloat("GravityInfluence##Trail", &currentPreset_.gravityInfluence, 0.0f, 1.0f);

		ImGui::Separator();

		//========================================
		// Transform設定
		ImGui::DragFloat3("Position##Trail", &transform_.translate.x, 0.1f);
		ImGui::DragFloat3("Scale##Trail", &transform_.scale.x, 0.01f);

		//========================================
		// 統計情報
		ImGui::Separator();
		ImGui::Text("Particle Count: %zu", GetParticleCount());
		ImGui::Text("Preset: %s", currentPreset_.name.c_str());

		//========================================
		// 操作ボタン
		if (ImGui::Button("Clear Trails")) {
			ClearTrails();
		}

		ImGui::End();
	}

	///=============================================================================
	///						プリセット適用
	void TrailEffect::ApplyPreset(const TrailEffectPreset &preset) {
		currentPreset_ = preset;

		//========================================
		// Emitterにパラメータを適用
		emitter_.SetColor(preset.color);
		emitter_.SetOpacity(preset.opacity);
		emitter_.SetWidth(preset.width);
		emitter_.SetLifeTime(preset.lifeTime);
		emitter_.SetVelocityDamping(preset.velocityDamping);
		emitter_.SetGravityInfluence(preset.gravityInfluence);

		//========================================
		// グラデーション色を適用
		emitter_.SetStartColor(preset.startColor);
		emitter_.SetEndColor(preset.endColor);

		Log("Preset applied: " + preset.name, Logger::LogLevel::Info);
	}

	///=============================================================================
	///						軌跡を生成
	void TrailEffect::EmitAt(const MagMath::Vector3 &position, const MagMath::Vector3 &velocity) {
		//========================================
		// ワールド座標に変換
		MagMath::Vector3 worldPosition = transform_.translate + position;

		//========================================
		// 軌跡を生成
		emitter_.EmitTrail(worldPosition, velocity);
	}

	///=============================================================================
	///						すべての軌跡をクリア
	void TrailEffect::ClearTrails() {
		emitter_.ClearTrails();
	}
}

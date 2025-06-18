#pragma once
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Vector3.h"
#include <memory>
#include <vector>

class Cloud {
public:
	/// \brief 初期化
	/// \param particle パーティクルシステム
	/// \param basePosition 基準位置（プレイヤーの位置）
	void Initialize(Particle *particle, const Vector3 &basePosition);

	/// \brief 更新
	/// \param playerPosition プレイヤーの現在位置
	void Update(const Vector3 &playerPosition);

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	/// \brief 雲の密度を設定
	void SetCloudDensity(float density) {
		cloudDensity_ = density;
	}

	/// \brief 雲の流れる速度を設定
	void SetCloudSpeed(float speed) {
		cloudSpeed_ = speed;
	}

	/// \brief 雲の高度範囲を設定
	void SetAltitudeRange(float minAltitude, float maxAltitude) {
		minAltitude_ = minAltitude;
		maxAltitude_ = maxAltitude;
	}

	/// \brief アフターバーナー効果の設定
	void SetAfterburnerEffect(bool enable, float intensity = 1.0f) {
		afterburnerMode_ = enable;
		afterburnerIntensity_ = intensity;
	}

	/// \brief 速度感演出の設定
	void SetSpeedEffect(float speedMultiplier) {
		speedEffectMultiplier_ = speedMultiplier;
	}

	/// \brief 雲の流れる量を設定
	void SetCloudFlowAmount(float flowAmount) {
		cloudFlowAmount_ = flowAmount;
	}

private:
	// パーティクルシステム
	Particle *particle_;

	// 雲エミッター群
	std::vector<std::unique_ptr<ParticleEmitter>> cloudEmitters_;

	// 雲の設定パラメータ
	float cloudDensity_;   // 雲の密度（エミッター数に影響）
	float cloudSpeed_;	   // 雲の流れる速度
	float minAltitude_;	   // 最低高度
	float maxAltitude_;	   // 最高高度
	float emissionRadius_; // エミッション半径

	// 追従設定
	Vector3 lastPlayerPosition_;
	float followSmoothing_; // 追従の滑らかさ

	// アフターバーナー効果関連
	bool afterburnerMode_;			// アフターバーナーモード
	float afterburnerIntensity_;	// アフターバーナー強度
	float speedEffectMultiplier_;	// 速度感演出倍率
	float cloudFlowAmount_;			// 雲の流れる量

	// 内部処理
	void CreateCloudEmitters();
	void UpdateEmitterPositions(const Vector3 &playerPosition);
	void CreateAfterburnerEmitters();
	void UpdateSpeedEffect();
};

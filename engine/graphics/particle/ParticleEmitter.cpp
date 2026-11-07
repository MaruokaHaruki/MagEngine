// Emit.cpp
#include "ParticleEmitter.h"
#include <cmath>
#include <random>
#include <stdexcept>
#define _USE_MATH_DEFINES
#include <math.h>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	ParticleEmitter::ParticleEmitter(Particle *particle, const std::string &name, const MagMath::Transform &transform, uint32_t count, float frequency, bool repeat)
		: particle_(particle), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat) {
		if(!particle_) { throw std::invalid_argument("ParticleEmitter " + name_ + ": particle must not be null"); }
		if(!std::isfinite(frequency_) || frequency_ <= 0.0f) {
			Logger::Log("ParticleEmitter " + name_ + ": invalid frequency; repeat disabled.", Logger::LogLevel::Error);
			repeat_ = false;
		}
		Emit(); // 既存の生成時即時発生を維持する
	}

	void ParticleEmitter::Update(float deltaTime) {
		// NOTE: 共有Particleの更新は所有側のSceneに限定し、Emitterの数で時間を進めない。
		if(!std::isfinite(deltaTime) || deltaTime < 0.0f) {
			Logger::Log("ParticleEmitter " + name_ + ": invalid deltaTime; update skipped.", Logger::LogLevel::Error);
			return;
		}
		if(deltaTime == 0.0f || !std::isfinite(frequency_) || frequency_ <= 0.0f) { return; }

		if(!repeat_)
			return; // 繰り返しフラグがfalseの場合は処理をスキップ

		// COMMENT: 可変 deltaTime で フレームレートに依存しない更新
		elapsedTime_ += deltaTime;

		if(elapsedTime_ >= frequency_) {
			Emit();
			elapsedTime_ -= frequency_; // 周期的に実行するためリセット
		}
	}

	void ParticleEmitter::Emit() {
		// エミッター位置からパーティクルを生成
		particle_->Emit(name_, transform_.translate, count_);
	}

	void ParticleEmitter::SetRepeat(bool repeat) {
		repeat_ = repeat;
	}

	ParticleEmitter &ParticleEmitter::ApplyConfig(const ParticleConfig &config) {
		// 形状設定
		SetParticleShape(config.shape);
		SetRingRadius(config.ringRadius);
		SetCylinderParams(config.cylinderHeight, config.cylinderRadius);

		// 基本設定
		SetTranslateRange(config.translateMin, config.translateMax);
		SetVelocityRange(config.velocityMin, config.velocityMax);

		// スケール設定
		SetInitialScaleRange(config.initialScaleMin, config.initialScaleMax);
		SetEndScaleRange(config.endScaleMin, config.endScaleMax);

		// 回転設定
		SetInitialRotationRange(config.initialRotationMin, config.initialRotationMax);
		SetEndRotationRange(config.endRotationMin, config.endRotationMax);

		// 色設定
		SetColorRange(config.colorMin, config.colorMax);

		// その他
		SetLifetimeRange(config.lifetimeMin, config.lifetimeMax);
		SetGravity(config.gravity);
		SetFadeInOut(config.fadeInRatio, config.fadeOutRatio);
		SetBillboard(config.billboard);

		return *this;
	}
}

// Emit.cpp
#include "ParticleEmitter.h"
#include <cmath>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

ParticleEmitter::ParticleEmitter(Particle *particle, const std::string &name, const MagMath::Transform &transform, uint32_t count, float frequency, bool repeat)
	: particle_(particle), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat) {
	Emit(); // 初期化時に即時発生
}

void ParticleEmitter::Update() {
	// パーティクルの更新
	particle_->Update();

	if (!repeat_)
		return; // 繰り返しフラグがfalseの場合は処理をスキップ

	// 時間経過によるエミット処理
	elapsedTime_ += 1.0f / 60.0f; // フレーム単位の経過時間を加算

	if (elapsedTime_ >= frequency_) {
		Emit();
		elapsedTime_ -= frequency_; // 周期的に実行するためリセット
	}
}

void ParticleEmitter::Draw() {
	// 取得したパーティクルの描画はParticleクラスに任せない
	particle_->Draw();
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

// Emit.cpp
#include "ParticleEmitter.h"
#include "AffineTransformations.h"
#include "Camera.h"
#include "MathFunc4x4.h"
#include "TextureManager.h"
#include <cmath>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

ParticleEmitter::ParticleEmitter(Particle *particle, const std::string &name, const Transform &transform, uint32_t count, float frequency, bool repeat)
	: particle_(particle), name_(name), transform_(transform), count_(count), frequency_(frequency), elapsedTime_(frequency), repeat_(repeat) {
	Emit(); // 初期化時に即時発生
}

void ParticleEmitter::Update() {
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
}

void ParticleEmitter::Emit() {
	// エミッター位置からパーティクルを生成
	particle_->Emit(name_, transform_.translate, count_);

	// エミット時のイベントログ
	// Log("Emitted " + std::to_string(count_) + " particles for group: " + name_);
}

void ParticleEmitter::SetRepeat(bool repeat) {
	repeat_ = repeat;
}

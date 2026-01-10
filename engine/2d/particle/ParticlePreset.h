/*********************************************************************
 * \file   ParticlePreset.h
 * \brief  パーティクルのプリセット定義
 *
 * \author Harukichimaru
 * \date   January 2025
 *********************************************************************/
#pragma once
#include "Particle.h"
#include "MagMath.h"
/// パーティクル設定構造体
struct ParticleConfig {
	// 形状設定
	ParticleShape shape = ParticleShape::Board;
	float ringRadius = 1.0f;
	float cylinderHeight = 1.0f;
	float cylinderRadius = 0.5f;

	// 基本設定
	MagMath::Vector3 translateMin = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 translateMax = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 velocityMin = {-0.1f, -0.1f, -0.1f};
	MagMath::Vector3 velocityMax = {0.1f, 0.1f, 0.1f};

	// スケール設定
	MagMath::Vector3 initialScaleMin = {1.0f, 1.0f, 1.0f};
	MagMath::Vector3 initialScaleMax = {1.0f, 1.0f, 1.0f};
	MagMath::Vector3 endScaleMin = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 endScaleMax = {0.0f, 0.0f, 0.0f};

	// 回転設定
	MagMath::Vector3 initialRotationMin = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 initialRotationMax = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 endRotationMin = {0.0f, 0.0f, 0.0f};
	MagMath::Vector3 endRotationMax = {0.0f, 0.0f, 0.0f};

	// 色設定
	MagMath::Vector4 colorMin = {1.0f, 1.0f, 1.0f, 1.0f};
	MagMath::Vector4 colorMax = {1.0f, 1.0f, 1.0f, 1.0f};

	// その他
	float lifetimeMin = 1.0f;
	float lifetimeMax = 3.0f;
	MagMath::Vector3 gravity = {0.0f, -9.8f, 0.0f};
	float fadeInRatio = 0.1f;
	float fadeOutRatio = 0.1f;
	bool billboard = true;
};

/// パーティクルプリセット集
namespace ParticlePresets {
	/// 炎エフェクト
	inline ParticleConfig Fire() {
		ParticleConfig config;
		config.shape = ParticleShape::Board;
		config.velocityMin = {-0.2f, 1.0f, -0.2f};
		config.velocityMax = {0.2f, 2.0f, 0.2f};
		config.initialScaleMin = {0.3f, 0.3f, 0.3f};
		config.initialScaleMax = {0.6f, 0.6f, 0.6f};
		config.endScaleMin = {0.1f, 0.1f, 0.1f};
		config.endScaleMax = {0.2f, 0.2f, 0.2f};
		config.colorMin = {1.0f, 0.3f, 0.0f, 1.0f};
		config.colorMax = {1.0f, 0.8f, 0.0f, 1.0f};
		config.lifetimeMin = 1.0f;
		config.lifetimeMax = 2.0f;
		config.gravity = {0.0f, 0.5f, 0.0f};
		config.billboard = true;
		return config;
	}

	/// 煙エフェクト
	inline ParticleConfig Smoke() {
		ParticleConfig config;
		config.shape = ParticleShape::Board;
		config.velocityMin = {-0.3f, 0.5f, -0.3f};
		config.velocityMax = {0.3f, 1.0f, 0.3f};
		config.initialScaleMin = {0.5f, 0.5f, 0.5f};
		config.initialScaleMax = {0.8f, 0.8f, 0.8f};
		config.endScaleMin = {1.5f, 1.5f, 1.5f};
		config.endScaleMax = {2.0f, 2.0f, 2.0f};
		config.colorMin = {0.5f, 0.5f, 0.5f, 0.3f};
		config.colorMax = {0.7f, 0.7f, 0.7f, 0.5f};
		config.lifetimeMin = 2.0f;
		config.lifetimeMax = 4.0f;
		config.gravity = {0.0f, 0.2f, 0.0f};
		config.fadeInRatio = 0.2f;
		config.fadeOutRatio = 0.5f;
		config.billboard = true;
		return config;
	}

	/// 爆発エフェクト
	inline ParticleConfig Explosion() {
		ParticleConfig config;
		config.shape = ParticleShape::Board;
		config.velocityMin = {-2.0f, -2.0f, -2.0f};
		config.velocityMax = {2.0f, 2.0f, 2.0f};
		config.initialScaleMin = {0.5f, 0.5f, 0.5f};
		config.initialScaleMax = {1.0f, 1.0f, 1.0f};
		config.endScaleMin = {0.0f, 0.0f, 0.0f};
		config.endScaleMax = {0.1f, 0.1f, 0.1f};
		config.colorMin = {1.0f, 0.5f, 0.0f, 1.0f};
		config.colorMax = {1.0f, 1.0f, 0.3f, 1.0f};
		config.lifetimeMin = 0.5f;
		config.lifetimeMax = 1.5f;
		config.gravity = {0.0f, -5.0f, 0.0f};
		config.billboard = true;
		return config;
	}

	/// 魔法陣エフェクト
	inline ParticleConfig MagicCircle() {
		ParticleConfig config;
		config.shape = ParticleShape::Ring;
		config.ringRadius = 2.0f;
		config.velocityMin = {0.0f, 0.0f, 0.0f};
		config.velocityMax = {0.0f, 0.0f, 0.0f};
		config.initialScaleMin = {0.5f, 0.5f, 0.5f};
		config.initialScaleMax = {0.8f, 0.8f, 0.8f};
		config.endScaleMin = {1.5f, 1.5f, 1.5f};
		config.endScaleMax = {2.0f, 2.0f, 2.0f};
		config.colorMin = {0.5f, 0.0f, 1.0f, 1.0f};
		config.colorMax = {1.0f, 0.5f, 1.0f, 1.0f};
		config.lifetimeMin = 2.0f;
		config.lifetimeMax = 3.0f;
		config.gravity = {0.0f, 0.0f, 0.0f};
		config.billboard = false;
		return config;
	}

	/// 円柱エフェクト
	inline ParticleConfig CylinderBeam() {
		ParticleConfig config;
		config.shape = ParticleShape::Cylinder;
		config.cylinderHeight = 2.0f;
		config.cylinderRadius = 0.3f;
		config.velocityMin = {0.0f, 0.0f, 0.0f};
		config.velocityMax = {0.0f, 0.0f, 0.0f};
		config.initialScaleMin = {1.0f, 1.0f, 1.0f};
		config.initialScaleMax = {1.0f, 1.0f, 1.0f};
		config.endScaleMin = {0.5f, 0.5f, 0.5f};
		config.endScaleMax = {0.8f, 0.8f, 0.8f};
		config.colorMin = {0.0f, 0.5f, 1.0f, 0.8f};
		config.colorMax = {0.5f, 1.0f, 1.0f, 1.0f};
		config.lifetimeMin = 1.0f;
		config.lifetimeMax = 2.0f;
		config.gravity = {0.0f, 0.0f, 0.0f};
		config.billboard = false;
		return config;
	}
}

/*********************************************************************
 * \file   Player.h
 *
 * \author Harukichimaru
 * \date   May 2025
 * \note
 *********************************************************************/
#pragma once
#include "BaseObject.h"
#include "Input.h"
#include "Object3d.h"
#include "ParticleEmitter.h"
#include "PlayerBullet.h"
#include <memory>
#include <string>
#include <vector>

class Object3dSetup;

class Player : public BaseObject {
public:
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath);
	void Update();
	void Draw();
	void DrawImGui();
	void DrawBullets();
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	Vector3 GetPosition() const {
		return obj_->GetPosition();
	}
	Object3d *GetObject3d() const {
		return obj_.get();
	}
	const std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() const {
		return bullets_;
	}

	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

private:
	void UpdateMovement();
	void ProcessMovementInput(bool pressW, bool pressS, bool pressA, bool pressD);
	void UpdateVelocity();
	void UpdatePosition();
	void UpdateRotation();
	void ProcessShooting();
	void UpdateBullets();
	void UpdateJetSmoke();

	std::unique_ptr<Object3d> obj_;

	// 移動関連
	Vector3 currentVelocity_;
	Vector3 targetVelocity_;
	Vector3 targetRotationEuler_;
	float moveSpeed_;
	float acceleration_;
	float rotationSmoothing_;
	float maxRollAngle_;
	float maxPitchAngle_;

	// 弾関連
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	Object3dSetup *object3dSetup_;
	float shootCoolTime_;
	float maxShootCoolTime_;

	// パーティクル関連
	Particle *particleSystem_;
	ParticleSetup *particleSetup_;
	std::unique_ptr<ParticleEmitter> jetSmokeEmitter_;

	friend class FollowCamera;
};
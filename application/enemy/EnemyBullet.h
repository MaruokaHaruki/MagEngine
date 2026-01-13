#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "BaseObject.h"
#include "Object3d.h"
#include "Object3dSetup.h"
#include "Particle.h"
#include "ParticleSetup.h"
#include <memory>
#include <string>

namespace EnemyBulletConstants {
	constexpr float kSpeed = 35.0f; // 25.0f から 35.0f に変更
	constexpr float kRadius = 0.5f;
	constexpr float kLifeTime = 5.0f;
}

///=============================================================================
///						EnemyBulletクラス
class EnemyBullet : public BaseObject {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position, const Vector3 &direction);

	/// \brief パーティクルシステムの設定
	void SetParticleSystem(MagEngine::Particle *particle, MagEngine::ParticleSetup *particleSetup);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief 生存確認
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief 位置取得
	Vector3 GetPosition() const {
		return transform_.translate;
	}

	/// \brief 半径取得
	float GetRadius() const {
		return radius_;
	}

	/// \brief 衝突処理
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

	///--------------------------------------------------------------
	///							メンバ変数
private:
	std::unique_ptr<MagEngine::Object3d> obj_;
	Transform transform_;
	Vector3 velocity_;
	float radius_;
	float lifeTimer_;
	bool isAlive_;
	MagEngine::Particle *particle_;
	MagEngine::ParticleSetup *particleSetup_;
};

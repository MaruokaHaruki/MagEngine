#pragma once
#include "BaseObject.h"
#include "Object3d.h"
#include "Vector3.h"
#include <memory>
#include <string>

// Forward declarations
class Object3dSetup;
class Particle;
class ParticleSetup;

class Enemy : public BaseObject {
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position);

	/// \brief パーティクルシステムの設定
	void SetParticleSystem(Particle *particle, ParticleSetup *particleSetup);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	/// \brief 生存フラグの取得
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief 削除フラグの設定
	void SetDead() {
		isAlive_ = false;
	}

	/// \brief 位置の取得
	Vector3 GetPosition() const;

	/// \brief 当たり判定の半径を取得
	float GetRadius() const {
		return radius_;
	}

	// BaseObjectの純粋仮想関数を実装
	void OnCollisionEnter(BaseObject *other) override;
	void OnCollisionStay(BaseObject *other) override;
	void OnCollisionExit(BaseObject *other) override;

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 velocity_;
	float speed_;
	bool isAlive_;
	float radius_;		  // 当たり判定用の半径
	float rotationSpeed_; // 回転速度

	// パーティクル関連
	Particle *particle_;
	ParticleSetup *particleSetup_;
	bool particleCreated_;

	// 破壊演出関連
	enum class DestroyState {
		Alive,		// 生存中
		Destroying, // 破壊中（パーティクル再生中）
		Dead		// 完全に消滅
	};
	DestroyState destroyState_;
	float destroyTimer_;
	float destroyDuration_; // 破壊演出の持続時間
};

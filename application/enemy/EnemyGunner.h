#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "EnemyBase.h"
#include "EnemyBullet.h"
#include <memory>
#include <vector>

namespace EnemyGunnerConstants {
	constexpr int kDefaultHP = 2;
	constexpr float kDefaultSpeed = 15.0f;
	constexpr float kShootingDistance = 35.0f;
	constexpr float kShootInterval = 1.5f;
	constexpr float kApproachSpeed = 18.0f;
	constexpr float kRetreatSpeed = 20.0f;
	constexpr float kCombatDuration = 15.0f;
	constexpr float kCombatDepth = 45.0f; // マイナスを削除
	constexpr float kCombatRadius = 40.0f;
}

///=============================================================================
///						EnemyGunnerクラス
class EnemyGunner : public EnemyBase {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize(MagEngine::Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position) override;

	/// \brief 更新
	void Update() override;

	/// \brief 描画
	void Draw() override;

	/// \brief ImGui描画
	void DrawImGui() override;

	/// \brief 弾リストの取得
	std::vector<std::unique_ptr<EnemyBullet>> &GetBullets() {
		return bullets_;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	enum class GunnerState {
		Approach,
		Shooting,
		Retreat
	};

	GunnerState state_;
	float shootTimer_;
	float combatTimer_;
	float moveTimer_;
	Vector3 targetPosition_;
	Vector3 combatCenter_;
	std::vector<std::unique_ptr<EnemyBullet>> bullets_;
	MagEngine::Object3dSetup *object3dSetup_;
};

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
	
	//! ジャスト回避判定用パラメータ
	//! プレイヤーが敵弾を引き付けて回避する距離
	constexpr float kJustAvoidanceDetectionRadius = 5.0f; // 検出範囲（接近中のみ判定）
	constexpr float kJustAvoidanceMinDistance = 1.5f;      // 最小判定距離（この距離以内で判定開始）
	constexpr float kJustAvoidanceWindow = 0.25f;          // ジャスト判定窓（秒）
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
	///							ジャスト回避関連
	/// @brief 前フレームの位置を取得（接近判定用）
	Vector3 GetPreviousPosition() const {
		return previousPosition_;
	}

	/// @brief 距離を計算
	/// @param targetPos ターゲット位置
	/// @return ターゲットとの距離
	float CalculateDistance(const Vector3 &targetPos) const {
		Vector3 diff = transform_.translate - targetPos;
		return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
	}

	/// @brief 接近中かどうか（ジャスト判定用）
	/// @param targetPos ターゲット位置
	/// @return true: 接近中、false: 遠ざかり中
	bool IsApproaching(const Vector3 &targetPos) const {
		float currentDistance = CalculateDistance(targetPos);
		Vector3 previousDiff = previousPosition_ - targetPos;
		float previousDistance = std::sqrt(previousDiff.x * previousDiff.x + 
										  previousDiff.y * previousDiff.y + 
										  previousDiff.z * previousDiff.z);
		return currentDistance < previousDistance;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	std::unique_ptr<MagEngine::Object3d> obj_;
	Transform transform_;
	Vector3 velocity_;
	Vector3 previousPosition_;  //! 前フレームの位置（接近判定用）
	float radius_;
	float lifeTimer_;
	bool isAlive_;
	MagEngine::Particle *particle_;
	MagEngine::ParticleSetup *particleSetup_;
};

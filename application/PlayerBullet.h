#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include <memory>
#include <string>

// Forward declarations
class Object3dSetup;

class PlayerBullet {
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelPath, const Vector3 &position, const Vector3 &direction);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

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

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 velocity_;
	float speed_;
	float lifeTime_;
	float maxLifeTime_;
	bool isAlive_;
	float radius_; // 当たり判定用の半径
};

#pragma once
#include "Object3d.h"
#include <memory>

class Skydome {
public:
	/// \brief 初期化
	void Initialize(Object3dSetup *object3dSetup, const std::string &modelName);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

private:
	std::unique_ptr<Object3d> object3d_;
};
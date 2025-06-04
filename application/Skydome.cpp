#include "Skydome.h"

void Skydome::Initialize(Object3dSetup *object3dSetup, const std::string &modelName) {
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(object3dSetup);
	object3d_->SetModel(modelName);

	// スカイドームのトランスフォーム設定（通常はスケールを大きくする）
	Transform transform;
	transform.scale = {100.0f, 100.0f, 100.0f};
	transform.rotate = {0.0f, 0.0f, 0.0f};
	transform.translate = {0.0f, 0.0f, 0.0f};
	object3d_->SetTransform(transform);
}

void Skydome::Update() {
	if (object3d_) {
		object3d_->Update();
	}
}

void Skydome::Draw() {
	if (object3d_) {
		object3d_->Draw();
	}
}

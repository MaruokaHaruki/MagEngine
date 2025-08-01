#include "Skydome.h"
#include "ImguiSetup.h"

void Skydome::Initialize(Object3dSetup *object3dSetup, const std::string &modelName) {
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(object3dSetup);
	object3d_->SetModel(modelName);

	// スカイドームのトランスフォーム設定（通常はスケールを大きくする）
	Transform transform;
	transform.scale = {1.0f, 1.0f, 1.0f};
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

void Skydome::DrawImGui() {
	// オブジェクトのImgui操作
	ImGui::Begin("Skydome Debug");
	// 移動
	ImGui::DragFloat3("Position", &object3d_->GetTransform()->translate.x, 0.1f);
	// 回転
	ImGui::DragFloat3("Rotation", &object3d_->GetTransform()->rotate.x, 0.1f);
	// スケール
	ImGui::DragFloat3("Scale", &object3d_->GetTransform()->scale.x, 0.1f);
	ImGui::End();
}

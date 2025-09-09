#include "Ball.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include "Vector4.h"
#include <algorithm>

///=============================================================================
///						初期化
void Ball::Initialize() {
	// デフォルト値は既にヘッダで設定済み
}

///=============================================================================
///						更新
void Ball::Update() {
	// 重力を加速度に適用（Y軸負方向）
	acceleration_.y = -gravity_;

	// 速度に加速度を適用
	velocity_.x += acceleration_.x * deltaTime_;
	velocity_.y += acceleration_.y * deltaTime_;

	// 位置に速度を適用
	position_.x += velocity_.x * deltaTime_;
	position_.y += velocity_.y * deltaTime_;

	// 境界での跳ね返りチェック
	// 左右の境界
	if (position_.x - radius_ <= boundsMinX_) {
		position_.x = boundsMinX_ + radius_;
		velocity_.x = -velocity_.x * restitution_;
		velocity_.x *= (1.0f - friction_); // 摩擦適用
	} else if (position_.x + radius_ >= boundsMaxX_) {
		position_.x = boundsMaxX_ - radius_;
		velocity_.x = -velocity_.x * restitution_;
		velocity_.x *= (1.0f - friction_); // 摩擦適用
	}

	// 上下の境界
	if (position_.y - radius_ <= boundsMinY_) {
		position_.y = boundsMinY_ + radius_;
		velocity_.y = -velocity_.y * restitution_;
		velocity_.x *= (1.0f - friction_); // 地面摩擦
	} else if (position_.y + radius_ >= boundsMaxY_) {
		position_.y = boundsMaxY_ - radius_;
		velocity_.y = -velocity_.y * restitution_;
	}

	// 空気抵抗（微小）
	velocity_.x *= 0.999f;
	velocity_.y *= 0.999f;
}

///=============================================================================
///						描画
void Ball::Draw() {
	// ワイヤーフレーム描画
	DrawWireframe();
	// 境界描画
	DrawBounds();
}

///=============================================================================
///						境界範囲設定
void Ball::SetBounds(float minX, float maxX, float minY, float maxY) {
	boundsMinX_ = minX;
	boundsMaxX_ = maxX;
	boundsMinY_ = minY;
	boundsMaxY_ = maxY;
}

///=============================================================================
///						ワイヤーフレーム描画
void Ball::DrawWireframe() {
	LineManager *lineManager = LineManager::GetInstance();
	if (!lineManager)
		return;

	// 2D平面での円として描画（Z=0固定）
	Vector3 center2D = {position_.x, position_.y, 0.0f};
	Vector3 normal = {0.0f, 0.0f, 1.0f}; // Z軸方向の法線

	// ボールを円として描画
	lineManager->DrawCircle(center2D, radius_, ballColor_, 1.0f, normal, circleSegments_);

	// ボールの中心に小さな十字を描画
	float crossSize = radius_ * 0.3f;
	lineManager->DrawLine(
		{center2D.x - crossSize, center2D.y, center2D.z},
		{center2D.x + crossSize, center2D.y, center2D.z},
		ballColor_);
	lineManager->DrawLine(
		{center2D.x, center2D.y - crossSize, center2D.z},
		{center2D.x, center2D.y + crossSize, center2D.z},
		ballColor_);
}

///=============================================================================
///						境界描画
void Ball::DrawBounds() {
	LineManager *lineManager = LineManager::GetInstance();
	if (!lineManager)
		return;

	// 境界の4つの壁を線で描画（2D矩形）
	// 下辺
	lineManager->DrawLine(
		{boundsMinX_, boundsMinY_, 0.0f},
		{boundsMaxX_, boundsMinY_, 0.0f},
		boundsColor_, 2.0f);

	// 上辺
	lineManager->DrawLine(
		{boundsMinX_, boundsMaxY_, 0.0f},
		{boundsMaxX_, boundsMaxY_, 0.0f},
		boundsColor_, 2.0f);

	// 左辺
	lineManager->DrawLine(
		{boundsMinX_, boundsMinY_, 0.0f},
		{boundsMinX_, boundsMaxY_, 0.0f},
		boundsColor_, 2.0f);

	// 右辺
	lineManager->DrawLine(
		{boundsMaxX_, boundsMinY_, 0.0f},
		{boundsMaxX_, boundsMaxY_, 0.0f},
		boundsColor_, 2.0f);

	// 境界の角に小さなマーカーを描画
	float markerSize = 0.5f;
	Vector4 cornerColor = {1.0f, 1.0f, 0.0f, 1.0f}; // 黄色

	// 四隅のマーカー
	Vector3 corners[4] = {
		{boundsMinX_, boundsMinY_, 0.0f}, // 左下
		{boundsMaxX_, boundsMinY_, 0.0f}, // 右下
		{boundsMaxX_, boundsMaxY_, 0.0f}, // 右上
		{boundsMinX_, boundsMaxY_, 0.0f}  // 左上
	};

	for (int i = 0; i < 4; ++i) {
		lineManager->DrawLine(
			{corners[i].x - markerSize, corners[i].y, corners[i].z},
			{corners[i].x + markerSize, corners[i].y, corners[i].z},
			cornerColor);
		lineManager->DrawLine(
			{corners[i].x, corners[i].y - markerSize, corners[i].z},
			{corners[i].x, corners[i].y + markerSize, corners[i].z},
			cornerColor);
	}
}

///=============================================================================
///						ImGUI描画
void Ball::DrawImGui() {
	ImGui::Begin("Ball Controller");

	//========================================
	// 物理設定
	ImGui::SeparatorText("Physics Settings");

	// 位置
	ImGui::DragFloat3("Position", &position_.x, 0.1f);

	// 速度
	ImGui::DragFloat3("Velocity", &velocity_.x, 0.1f);

	// 物理パラメータ
	ImGui::SliderFloat("Radius", &radius_, 0.1f, 5.0f);
	ImGui::SliderFloat("Mass", &mass_, 0.1f, 10.0f);
	ImGui::SliderFloat("Restitution", &restitution_, 0.0f, 1.0f);
	ImGui::SliderFloat("Friction", &friction_, 0.0f, 1.0f);
	ImGui::SliderFloat("Gravity", &gravity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Delta Time", &deltaTime_, 1.0f / 120.0f, 1.0f / 30.0f, "%.4f");

	//========================================
	// 境界設定
	ImGui::SeparatorText("Bounds Settings");

	ImGui::SliderFloat("Min X", &boundsMinX_, -50.0f, 0.0f);
	ImGui::SliderFloat("Max X", &boundsMaxX_, 0.0f, 50.0f);
	ImGui::SliderFloat("Min Y", &boundsMinY_, -50.0f, 0.0f);
	ImGui::SliderFloat("Max Y", &boundsMaxY_, 0.0f, 50.0f);

	//========================================
	// 描画設定
	ImGui::SeparatorText("Render Settings");

	ImGui::ColorEdit4("Ball Color", &ballColor_.x);
	ImGui::ColorEdit4("Bounds Color", &boundsColor_.x);
	ImGui::SliderInt("Circle Segments", &circleSegments_, 8, 64);

	//========================================
	// 操作ボタン
	ImGui::SeparatorText("Controls");

	// リセットボタン
	if (ImGui::Button("Reset Position")) {
		position_ = {0.0f, 0.0f, 0.0f};
		velocity_ = {0.0f, 0.0f, 0.0f};
	}

	ImGui::SameLine();

	// ランダム速度
	if (ImGui::Button("Random Velocity")) {
		velocity_.x = (rand() % 21 - 10) * 0.5f; // -5.0 to 5.0
		velocity_.y = (rand() % 21 - 10) * 0.5f;
	}

	// プリセットボタン
	if (ImGui::Button("Basketball")) {
		radius_ = 1.2f;
		restitution_ = 0.8f;
		friction_ = 0.1f;
		ballColor_ = {1.0f, 0.5f, 0.0f, 1.0f};
	}

	ImGui::SameLine();

	if (ImGui::Button("Tennis Ball")) {
		radius_ = 0.7f;
		restitution_ = 0.7f;
		friction_ = 0.2f;
		ballColor_ = {1.0f, 1.0f, 0.0f, 1.0f};
	}

	ImGui::SameLine();

	if (ImGui::Button("Bowling Ball")) {
		radius_ = 1.5f;
		restitution_ = 0.3f;
		friction_ = 0.8f;
		mass_ = 7.0f;
		ballColor_ = {0.2f, 0.2f, 0.2f, 1.0f};
	}

	//========================================
	// 情報表示
	ImGui::SeparatorText("Information");

	float speed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y);
	ImGui::Text("Speed: %.2f", speed);

	float kineticEnergy = 0.5f * mass_ * speed * speed;
	ImGui::Text("Kinetic Energy: %.2f", kineticEnergy);

	ImGui::Text("Position: (%.2f, %.2f)", position_.x, position_.y);

	ImGui::End();
}

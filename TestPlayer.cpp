#include "TestPlayer.h"
#include "DebugTextManager.h"
#include "Input.h"
#include "LineManager.h"
#include "imgui.h"

void TestPlayer::Initialize() {
	// 初期位置を原点付近に設定
	position_ = {0.0f, 0.0f};
	velocity_ = {0.0f, 0.0f};
	size_ = {1.0f, 1.0f}; // 小さいサイズに変更
	speed_ = 5.0f;		  // 3D空間に適したスピードに調整

	// ジャンプ・重力関連の初期化
	jumpPower_ = 15.0f;		  // ジャンプ力
	gravity_ = 30.0f;		  // 重力の強さ
	verticalVelocity_ = 0.0f; // 垂直方向の速度
	isGrounded_ = true;		  // 初期状態では地面に接触
	groundLevel_ = 0.0f;	  // 地面のレベル

	// FPS関連の初期化
	targetFPS_ = 60.0f;
	deltaTime_ = 1.0f / 60.0f;
	lastFrameTime_ = std::chrono::high_resolution_clock::now();
	updateAccumulator_ = 0.0f;

	// デバッグ用の初期化
	currentFPS_ = 0.0f;
	frameCount_ = 0;
	fpsTimer_ = 0.0f;
}

void TestPlayer::Update() {
	// 60FPS固定のメインループを前提とした処理
	const float baseFPS = 60.0f;
	const float baseFrameTime = 1.0f / baseFPS;

	// 目標FPSに基づく更新間隔の計算
	float targetFrameTime = 1.0f / targetFPS_;

	// アキュムレータに固定フレーム時間を加算
	updateAccumulator_ += baseFrameTime;

	// 目標フレーム時間に達していない場合は更新をスキップ
	if (updateAccumulator_ < targetFrameTime) {
		return;
	}

	// 更新を実行し、アキュムレータから目標フレーム時間を減算
	updateAccumulator_ -= targetFrameTime;
	// アキュムレータが大きくなりすぎないように制限
	if (updateAccumulator_ > targetFrameTime) {
		updateAccumulator_ = 0.0f;
	}

	// 目標FPSに応じたデルタタイムを設定
	deltaTime_ = targetFrameTime;

	// FPS計算（実際の更新頻度を計測）
	frameCount_++;
	fpsTimer_ += baseFrameTime;
	if (fpsTimer_ >= 1.0f) {
		currentFPS_ = frameCount_ / fpsTimer_;
		frameCount_ = 0;
		fpsTimer_ = 0.0f;
	}

	// キーボード入力による移動制御
	velocity_ = {0.0f, 0.0f};

	// 水平移動
	if (Input::GetInstance()->PushKey(DIK_A) || Input::GetInstance()->PushKey(DIK_LEFT)) {
		velocity_.x = -speed_;
	}
	if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_RIGHT)) {
		velocity_.x = speed_;
	}

	// ジャンプ処理（Wキー）
	if ((Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_SPACE)) && isGrounded_) {
		verticalVelocity_ = jumpPower_;
		isGrounded_ = false;
	}

	// 下移動（Sキー、地面にいる時のみ）
	if ((Input::GetInstance()->PushKey(DIK_S) || Input::GetInstance()->PushKey(DIK_DOWN)) && isGrounded_) {
		// 地面にいる時は特に何もしない（将来的にしゃがみ等を実装可能）
	}

	// 重力の適用
	if (!isGrounded_) {
		verticalVelocity_ -= gravity_ * deltaTime_;
	}

	// 水平位置を更新
	position_.x += velocity_.x * deltaTime_;

	// 垂直位置を更新
	position_.y += verticalVelocity_ * deltaTime_;

	// 地面との衝突判定
	if (position_.y <= groundLevel_) {
		position_.y = groundLevel_;
		verticalVelocity_ = 0.0f;
		isGrounded_ = true;
	}

	// デバッグテキストの表示
	Vector3 playerPos3D = {position_.x, position_.y, 0.0f};
	char fpsText[128];
	sprintf_s(fpsText, "FPS: %.1f/%.1f\nPos: (%.2f, %.2f)\nSpeed: %.1f\nGrounded: %s\nVVel: %.2f",
			  currentFPS_, targetFPS_, position_.x, position_.y, speed_,
			  isGrounded_ ? "Yes" : "No", verticalVelocity_);

	DebugTextManager::GetInstance()->AddText3D(
		fpsText,
		{playerPos3D.x + 1.5f, playerPos3D.y + 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f, 1.0f},
		0.1f,
		0.6f);
}

void TestPlayer::Draw() {
	// ボックスの四角形をラインで描画
	Vector2 topLeft = {position_.x - size_.x / 2, position_.y + size_.y / 2};
	Vector2 topRight = {position_.x + size_.x / 2, position_.y + size_.y / 2};
	Vector2 bottomLeft = {position_.x - size_.x / 2, position_.y - size_.y / 2};
	Vector2 bottomRight = {position_.x + size_.x / 2, position_.y - size_.y / 2};

	// Vector2をVector3に変換してLineManagerに渡す
	Vector3 topLeft3D = {topLeft.x, topLeft.y, 0.0f};
	Vector3 topRight3D = {topRight.x, topRight.y, 0.0f};
	Vector3 bottomLeft3D = {bottomLeft.x, bottomLeft.y, 0.0f};
	Vector3 bottomRight3D = {bottomRight.x, bottomRight.y, 0.0f};

	Vector4 boxColor = {1.0f, 1.0f, 1.0f, 1.0f};

	// 上辺
	LineManager::GetInstance()->DrawLine(topLeft3D, topRight3D, boxColor);
	// 右辺
	LineManager::GetInstance()->DrawLine(topRight3D, bottomRight3D, boxColor);
	// 下辺
	LineManager::GetInstance()->DrawLine(bottomRight3D, bottomLeft3D, boxColor);
	// 左辺
	LineManager::GetInstance()->DrawLine(bottomLeft3D, topLeft3D, boxColor);
}

void TestPlayer::DrawImGui() {
	// ImGuiウィンドウのタイトルに目標FPSを表示
	char windowTitle[64];
	sprintf_s(windowTitle, "TestPlayer (Target: %.0f FPS)", targetFPS_);

	ImGui::Begin(windowTitle);

	// FPS設定
	ImGui::SliderFloat("Target FPS", &targetFPS_, 10.0f, 144.0f);
	ImGui::Text("Actual FPS: %.1f", currentFPS_);
	ImGui::Text("Update Rate: %.1f%%", (targetFPS_ > 0 ? (currentFPS_ / targetFPS_) * 100.0f : 0.0f));
	ImGui::Text("Delta Time: %.4f ms", deltaTime_ * 1000.0f);

	ImGui::Separator();

	// 移動・ジャンプ設定
	ImGui::SliderFloat("Speed (units/s)", &speed_, 1.0f, 20.0f);
	ImGui::SliderFloat("Jump Power", &jumpPower_, 5.0f, 30.0f);
	ImGui::SliderFloat("Gravity", &gravity_, 10.0f, 50.0f);

	ImGui::Separator();

	// 位置・状態情報
	ImGui::Text("Position: (%.2f, %.2f)", position_.x, position_.y);
	ImGui::Text("Velocity: (%.2f, %.2f)", velocity_.x, velocity_.y);
	ImGui::Text("Vertical Velocity: %.2f", verticalVelocity_);
	ImGui::Text("Grounded: %s", isGrounded_ ? "Yes" : "No");

	ImGui::Separator();

	// 操作説明
	ImGui::Text("Controls:");
	ImGui::Text("A/D or ←/→: Move horizontally");
	ImGui::Text("W or ↑: Jump (when grounded)");

	// 位置リセットボタン
	if (ImGui::Button("Reset Position")) {
		position_ = {0.0f, 0.0f};
		verticalVelocity_ = 0.0f;
		isGrounded_ = true;
	}

	ImGui::End();
}

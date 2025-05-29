#include "TestPlayer.h"
#include "Input.h"
#include "LineManager.h"

void TestPlayer::Initialize() {
	// 初期位置を画面中央に設定
	position_ = {640.0f, 360.0f};
	velocity_ = {0.0f, 0.0f};
	size_ = {50.0f, 50.0f};
	speed_ = 5.0f;
}

void TestPlayer::Update() {
	// キーボード入力による移動制御
	velocity_ = {0.0f, 0.0f};

	if (Input::GetInstance()->PushKey(DIK_A) || Input::GetInstance()->PushKey(DIK_LEFT)) {
		velocity_.x = -speed_;
	}
	if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_RIGHT)) {
		velocity_.x = speed_;
	}
	if (Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_UP)) {
		velocity_.y = -speed_;
	}
	if (Input::GetInstance()->PushKey(DIK_S) || Input::GetInstance()->PushKey(DIK_DOWN)) {
		velocity_.y = speed_;
	}

	// 位置を更新
	position_.x += velocity_.x;
	position_.y += velocity_.y;
}

void TestPlayer::Draw() {
	// ボックスの四角形をラインで描画
	Vector2 topLeft = {position_.x - size_.x / 2, position_.y - size_.y / 2};
	Vector2 topRight = {position_.x + size_.x / 2, position_.y - size_.y / 2};
	Vector2 bottomLeft = {position_.x - size_.x / 2, position_.y + size_.y / 2};
	Vector2 bottomRight = {position_.x + size_.x / 2, position_.y + size_.y / 2};

	// 上辺
	LineManager::GetInstance()->DrawLine(topLeft, topRight, 0xFFFFFFFF);
	// 右辺
	LineManager::GetInstance()->DrawLine(topRight, bottomRight, 0xFFFFFFFF);
	// 下辺
	LineManager::GetInstance()->DrawLine(bottomRight, bottomLeft, 0xFFFFFFFF);
	// 左辺
	LineManager::GetInstance()->DrawLine(bottomLeft, topLeft, 0xFFFFFFFF);
}

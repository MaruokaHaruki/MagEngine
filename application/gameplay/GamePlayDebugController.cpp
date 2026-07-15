#include "GamePlayDebugController.h"

#ifdef _DEBUG

#include "Cloud.h"
#include "Input.h"
#include "Logger.h"
#include "Player.h"
#include <cmath>

namespace {
constexpr int kRandomOffsetXMin = -100;
constexpr int kRandomOffsetXMax = 100;
constexpr int kRandomOffsetYMin = -20;
constexpr int kRandomOffsetYMax = 20;
constexpr int kRandomOffsetZMin = -100;
constexpr int kRandomOffsetZMax = 100;
constexpr float kBulletHoleDirectionScale = 0.01f;
}

void GamePlayDebugController::Initialize(MagEngine::Input *input, Player *player, MagEngine::Cloud *cloud) {
	input_ = input;
	player_ = player;
	cloud_ = cloud;
	randomEngine_.seed(std::random_device{}());
	isEnabled_ = false;
}

void GamePlayDebugController::Finalize() {
	input_ = nullptr;
	player_ = nullptr;
	cloud_ = nullptr;
	isEnabled_ = false;
}

void GamePlayDebugController::Update() {
	if (!input_) {
		return;
	}

	if (input_->TriggerKey(DIK_F10)) {
		isEnabled_ = !isEnabled_;
		Logger::Log(isEnabled_ ? "Gameplay debug input enabled" : "Gameplay debug input disabled", Logger::LogLevel::Info);
	}
	if (!isEnabled_ || !cloud_) {
		return;
	}

	if (input_->TriggerKey(DIK_J)) {
		AddPlayerBulletHole();
	}
	if (input_->TriggerKey(DIK_K)) {
		AddRandomBulletHole();
	}
	if (input_->TriggerKey(DIK_L)) {
		cloud_->ClearBulletHoles();
		Logger::Log("All bullet holes cleared", Logger::LogLevel::Info);
	}
}

void GamePlayDebugController::AddPlayerBulletHole() {
	if (!player_) {
		return;
	}

	const MagMath::Vector3 origin = player_->GetPosition() - MagMath::Vector3{0.0f, 0.0f, 100.0f};
	const float yaw = player_->GetTransform()->rotate.y;
	MagMath::Vector3 forward{std::sin(yaw), 0.0f, std::cos(yaw)};
	forward = MagMath::Normalize(forward);
	cloud_->AddBulletHole(origin, forward, 16.0f, 8.0f, 700.0f, 2.0f);
	Logger::Log("BulletHole added at player position", Logger::LogLevel::Info);
}

void GamePlayDebugController::AddRandomBulletHole() {
	std::uniform_int_distribution<int> xDistribution(kRandomOffsetXMin, kRandomOffsetXMax);
	std::uniform_int_distribution<int> yDistribution(kRandomOffsetYMin, kRandomOffsetYMax);
	std::uniform_int_distribution<int> zDistribution(kRandomOffsetZMin, kRandomOffsetZMax);
	std::uniform_int_distribution<int> directionDistribution(-100, 100);

	const MagMath::Vector3 center = cloud_->GetTransform().translate;
	const MagMath::Vector3 origin{
		center.x + static_cast<float>(xDistribution(randomEngine_)),
		center.y + static_cast<float>(yDistribution(randomEngine_)),
		center.z + static_cast<float>(zDistribution(randomEngine_))};
	MagMath::Vector3 direction{
		static_cast<float>(directionDistribution(randomEngine_)) * kBulletHoleDirectionScale,
		static_cast<float>(directionDistribution(randomEngine_)) * kBulletHoleDirectionScale,
		static_cast<float>(directionDistribution(randomEngine_)) * kBulletHoleDirectionScale};
	if (MagMath::Length(direction) <= 0.001f) {
		direction = {0.0f, 0.0f, 1.0f};
	} else {
		direction = MagMath::Normalize(direction);
	}
	cloud_->AddBulletHole(origin, direction, 2.0f, 0.2f, 300.0f, 20.0f);
	Logger::Log("Random BulletHole added", Logger::LogLevel::Info);
}

#endif

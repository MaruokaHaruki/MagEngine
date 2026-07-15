#pragma once

#ifdef _DEBUG

#include <random>

namespace MagEngine {
class Cloud;
class Input;
}

class Player;

/// @brief GamePlayScene専用のデバッグ入力を管理するクラス
class GamePlayDebugController {
public:
	void Initialize(MagEngine::Input *input, Player *player, MagEngine::Cloud *cloud);
	void Finalize();
	void Update();

private:
	void AddPlayerBulletHole();
	void AddRandomBulletHole();

	MagEngine::Input *input_ = nullptr;
	Player *player_ = nullptr;
	MagEngine::Cloud *cloud_ = nullptr;
	std::mt19937 randomEngine_;
	bool isEnabled_ = false;
};

#endif

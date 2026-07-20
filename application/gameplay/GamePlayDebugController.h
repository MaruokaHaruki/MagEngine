#pragma once

#ifdef _DEBUG

#include <random>

namespace MagEngine {
class Cloud;
class Input;
}

class Player;

/// @brief GamePlayScene専用のデバッグ入力を管理するクラス
/// @details リリースビルドへデバッグ操作を持ち込まないため、クラス定義を含めて_DEBUG時だけに存在する。
///          所有権は持たず、Sceneの終了前にFinalize()で依存を解除する。
class GamePlayDebugController {
public:
	/// @brief デバッグ入力に必要な外部オブジェクトを設定する
	/// @note すべて非所有参照。Update()を行う間は有効でなければならない。
	void Initialize(MagEngine::Input *input, Player *player, MagEngine::Cloud *cloud);
	/// @brief 参照先の破棄前に依存を解除する
	void Finalize();
	/// @brief F10で有効化した入力操作を処理する
	/// @note クラウドが未設定の場合、トグル以外の操作は行わない。
	void Update();

private:
	/// @brief プレイヤー前方を基準にクラウドへ弾痕を追加する
	/// @note プレイヤー未設定時は何もしない。
	void AddPlayerBulletHole();
	/// @brief クラウド周辺のランダムな位置と方向へ弾痕を追加する
	void AddRandomBulletHole();

	MagEngine::Input *input_ = nullptr; // GamePlaySceneが所有する入力サービスへの非所有参照
	Player *player_ = nullptr;           // プレイヤー基準の弾痕生成に使う非所有参照
	MagEngine::Cloud *cloud_ = nullptr;  // 弾痕の追加・削除先となる非所有参照
	std::mt19937 randomEngine_;          // 他システムの乱数状態を汚染しない専用生成器
	bool isEnabled_ = false;             // F10で明示的に有効化した場合だけ操作を受け付ける
};

#endif

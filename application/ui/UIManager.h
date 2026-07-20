#pragma once
#include "GameClearAnimation.h"
#include "GameOverUI.h"
#include "HUD.h"
#include "LockOnHUD.h"
#include "MenuUI.h"
#include "Object3dSetup.h"
#include "OperationGuideUI.h"
#include "Sprite.h"
#include "SpriteSetup.h"
#include "StartAnimation.h"
#include <memory>

namespace MagEngine {
	class Input;
	class CameraManager;
	class LineManager;
	class RenderWorld;
	class TextRenderer;
}

///=============================================================================
///                        UI管理クラス
class UIManager {
public:
	/// \brief 全UI要素を生成し、描画・入力に必要な外部サービスを設定する
	/// @param spriteSetup 2Dスプライト生成に使用するセットアップ
	/// @param object3dSetup 3D表示を伴うUI初期化に使用するセットアップ
	/// @param input メニューと操作ガイドに使用する入力
	/// @param cameraManager HUD座標変換に使用するカメラ管理
	/// @param lineManager HUDとロックオンHUDの線描画管理
	/// @param textRenderer HUD文字列の描画管理
	void Initialize(MagEngine::SpriteSetup *spriteSetup,
					MagEngine::Object3dSetup *object3dSetup,
					MagEngine::Input &input,
					MagEngine::CameraManager &cameraManager,
					MagEngine::LineManager &lineManager,
					MagEngine::TextRenderer &textRenderer);

	/// \brief 全UI要素を終了し、保持する参照を解除する
	void Finalize();

	/// \brief プレイヤー状態とゲーム状態に応じて各UI要素を更新する
	/// @param player HUDへ反映するプレイヤー状態
	/// @param unscaledDeltaTime 時間停止の影響を受けない経過時間（秒）
	void Update(const class Player *player, float unscaledDeltaTime);

	/// \brief 各UIのスプライトとテキストを描画対象へ登録する
	/// @param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 各UI要素のデバッグImGuiを描画する
	void DrawImGui();

	///--------------------------------------------------------------
	///                        UI要素のアクセッサ
	/// \brief ゲームオーバーUIを取得する
	/// @return 管理中のGameOverUI。未初期化時はnullptr
	GameOverUI *GetGameOverUI() {
		return gameOverUI_.get();
	}

	/// \brief ゲームクリア演出を取得する
	/// @return 管理中のGameClearAnimation。未初期化時はnullptr
	GameClearAnimation *GetGameClearAnimation() {
		return gameClearAnimation_.get();
	}

	/// \brief 操作ガイドUIを取得する
	/// @return 管理中のOperationGuideUI。未初期化時はnullptr
	OperationGuideUI *GetOperationGuideUI() {
		return operationGuideUI_.get();
	}

	/// \brief 開始演出を取得する
	/// @return 管理中のStartAnimation。未初期化時はnullptr
	StartAnimation *GetStartAnimation() {
		return startAnimation_.get();
	}

	/// \brief HUDを取得する
	/// @return 管理中のHUD。未初期化時はnullptr
	HUD *GetHUD() {
		return hud_.get();
	}

	/// \brief メニューUIを取得する
	/// @return 管理中のMenuUI。未初期化時はnullptr
	MenuUI *GetMenuUI() {
		return menuUI_.get();
	}

	/// \brief ロックオンHUDを取得する
	/// @return 管理中のLockOnHUD。未初期化時はnullptr
	LockOnHUD *GetLockOnHUD() {
		return lockOnHUD_.get();
	}

	///--------------------------------------------------------------
	///                        ゲーム状態管理
	/// \brief ゲームオーバー状態を設定する
	/// @param isGameOver trueの場合はゲームオーバーUI更新経路を有効にする
	void SetGameOver(bool isGameOver) {
		isGameOver_ = isGameOver;
	}

	/// \brief ゲームクリア状態を設定する
	/// @param isGameClear trueの場合はクリア演出更新経路を有効にする
	void SetGameClear(bool isGameClear) {
		isGameClear_ = isGameClear;
	}

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// スプライト設定
	MagEngine::SpriteSetup *spriteSetup_ = nullptr;
	MagEngine::CameraManager *cameraManager_ = nullptr;
	MagEngine::TextRenderer *textRenderer_ = nullptr;

	// UI要素
	std::unique_ptr<GameOverUI> gameOverUI_;
	std::unique_ptr<GameClearAnimation> gameClearAnimation_;
	std::unique_ptr<OperationGuideUI> operationGuideUI_;
	std::unique_ptr<StartAnimation> startAnimation_;
	std::unique_ptr<HUD> hud_;
	std::unique_ptr<MenuUI> menuUI_;
	std::unique_ptr<LockOnHUD> lockOnHUD_;

	// ゲーム状態
	bool isGameOver_ = false;
	bool isGameClear_ = false;
};

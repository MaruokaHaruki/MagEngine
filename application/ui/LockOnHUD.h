#pragma once
#include "MagMath.h"
using namespace MagMath;
#include "Camera.h"
#include <memory>
#include <vector>

// 前方宣言
class Player;
class EnemyBase;
class EnemyManager;
namespace MagEngine {
	class LineManager;
}

///=============================================================================
///                        ロックオンHUDクラス
/// @brief エースコンバット風のロックオンHUD
/// @details
/// - すべての敵の位置に三角形マーカーを表示
/// - ロックオン中の敵は特別な色でハイライト
/// - スムーズなアニメーションと効果で、スタイリッシュなUIを提供
class LockOnHUD {
	///--------------------------------------------------------------
	///                        メンバ関数
public:
	/// @brief 初期化
	/// @param player プレイヤーポインタ
	/// @param enemyManager 敵マネージャーポインタ
	void Initialize(Player *player, EnemyManager *enemyManager);

	/// @brief 終了処理
	void Finalize();

	/// @brief 更新処理
	void Update(MagEngine::Camera *camera);

	/// @brief 描画処理
	void Draw();

	/// @brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///                        機能制御
	/// @brief ロックオンHUDの表示/非表示
	void SetVisible(bool visible) {
		isVisible_ = visible;
	}

	/// @brief 表示状態の取得
	bool IsVisible() const {
		return isVisible_;
	}

	///--------------------------------------------------------------
private:
	/// @brief 敵マーカーの描画
	/// @param enemy 敵ポインタ
	/// @param camera カメラ
	/// @param isLocked ロックオン中かどうか
	void DrawEnemyMarker(EnemyBase *enemy, MagEngine::Camera *camera, bool isLocked);

	/// @brief 敵の4コーナーブラケットを描画（ビルボード対応）
	/// @param markerPos マーカー位置（3D）
	/// @param cameraPos カメラ位置（ビルボード用）
	/// @param size マーカーサイズ
	/// @param color マーカー色
	/// @param isLocked ロック状態
	/// @param accentColor アクセントカラー（ロック時ドット用）
	void DrawBracketMarker(const Vector3 &markerPos, const Vector3 &cameraPos, float size, const Vector4 &color, bool isLocked = false, const Vector4 &accentColor = Vector4{0.0f, 0.0f, 0.0f, 0.0f});

	/// @brief ロックオンラインの描画（複数敵への結線）
	void DrawLockOnLines();

	/// @brief 影響範囲のディスプレイ設定を取得
	float GetMarkerAlpha(float distance, float maxRange) const;

	/// @brief パルスアニメーション値を取得
	float GetPulseValue() const;

	///--------------------------------------------------------------
	///                        メンバ変数
private:
	// 参照
	Player *player_ = nullptr;
	EnemyManager *enemyManager_ = nullptr;
	MagEngine::LineManager *lineManager_ = nullptr;

	// 表示設定
	bool isVisible_ = true;

	// アニメーション用
	float pulseTime_ = 0.0f;
	float pulseSpeed_ = 4.0f; // パルス周期

	// マーカー設定
	Vector4 normalMarkerColor_ = {0.9f, 0.7f, 0.15f, 0.75f}; // ダムアンバー（未ロック）
	Vector4 lockOnMarkerColor_ = {1.0f, 0.0f, 0.0f, 1.0f}; // 鮮烈レッド（ロック時）
	Vector4 lockOnAccentColor_ = {1.0f, 0.3f, 0.3f, 1.0f};	 // ライトレッド（ロック中心ドット）
	Vector4 warningMarkerColor_ = {1.0f, 0.5f, 0.0f, 0.7f};	 // 警告色（未ロック敵）

	float markerSize_ = 1.0f;
	float lockOnMarkerSize_ = 2.0f;
	float markerDistance_ = 100.0f; // マーカーをカメラから何ユニット先に配置するか

	// スクリーン設定
	float screenWidth_ = 1280.0f;
	float screenHeight_ = 720.0f;

	// グロー効果
	float glowIntensity_ = 0.0f;
	float glowPulseSpeed_ = 6.0f;

	// トラッキング範囲
	float trackingRange_ = 500.0f; // ロックオン可能な最大距離

	// ImGui用デバッグ設定
	struct DebugSettings {
		bool showAllMarkers = true;
		bool showLockOnLines = false; // ロックオンライン非表示
		bool enableAnimation = true;
	} debugSettings_;
};

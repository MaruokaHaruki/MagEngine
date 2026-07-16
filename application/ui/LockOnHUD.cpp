#define _USE_MATH_DEFINES
#define NOMINMAX
#include "LockOnHUD.h"
#include "../enemy/base/EnemyBase.h"
#include "../enemy/manager/EnemyManager.h"
#include "../player/Player.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

namespace {
	class ScopedLineRenderMode {
	public:
		ScopedLineRenderMode(MagEngine::LineManager &lineManager, MagEngine::LineRenderMode renderMode)
			: lineManager_(lineManager),
			  previousRenderMode_(lineManager.GetRenderMode()) {
			lineManager_.SetRenderMode(renderMode);
		}

		~ScopedLineRenderMode() {
			lineManager_.SetRenderMode(previousRenderMode_);
		}

	private:
		MagEngine::LineManager &lineManager_;
		MagEngine::LineRenderMode previousRenderMode_;
	};

	class ScopedLockOnHudLineSource {
	public:
		explicit ScopedLockOnHudLineSource(MagEngine::LineManager &lineManager)
			: lineManager_(lineManager) {
			lineManager_.BeginHudLineSource(true);
		}

		~ScopedLockOnHudLineSource() {
			lineManager_.EndHudLineSource();
		}

	private:
		MagEngine::LineManager &lineManager_;
	};
}

///=============================================================================
///                        初期化
void LockOnHUD::Initialize(Player *player, EnemyManager *enemyManager) {
	player_ = player;
	enemyManager_ = enemyManager;

	isVisible_ = true;
	pulseTime_ = 0.0f;

	// デバッグ設定のデフォルト値
	debugSettings_.showAllMarkers = false;
	debugSettings_.showLockOnLines = false; // ロックオンライン非表示
	debugSettings_.enableAnimation = true;
}

///=============================================================================
///                        終了処理
void LockOnHUD::Finalize() {
	player_ = nullptr;
	enemyManager_ = nullptr;
	lineManager_ = nullptr;
	currentCamera_ = nullptr;
}

///=============================================================================
///                        更新処理
void LockOnHUD::Update(MagEngine::Camera *camera, float unscaledDeltaTime) {
	if (!isVisible_ || !player_ || !enemyManager_) {
		return;
	}
	currentCamera_ = camera;

	// アニメーション更新
	if (debugSettings_.enableAnimation) {
		pulseTime_ += unscaledDeltaTime;
		if (pulseTime_ > 2.0f * M_PI / pulseSpeed_) {
			pulseTime_ = 0.0f;
		}
	}
}

///=============================================================================
///                        描画処理
void LockOnHUD::Draw() {
	if (!isVisible_ || !player_ || !enemyManager_ || !lineManager_) {
		return;
	}

	// カメラを取得
	MagEngine::Camera *camera = currentCamera_;
	if (!camera) {
		return;
	}

	// ロックオン情報を取得
	const auto &lockedEnemyHandles = player_->GetAllLockOnTargetHandles();
	const bool isLockOnMode = player_->IsMissileLockOnMode();
	EnemyBase *aimingTarget = enemyManager_->ResolveEnemy(player_->GetAimingLockOnTargetHandle());

	// ロックオンモード外でロック済みもない場合は描画しない
	if (!isLockOnMode && lockedEnemyHandles.empty()) {
		return;
	}
	ScopedLineRenderMode lineModeGuard(*lineManager_, LineRenderMode::Hud);
	ScopedLockOnHudLineSource lineSourceGuard(*lineManager_);
	lineManager_->NotifyLockOnHudUpdate();

	// すべての敵を取得
	const auto &allEnemies = enemyManager_->GetEnemies();

	// デバッグ時のみ、すべての敵にマーカーを描画
	if (debugSettings_.showAllMarkers) {
		for (const auto &enemyPtr : allEnemies) {
			EnemyBase *enemy = enemyPtr.get();
			if (!enemy || !enemy->IsAlive()) {
				continue;
			}

			// ロック対象かチェック
			bool isLocked = std::find(lockedEnemyHandles.begin(), lockedEnemyHandles.end(), enemy->GetHandle()) != lockedEnemyHandles.end();

			DrawEnemyMarker(enemy, camera, isLocked);
		}
	}

	// ロック済みターゲットは常に描画
	for (EnemyHandle handle : lockedEnemyHandles) {
		EnemyBase *enemy = enemyManager_->ResolveEnemy(handle);
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}
		DrawEnemyMarker(enemy, camera, true);
	}

	// 長押し中の照準候補（未ロック）を描画
	if (isLockOnMode && aimingTarget &&
		std::find(lockedEnemyHandles.begin(), lockedEnemyHandles.end(), aimingTarget->GetHandle()) == lockedEnemyHandles.end()) {
		DrawEnemyMarker(aimingTarget, camera, false);
	}

	// ロックオンラインを描画
	if (debugSettings_.showLockOnLines && !lockedEnemyHandles.empty()) {
		DrawLockOnLines();
	}
}

///=============================================================================
///                        敵マーカーの描画
void LockOnHUD::DrawEnemyMarker(EnemyBase *enemy, MagEngine::Camera *camera, bool isLocked) {
	if (!enemy || !camera) {
		return;
	}

	Vector3 enemyPos = enemy->GetPosition();
	Vector3 cameraPos = camera->GetTransform().translate;
	Vector3 relativePos = enemyPos - cameraPos;

	// カメラから敵への距離
	float distance = Length(relativePos);

	// 追跡範囲外なら描画しない
	if (distance > trackingRange_) {
		return;
	}

	// 敵の方向ベクトル（カメラからの相対位置）
	Vector3 cameraDirection = Normalize(relativePos);

	// カメラの前方から所定の距離の位置にマーカーを配置
	Vector3 markerPos = cameraPos + cameraDirection * markerDistance_;

	// マーカーのサイズと色を決定
	float size = isLocked ? lockOnMarkerSize_ : markerSize_;

	Vector4 baseColor = isLocked ? lockOnMarkerColor_ : normalMarkerColor_;
	Vector4 accentColor = lockOnAccentColor_;

	// アルファ値の調整（距離に基づく）
	float alpha = GetMarkerAlpha(distance, trackingRange_);
	Vector4 finalColor = baseColor;
	finalColor.w *= alpha;

	// アクセントカラーにも透明度を適用
	Vector4 finalAccentColor = accentColor;
	finalAccentColor.w *= alpha;

	// ブラケットマーカーを描画（ビルボード対応）
	DrawBracketMarker(markerPos, cameraPos, size, finalColor, isLocked, finalAccentColor);
}

///=============================================================================
///                        4コーナーブラケットマーカー（ビルボード対応）
void LockOnHUD::DrawBracketMarker(const Vector3 &markerPos, const Vector3 &cameraPos, float size, const Vector4 &color, bool isLocked, const Vector4 &accentColor) {
	if (!lineManager_) {
		return;
	}

	// ---- ビルボード用基底ベクトルを構築 ----
	auto Cross = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	};

	Vector3 toCam = Normalize(cameraPos - markerPos);
	Vector3 upDir = {0.0f, 1.0f, 0.0f};
	Vector3 rightDir = Cross(upDir, toCam);
	if (Length(rightDir) < 0.001f) {
		rightDir = {1.0f, 0.0f, 0.0f};
	} else {
		rightDir = Normalize(rightDir);
	}
	upDir = Normalize(Cross(toCam, rightDir));

	// ---- 4コーナーL字ブラケットを描く汎用ラムダ ----
	auto DrawCorners = [&](float s, const Vector4 &col, float thick) {
		float arm = s * 0.52f; // アームの長さ (ブラケット辺の比率)
		MagEngine::LineStyle style{};
		style.mode = LineRenderMode::Hud;
		style.color = col;
		style.thickness = thick;

		// 左上
		Vector3 tl = markerPos - rightDir * s + upDir * s;
		lineManager_->DrawLine(tl, tl + rightDir * arm, style);
		lineManager_->DrawLine(tl, tl - upDir * arm, style);
		// 右上
		Vector3 tr = markerPos + rightDir * s + upDir * s;
		lineManager_->DrawLine(tr, tr - rightDir * arm, style);
		lineManager_->DrawLine(tr, tr - upDir * arm, style);
		// 左下
		Vector3 bl = markerPos - rightDir * s - upDir * s;
		lineManager_->DrawLine(bl, bl + rightDir * arm, style);
		lineManager_->DrawLine(bl, bl + upDir * arm, style);
		// 右下
		Vector3 br = markerPos + rightDir * s - upDir * s;
		lineManager_->DrawLine(br, br - rightDir * arm, style);
		lineManager_->DrawLine(br, br + upDir * arm, style);
	};

	// ========== 未ロック：薄いアンバーの細いブラケット ==========
	if (!isLocked) {
		DrawCorners(size, color, 1.5f);
	}
	// ========== ロック時：内側タイトブラケット＋外パルス＋中心ドット ==========
	else {
		// 内側（タイトで太い）
		DrawCorners(size * 0.72f, color, 2.0f);

		// 外側パルスブラケット
		if (debugSettings_.enableAnimation) {
			float pulse = 0.25f + 0.35f * sinf(pulseTime_ * pulseSpeed_);
			Vector4 outerCol = color;
			outerCol.w = pulse;
			DrawCorners(size * 1.3f, outerCol, 1.5f);
		}

		// 中心ドット (小さな十字)
		float dotR = size * 0.17f;
		Vector4 dotCol = accentColor;
		dotCol.w = 1.0f;
		MagEngine::LineStyle dotStyle{};
		dotStyle.mode = LineRenderMode::Hud;
		dotStyle.color = dotCol;
		dotStyle.thickness = 2.5f;
		lineManager_->DrawLine(markerPos - rightDir * dotR, markerPos + rightDir * dotR, dotStyle);
		lineManager_->DrawLine(markerPos - upDir * dotR, markerPos + upDir * dotR, dotStyle);
	}
}

///=============================================================================
///                        ロックオンラインの描画
void LockOnHUD::DrawLockOnLines() {
	if (!player_ || !lineManager_) {
		return;
	}

	Vector3 playerPos = player_->GetPosition();
	const auto &lockedEnemyHandles = player_->GetAllLockOnTargetHandles();

	// プレイヤーからすべてのロック敵へのラインを描画
	for (EnemyHandle handle : lockedEnemyHandles) {
		EnemyBase *enemy = enemyManager_ ? enemyManager_->ResolveEnemy(handle) : nullptr;
		if (!enemy || !enemy->IsAlive()) {
			continue;
		}

		Vector3 enemyPos = enemy->GetPosition();

		// プレイヤーから敵へのライン
		Vector4 lineColor = lockOnAccentColor_;

		// パルス効果で透明度を変更
		if (debugSettings_.enableAnimation) {
			float pulse = 0.4f + 0.5f * sinf(pulseTime_ * glowPulseSpeed_);
			lineColor.w = lockOnAccentColor_.w * pulse;
		}

		// ロックライン（グリーン系で目立たせる）
		lineManager_->DrawLine(playerPos, enemyPos, lineColor, 1.5f);
	}
}

///=============================================================================
///                        距離に基づくアルファ値を取得
float LockOnHUD::GetMarkerAlpha(float distance, float maxRange) const {
	// 距離が遠いほど薄くなる
	float alpha = 1.0f - (distance / maxRange);
	return std::max(0.2f, alpha); // 最小透明度は0.2
}

///=============================================================================
///                        パルスアニメーション値を取得
float LockOnHUD::GetPulseValue() const {
	return 0.5f + 0.5f * sinf(pulseTime_ * pulseSpeed_);
}

///=============================================================================
///                        ImGui描画
void LockOnHUD::DrawImGui() {
#ifdef _DEBUG
	{
		ImGui::Checkbox("Visible##LockOnHUD", &isVisible_);
		ImGui::Checkbox("Show All Markers", &debugSettings_.showAllMarkers);
		ImGui::Checkbox("Show LockOn Lines", &debugSettings_.showLockOnLines);
		ImGui::Checkbox("Enable Animation", &debugSettings_.enableAnimation);

		ImGui::Separator();

		ImGui::DragFloat("Marker Size##LockOn", &markerSize_, 1.0f, 3.0f, 50.0f);
		ImGui::DragFloat("LockOn Marker Size##LockOn", &lockOnMarkerSize_, 1.0f, 5.0f, 60.0f);

		ImGui::DragFloat("Marker Distance", &markerDistance_, 5.0f, 10.0f, 200.0f);
		ImGui::DragFloat("Tracking Range", &trackingRange_, 10.0f, 100.0f, 1000.0f);

		ImGui::DragFloat("Pulse Speed##LockOn", &pulseSpeed_, 0.1f, 1.0f, 10.0f);

		ImGui::Separator();

		ImGui::ColorEdit4("Normal Marker Color", &normalMarkerColor_.x);
		ImGui::ColorEdit4("LockOn Marker Color", &lockOnMarkerColor_.x);
		ImGui::ColorEdit4("LockOn Accent Color", &lockOnAccentColor_.x);

		if (player_) {
			ImGui::Separator();
			ImGui::Text("LockOn Mode: %s", player_->IsMissileLockOnMode() ? "ACTIVE" : "INACTIVE");
			ImGui::Text("LockOn Targets: %zu", player_->GetLockOnTargetCount());
		}

	}
#endif
}

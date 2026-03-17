#define _USE_MATH_DEFINES
#define NOMINMAX
#include "LockOnHUD.h"
#include "../enemy/EnemyBase.h"
#include "../enemy/EnemyManager.h"
#include "../player/Player.h"
#include "CameraManager.h"
#include "ImguiSetup.h"
#include "LineManager.h"
#include <algorithm>
#include <cmath>
using namespace MagEngine;

///=============================================================================
///                        初期化
void LockOnHUD::Initialize(Player *player, EnemyManager *enemyManager) {
	player_ = player;
	enemyManager_ = enemyManager;
	lineManager_ = LineManager::GetInstance();

	isVisible_ = true;
	pulseTime_ = 0.0f;

	// デバッグ設定のデフォルト値
	debugSettings_.showAllMarkers = true;
	debugSettings_.showLockOnLines = false; // ロックオンライン非表示
	debugSettings_.enableAnimation = true;
}

///=============================================================================
///                        終了処理
void LockOnHUD::Finalize() {
	player_ = nullptr;
	enemyManager_ = nullptr;
	lineManager_ = nullptr;
}

///=============================================================================
///                        更新処理
void LockOnHUD::Update(MagEngine::Camera *camera) {
	if (!isVisible_ || !player_ || !enemyManager_) {
		return;
	}

	// アニメーション更新
	if (debugSettings_.enableAnimation) {
		pulseTime_ += 1.0f / 60.0f; // 60FPS想定
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
	MagEngine::Camera *camera = CameraManager::GetInstance()->GetCamera("FollowCamera");
	if (!camera) {
		return;
	}

	// ロックオン情報を取得
	const auto &lockedEnemies = player_->GetAllLockOnTargets();

	// すべての敵を取得
	const auto &allEnemies = enemyManager_->GetEnemies();

	// すべての敵にマーカーを描画
	if (debugSettings_.showAllMarkers) {
		for (const auto &enemyPtr : allEnemies) {
			EnemyBase *enemy = enemyPtr.get();
			if (!enemy || !enemy->IsAlive()) {
				continue;
			}

			// ロック対象かチェック
			bool isLocked = std::find(lockedEnemies.begin(), lockedEnemies.end(), enemy) != lockedEnemies.end();

			DrawEnemyMarker(enemy, camera, isLocked);
		}
	}

	// ロックオンラインを描画
	if (debugSettings_.showLockOnLines && !lockedEnemies.empty()) {
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

		// 左上
		Vector3 tl = markerPos - rightDir * s + upDir * s;
		lineManager_->DrawLine(tl, tl + rightDir * arm, col, thick);
		lineManager_->DrawLine(tl, tl - upDir * arm, col, thick);
		// 右上
		Vector3 tr = markerPos + rightDir * s + upDir * s;
		lineManager_->DrawLine(tr, tr - rightDir * arm, col, thick);
		lineManager_->DrawLine(tr, tr - upDir * arm, col, thick);
		// 左下
		Vector3 bl = markerPos - rightDir * s - upDir * s;
		lineManager_->DrawLine(bl, bl + rightDir * arm, col, thick);
		lineManager_->DrawLine(bl, bl + upDir * arm, col, thick);
		// 右下
		Vector3 br = markerPos + rightDir * s - upDir * s;
		lineManager_->DrawLine(br, br - rightDir * arm, col, thick);
		lineManager_->DrawLine(br, br + upDir * arm, col, thick);
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
		lineManager_->DrawLine(markerPos - rightDir * dotR, markerPos + rightDir * dotR, dotCol, 2.5f);
		lineManager_->DrawLine(markerPos - upDir * dotR, markerPos + upDir * dotR, dotCol, 2.5f);
	}
}

///=============================================================================
///                        ロックオンラインの描画
void LockOnHUD::DrawLockOnLines() {
	if (!player_ || !lineManager_) {
		return;
	}

	Vector3 playerPos = player_->GetPosition();
	const auto &lockedEnemies = player_->GetAllLockOnTargets();

	// プレイヤーからすべてのロック敵へのラインを描画
	for (EnemyBase *enemy : lockedEnemies) {
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
	if (ImGui::Begin("LockOnHUD Settings")) {
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
			ImGui::Text("LockOn Targets: %zu", player_->GetLockOnTargetCount());
		}

		ImGui::End();
	}
#endif
}

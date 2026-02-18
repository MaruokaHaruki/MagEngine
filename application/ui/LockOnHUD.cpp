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

	// 三角形マーカーを描画（ビルボード対応）
	DrawTriangleMarker(markerPos, cameraPos, size, finalColor, isLocked, finalAccentColor);
}

///=============================================================================
///                        三角形マーカーの描画（ビルボード対応）
void LockOnHUD::DrawTriangleMarker(const Vector3 &markerPos, const Vector3 &cameraPos, float size, const Vector4 &color, bool isLocked, const Vector4 &accentColor) {
	if (!lineManager_) {
		return;
	}

	// ローカル関数: Cross積
	auto Cross = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
		return {
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x};
	};

	// マーカーからカメラへの向き（ビルボード用）
	Vector3 toCameraDir = Normalize(cameraPos - markerPos);

	// カメラの上方向を推定（Y軸を上とする）
	Vector3 upDir = {0.0f, 1.0f, 0.0f};

	// toCameraDirと上方向から右ベクトルを計算
	Vector3 rightDir = Cross(upDir, toCameraDir);
	if (Length(rightDir) > 0.0f) {
		rightDir = Normalize(rightDir);
	} else {
		// toCameraDirがY軸と平行な場合の処理
		rightDir = {1.0f, 0.0f, 0.0f};
	}

	// 最終的な上方向を再計算（直交系を保証）
	upDir = Normalize(Cross(toCameraDir, rightDir));

	// ========== 未ロック時：正三角形 ==========
	if (!isLocked) {
		// 正三角形の頂点を計算（中心：markerPos）
		// 上、左下、右下の順で60度間隔
		Vector3 vertex1 = markerPos + upDir * size;									  // 上
		Vector3 vertex2 = markerPos - rightDir * size * 0.866f - upDir * size * 0.5f; // 左下 (√3/2)
		Vector3 vertex3 = markerPos + rightDir * size * 0.866f - upDir * size * 0.5f; // 右下

		// 三角形の枠線を描画（線幅1.0px）
		lineManager_->DrawLine(vertex1, vertex2, color, 1.0f);
		lineManager_->DrawLine(vertex2, vertex3, color, 1.0f);
		lineManager_->DrawLine(vertex3, vertex1, color, 1.0f);
	}
	// ========== ロック時：ダブルマーカー（二重三角形） ==========
	else {
		float innerSize = size * 0.7f;
		float outerSize = size * 1.2f;

		// 外側の大きな正三角形
		Vector3 outer1 = markerPos + upDir * outerSize;
		Vector3 outer2 = markerPos - rightDir * outerSize * 0.866f - upDir * outerSize * 0.5f;
		Vector3 outer3 = markerPos + rightDir * outerSize * 0.866f - upDir * outerSize * 0.5f;

		// 内側の小さな正三角形
		Vector3 inner1 = markerPos + upDir * innerSize;
		Vector3 inner2 = markerPos - rightDir * innerSize * 0.866f - upDir * innerSize * 0.5f;
		Vector3 inner3 = markerPos + rightDir * innerSize * 0.866f - upDir * innerSize * 0.5f;

		// 外側の三角形を描画（太い線で目立たせる）
		lineManager_->DrawLine(outer1, outer2, color, 2.0f);
		lineManager_->DrawLine(outer2, outer3, color, 2.0f);
		lineManager_->DrawLine(outer3, outer1, color, 2.0f);

		// 内側の三角形をアクセントカラーで描画
		lineManager_->DrawLine(inner1, inner2, accentColor, 1.5f);
		lineManager_->DrawLine(inner2, inner3, accentColor, 1.5f);
		lineManager_->DrawLine(inner3, inner1, accentColor, 1.5f);

		// ロック固定を示す中央の小さな十字
		float crossSize = size * 0.25f;
		Vector4 crossColor = accentColor;
		crossColor.w *= 0.8f;

		lineManager_->DrawLine(
			markerPos - rightDir * crossSize,
			markerPos + rightDir * crossSize,
			crossColor, 1.0f);
		lineManager_->DrawLine(
			markerPos - upDir * crossSize * 0.7f,
			markerPos + upDir * crossSize * 0.7f,
			crossColor, 1.0f);

		// ロック状態を示すパルスする外枠（控えめに）
		if (debugSettings_.enableAnimation) {
			float pulse = 0.3f + 0.3f * sinf(pulseTime_ * pulseSpeed_);
			Vector4 pulseColor = color;
			pulseColor.w *= pulse;

			float pulseSize = outerSize * 1.3f;
			Vector3 pulse1 = markerPos + upDir * pulseSize;
			Vector3 pulse2 = markerPos - rightDir * pulseSize * 0.866f - upDir * pulseSize * 0.5f;
			Vector3 pulse3 = markerPos + rightDir * pulseSize * 0.866f - upDir * pulseSize * 0.5f;

			lineManager_->DrawLine(pulse1, pulse2, pulseColor, 1.0f);
			lineManager_->DrawLine(pulse2, pulse3, pulseColor, 1.0f);
			lineManager_->DrawLine(pulse3, pulse1, pulseColor, 1.0f);
		}
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

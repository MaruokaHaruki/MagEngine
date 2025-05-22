#pragma once
#include "Camera.h"
#include "Object3d.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "WinApp.h"
#include <string>
#include <unordered_map>
#include <vector>

// デバッグテキストの情報
struct DebugText {
	std::string text;		// 表示テキスト
	Vector3 worldPosition;	// 3D空間上の位置
	Vector4 color;			// テキストの色
	float scale;			// テキストスケール
	float duration;			// 表示時間（-1で無期限）
	float timer;			// 経過タイマー
	bool useScreenPosition; // true: スクリーン座標使用, false: ワールド座標使用
	Vector2 screenPosition; // スクリーン座標（useScreenPositionがtrueの時）
	void *targetObject;		// 追従対象オブジェクト（nullptrで固定位置）
};

class DebugTextManager {
public:
	// シングルトンインスタンスの取得
	static DebugTextManager *GetInstance();

	// 初期化
	void Initialize(WinApp *winApp);

	// 更新処理
	void Update();

	// ImGui描画処理
	void DrawImGui();

	// 3D空間上にテキスト追加
	void AddText3D(const std::string &text, const Vector3 &position,
				   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
				   float duration = -1.0f, float scale = 1.0f);

	// スクリーン座標にテキスト追加
	void AddTextScreen(const std::string &text, const Vector2 &position,
					   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
					   float duration = -1.0f, float scale = 1.0f);

	// オブジェクトに追従するテキスト追加
	template <typename T>
	void AddTextFollow(const std::string &text, T *object,
					   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
					   float duration = -1.0f, float scale = 1.0f);

	// 全テキストの削除
	void ClearAllTexts();

	// カメラの設定
	void SetCamera(Camera *camera) {
		camera_ = camera;
	}

	// デバッグテキスト表示切替
	void SetDebugTextEnabled(bool enabled) {
		isDebugTextEnabled_ = enabled;
	}
	bool IsDebugTextEnabled() const {
		return isDebugTextEnabled_;
	}

private:
	DebugTextManager() = default;
	~DebugTextManager() = default;
	DebugTextManager(const DebugTextManager &) = delete;
	DebugTextManager &operator=(const DebugTextManager &) = delete;

	// 3D空間座標からスクリーン座標への変換
	Vector2 WorldToScreen(const Vector3 &worldPosition) const;

	// オブジェクトから位置を取得（テンプレート特殊化で各オブジェクトタイプに対応）
	template <typename T>
	Vector3 GetObjectPosition(T *object) const;

	// メンバ変数
	static DebugTextManager *instance_;
	//========================================
	// WindowsAPI
	WinApp *winApp_ = nullptr;

	std::vector<DebugText> debugTexts_;
	Camera *camera_ = nullptr;
	bool isDebugTextEnabled_ = true;
};

// Object3dクラス用の特殊化（GetPosition関数を持つ前提）
template <>
inline Vector3 DebugTextManager::GetObjectPosition(Object3d *object) const {
	if (object) {
		return object->GetPosition();
	}
	return Vector3{0, 0, 0};
}

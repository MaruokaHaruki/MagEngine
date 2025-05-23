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

// Forward declaration for ImFont
struct ImFont;

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
	std::string fontName;	// 使用するフォント名 (空の場合はデフォルトフォント)
	bool isFixedToScreen;	// true: カメラ移動に影響されない位置に表示
	Vector2 fixedScreenPos; // 固定スクリーン位置（isFixedToScreenがtrueの時）
	bool isPersistent;		// true: テキストはClearAllTextsで削除されない
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

	// ImGuiでデバッグテキスト一覧を表示・管理するウィンドウを描画
	void DrawDebugTextManagerImGui();

	// 3D空間上にテキスト追加
	// isFixedToScreen: trueの場合、テキストはスクリーン上に固定されます（カメラ移動に影響されない）
	// isPersistent: trueの場合、ClearAllTextsでテキストが削除されません
	void AddText3D(const std::string &text, const Vector3 &position,
				   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
				   float duration = -1.0f, float scale = 1.0f,
				   const std::string &fontName = "",
				   bool isFixedToScreen = false,
				   bool isPersistent = false);

	// 3D空間上に軸名やポイント名などのデバッグ用のテキストを追加する簡易関数
	// これらは自動的に永続的なテキストになります
	void AddAxisLabels();
	void AddGridLabels(float gridSize = 5.0f, int gridCount = 4);
	void AddPointLabel(const std::string &label, const Vector3 &position,
					   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f});

	// スクリーン座標にテキスト追加
	// isPersistent: trueの場合、ClearAllTextsでテキストが削除されない
	void AddTextScreen(const std::string &text, const Vector2 &position,
					   const Vector4 &color = {1.0f, 1.0f, 1.0f, 1.0f},
					   float duration = -1.0f, float scale = 1.0f,
					   const std::string &fontName = "",
					   bool isPersistent = false);

	// 全テキストの削除（永続的なテキストを除く）
	void ClearAllTexts();

	// 全てのテキストを削除（永続的なテキストを含む）
	void ClearAllTextsIncludingPersistent();

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

	// フォントのロード
	// fontName: 管理用のフォント名, filePath: ttf/otfファイルへのパス, size: フォントサイズ
	bool LoadFont(const std::string &fontName, const std::string &filePath, float size);

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
	std::unordered_map<std::string, ImFont *> loadedFonts_; // ロード済みフォントのマップ

	// ImGui管理用フラグ
	bool showOnlyPersistent_ = false;  // 永続テキストのみ表示
	bool showOnly3DTexts_ = false;	   // 3Dテキストのみ表示
	bool showOnlyScreenTexts_ = false; // スクリーンテキストのみ表示
};

// Object3dクラス用の特殊化（GetPosition関数を持つ前提）
template <>
inline Vector3 DebugTextManager::GetObjectPosition(Object3d *object) const {
	if (object) {
		return object->GetPosition();
	}
	return Vector3{0, 0, 0};
}

#include "DebugTextManager.h"
#include "MathFunc4x4.h"
#include "WinApp.h"
#include "imgui.h"
#include <iostream> // エラー出力用 (任意)

DebugTextManager *DebugTextManager::instance_ = nullptr;

DebugTextManager *DebugTextManager::GetInstance() {
	if (!instance_) {
		instance_ = new DebugTextManager();
	}
	return instance_;
}

void DebugTextManager::Initialize(WinApp *winApp) {
	if (!winApp) {
		// エラーハンドリング: WinAppがnullptrの場合
		// ここでエラーメッセージを表示するか、例外を投げるなどの処理を行う
		return;
	}
	winApp_ = winApp;
	// すべてのテキストを確実にクリア
	ClearAllTextsIncludingPersistent();
	loadedFonts_.clear(); // ロード済みフォントもクリア

	// 指定されたフォントファイルをロードする
	std::string fontPath = "resources\\fonts\\KikaiChokokuJIS-Md.ttf";
	std::string fontKey = "kikai_chokoku"; // このフォントを識別するためのキー (任意の名前に変更可能)
	float fontSize = 16.0f;				   // フォントサイズ (適宜調整してください)

	if (!LoadFont(fontKey, fontPath, fontSize)) {
		// フォントのロードに失敗した場合の処理 (任意)
		std::cerr << "Failed to load font: " << fontPath << std::endl;
	}

	// さらに他のフォントをロードする場合は、同様に LoadFont を呼び出します。
	// 例：
	// if (!LoadFont("another_font", "C:\\Kamata_Workbench\\MagEngine\\project\\resources\\fonts\\another_font_file.otf", 20.0f)) {
	//     std::cerr << "Failed to load font: C:\\Kamata_Workbench\\MagEngine\\project\\resources\\fonts\\another_font_file.otf" << std::endl;
	// }
}

void DebugTextManager::Update() {
	if (!camera_)
		return;

	float deltaTime = 1.0f / 60.0f; // 理想的には実際のフレーム時間を使用

	// テキスト情報を更新、期限切れを削除
	for (size_t i = 0; i < debugTexts_.size();) {
		DebugText &text = debugTexts_[i];

		// タイマー更新
		if (text.duration > 0) {
			text.timer += deltaTime;
			if (text.timer >= text.duration) {
				// 期限切れのテキストを削除
				debugTexts_.erase(debugTexts_.begin() + i);
				continue;
			}
		}

		// オブジェクト追従の場合、位置を更新
		if (text.targetObject) {
			if (auto obj3d = static_cast<Object3d *>(text.targetObject)) {
				text.worldPosition = GetObjectPosition(obj3d);
			}
		}

		// 固定スクリーン位置のテキストの場合、カメラ更新に合わせて再計算しない
		// そのほかのテキストは毎フレーム位置を再計算する必要がない

		++i;
	}
}

void DebugTextManager::DrawImGui() {
	if (!isDebugTextEnabled_ || !camera_ || debugTexts_.empty())
		return;

	ImGuiIO &io = ImGui::GetIO();
	ImDrawList *drawList = ImGui::GetBackgroundDrawList();

	for (const auto &text : debugTexts_) {
		Vector2 screenPos;

		if (text.useScreenPosition) {
			// スクリーン座標指定の場合
			screenPos = text.screenPosition;
		} else if (text.isFixedToScreen) {
			// 3D座標だがスクリーンに固定する場合
			// 初回の変換位置を使用（カメラ移動の影響を受けない）
			screenPos = text.fixedScreenPos;
		} else {
			// 通常の3D→スクリーン座標変換（カメラ移動の影響を受ける）
			screenPos = WorldToScreen(text.worldPosition);

			// カメラの後ろにあるオブジェクトのテキストは表示しない
			// 簡易的なカリング判定（より正確な判定が必要なら改良すること）
			Matrix4x4 viewMatrix = camera_->GetViewMatrix();
			Vector3 viewPos = Multiply(text.worldPosition, viewMatrix);
			if (viewPos.z < 0) {
				continue;
			}
		}

		// スクリーン外のテキストはスキップ（簡易的な判定）
		if (screenPos.x < 0 || screenPos.x > io.DisplaySize.x ||
			screenPos.y < 0 || screenPos.y > io.DisplaySize.y) {
			continue;
		}

		// フェードアウト効果（オプション）
		float alpha = 1.0f;
		if (text.duration > 0) {
			alpha = 1.0f - (text.timer / text.duration);
		}

		ImColor color(text.color.x, text.color.y, text.color.z,
					  text.color.w * alpha);

		// フォントの指定があれば適用
		ImFont *currentFont = nullptr;
		if (!text.fontName.empty()) {
			auto it = loadedFonts_.find(text.fontName);
			if (it != loadedFonts_.end()) {
				currentFont = it->second;
			}
		}

		if (currentFont) {
			ImGui::PushFont(currentFont);
		} else {
			// 指定フォントがない場合やデフォルトの場合は、ImGui::GetFont()が使われる
			// (PushFontしないことでデフォルトフォントになる)
		}

		// テキスト描画
		float fontSize = (currentFont ? currentFont->FontSize : ImGui::GetFontSize()) * text.scale; // 基本フォントサイズ × スケール
		drawList->AddText(currentFont ? currentFont : ImGui::GetFont(), fontSize,					// PushFontした場合でも、ここで明示的にフォントを指定する方が安全
						  ImVec2(screenPos.x, screenPos.y),
						  color, text.text.c_str());

		if (currentFont) {
			ImGui::PopFont();
		}
	}
}

Vector2 DebugTextManager::WorldToScreen(const Vector3 &worldPosition) const {
	if (!camera_ || !winApp_)
		return Vector2{0, 0};

	// ワールド→クリップ空間への変換
	Matrix4x4 viewProjMatrix = camera_->GetViewProjectionMatrix();

	// 同次座標でのクリップ空間変換（完全な行列乗算）
	Vector4 clipPos;
	clipPos.x = worldPosition.x * viewProjMatrix.m[0][0] + worldPosition.y * viewProjMatrix.m[1][0] +
				worldPosition.z * viewProjMatrix.m[2][0] + viewProjMatrix.m[3][0];
	clipPos.y = worldPosition.x * viewProjMatrix.m[0][1] + worldPosition.y * viewProjMatrix.m[1][1] +
				worldPosition.z * viewProjMatrix.m[2][1] + viewProjMatrix.m[3][1];
	clipPos.z = worldPosition.x * viewProjMatrix.m[0][2] + worldPosition.y * viewProjMatrix.m[1][2] +
				worldPosition.z * viewProjMatrix.m[2][2] + viewProjMatrix.m[3][2];
	clipPos.w = worldPosition.x * viewProjMatrix.m[0][3] + worldPosition.y * viewProjMatrix.m[1][3] +
				worldPosition.z * viewProjMatrix.m[2][3] + viewProjMatrix.m[3][3];

	// wが0に近い場合はエラー防止（極端な位置）
	if (std::abs(clipPos.w) < 1e-6f) {
		return Vector2{-1000.0f, -1000.0f}; // 画面外の座標を返す
	}

	// 正規化デバイス座標（NDC）に変換
	float ndcX = clipPos.x / clipPos.w;
	float ndcY = clipPos.y / clipPos.w;

	// NDCからスクリーン座標へ変換
	float width = static_cast<float>(winApp_->GetWindowWidth());
	float height = static_cast<float>(winApp_->GetWindowHeight());
	Vector2 screenPos;
	screenPos.x = (ndcX + 1.0f) * width * 0.5f;
	screenPos.y = (1.0f - ndcY) * height * 0.5f; // Y軸は反転

	return screenPos;
}

void DebugTextManager::AddText3D(const std::string &text, const Vector3 &position,
								 const Vector4 &color, float duration, float scale, const std::string &fontName,
								 bool isFixedToScreen, bool isPersistent) {
	// 既存の永続的なテキストで同じ内容があれば追加しない（重複防止）
	if (isPersistent) {
		for (const auto &existingText : debugTexts_) {
			if (existingText.isPersistent &&
				existingText.text == text &&
				existingText.worldPosition.x == position.x &&
				existingText.worldPosition.y == position.y &&
				existingText.worldPosition.z == position.z) {
				return; // 同一テキストが既に存在するので追加しない
			}
		}
	}

	DebugText newText{};
	newText.text = text;
	newText.worldPosition = position;
	newText.color = color;
	newText.scale = scale;
	newText.duration = duration;
	newText.timer = 0;
	newText.useScreenPosition = false;
	newText.targetObject = nullptr;
	newText.fontName = fontName;
	newText.isFixedToScreen = isFixedToScreen;
	newText.isPersistent = isPersistent;

	// スクリーンに固定する場合は、現在のカメラ視点での位置を記録
	if (isFixedToScreen && camera_) {
		newText.fixedScreenPos = WorldToScreen(position);
	}

	debugTexts_.push_back(newText);
}

void DebugTextManager::AddTextScreen(const std::string &text, const Vector2 &position,
									 const Vector4 &color, float duration, float scale, const std::string &fontName,
									 bool isPersistent) {
	DebugText newText{};
	newText.text = text;
	newText.screenPosition = position;
	newText.color = color;
	newText.scale = scale;
	newText.duration = duration;
	newText.timer = 0;
	newText.useScreenPosition = true;
	newText.targetObject = nullptr;
	newText.fontName = fontName;
	newText.isPersistent = isPersistent;

	debugTexts_.push_back(newText);
}

void DebugTextManager::ClearAllTexts() {
	// 永続的でないテキストのみを削除
	for (size_t i = 0; i < debugTexts_.size();) {
		if (!debugTexts_[i].isPersistent) {
			debugTexts_.erase(debugTexts_.begin() + i);
		} else {
			++i;
		}
	}
}

void DebugTextManager::ClearAllTextsIncludingPersistent() {
	// すべてのテキストを削除
	debugTexts_.clear();
}

bool DebugTextManager::LoadFont(const std::string &fontName, const std::string &filePath, float size) {
	ImGuiIO &io = ImGui::GetIO();
	ImFont *font = io.Fonts->AddFontFromFileTTF(filePath.c_str(), size, nullptr, io.Fonts->GetGlyphRangesJapanese());
	if (font) {
		loadedFonts_[fontName] = font;
		// ImGuiのフォントテクスチャを再構築する必要がある場合がある
		// 通常、ImGui_ImplDX12_CreateDeviceObjectsなどで初期化時に行われるが、
		// 動的にフォントを追加した場合は手動で更新が必要になることがある。
		// unsigned char* pixels;
		// int width, height;
		// io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		// (この後、テクスチャをGPUにアップロードし、SetTexIDする処理。通常はImGuiバックエンドが行う)
		// ここでは、フォントのロードのみ行い、テクスチャの更新はImGuiのメインループに任せるか、
		// 必要に応じて明示的な更新関数を設けることを検討。
		// 簡単のため、ここではロードのみ。
		return true;
	}
	return false;
}

void DebugTextManager::AddAxisLabels() {
	// 座標軸のラベルを追加（原点と各軸の正方向）
	AddText3D("Origin", {0, 0, 0}, {1.0f, 1.0f, 0.0f, 1.0f}, -1.0f, 1.0f, "", false, false);
	AddText3D("X+", {5, 0, 0}, {1.0f, 0.0f, 0.0f, 1.0f}, -1.0f, 1.0f, "", false, false);
	AddText3D("Y+", {0, 5, 0}, {0.0f, 1.0f, 0.0f, 1.0f}, -1.0f, 1.0f, "", false, false);
	AddText3D("Z+", {0, 0, 5}, {0.0f, 0.0f, 1.0f, 1.0f}, -1.0f, 1.0f, "", false, false);
}

void DebugTextManager::AddGridLabels(float gridSize, int gridCount) {
	// グリッドの座標ラベルを追加
	for (int x = -gridCount; x <= gridCount; x++) {
		for (int z = -gridCount; z <= gridCount; z++) {
			if (x == 0 && z == 0)
				continue; // 原点は飛ばす

			float xPos = x * gridSize;
			float zPos = z * gridSize;

			char label[32];
			sprintf_s(label, "(%d,%d)", x, z);
			AddText3D(label, {xPos, 0.1f, zPos}, {0.7f, 0.7f, 0.7f, 1.0f}, -1.0f, 0.8f, "", false, true);
		}
	}
}

void DebugTextManager::AddPointLabel(const std::string &label, const Vector3 &position, const Vector4 &color) {
	// 指定された位置にラベルを追加
	AddText3D(label, position, color, -1.0f, 1.0f, "", false, true);
}
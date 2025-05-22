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
	debugTexts_.clear();
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
	ClearAllTexts(); // 前フレームのテキストをクリア

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
			// オブジェクトタイプにより分岐（実装時はObject3dクラスなど実際のクラスで特殊化）
			if (auto obj3d = static_cast<Object3d *>(text.targetObject)) {
				text.worldPosition = GetObjectPosition(obj3d);
			}
		}

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
			screenPos = text.screenPosition;
		} else {
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
	if (!camera_)
		return Vector2{0, 0};

	// ワールド→ビュー→プロジェクション変換
	Matrix4x4 viewProjMatrix = camera_->GetViewProjectionMatrix();
	Vector3 ndcPos = Multiply(worldPosition, viewProjMatrix);

	// NDC座標からスクリーン座標への変換
	float width = static_cast<float>(winApp_->GetWindowWidth());
	float height = static_cast<float>(winApp_->GetWindowHeight());
	Vector2 screenPos;
	screenPos.x = (ndcPos.x + 1.0f) * width * 0.5f;
	screenPos.y = (1.0f - ndcPos.y) * height * 0.5f; // Y軸は反転

	return screenPos;
}

void DebugTextManager::AddText3D(const std::string &text, const Vector3 &position,
								 const Vector4 &color, float duration, float scale, const std::string &fontName) {
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

	debugTexts_.push_back(newText);
}

void DebugTextManager::AddTextScreen(const std::string &text, const Vector2 &position,
									 const Vector4 &color, float duration, float scale, const std::string &fontName) {
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

	debugTexts_.push_back(newText);
}

void DebugTextManager::ClearAllTexts() {
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
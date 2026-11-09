/*********************************************************************
 * \file EditorPanels.cpp
 * \brief Object3dから始める共通の制作操作
 *********************************************************************/
#include "EditorPanels.h"
#include "EditorUiSystem.h"
#ifdef _DEBUG
#include "imgui.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <cstring>
#include <array>
#endif

namespace MagEngine {
	EditorPanels::EditorPanels(EditorDocument &d, EditorDocument &s, EditorHistory &h, EditorPlaySession &p, EditorTypeRegistry &r, std::filesystem::path root)
		: document_(d), saved_(s), history_(h), session_(p), registry_(r), root_(std::move(root)) {}
	EditorDocument &EditorPanels::ActiveDocument() { return session_.IsRunning() ? session_.Runtime() : document_; }
#ifdef _DEBUG
	namespace {
		bool Text(const char *label, std::string &value) {
			std::vector<char> buffer((std::max)(value.size() + 256, size_t{1024}), '\0');
			std::copy(value.begin(), value.end(), buffer.begin());
			if (!ImGui::InputText(label, buffer.data(), buffer.size())) { return false; }
			value = buffer.data(); return true;
		}
	}
	///=============================================================================
	/// パネル登録と編集履歴
	void EditorPanels::Register(EditorUiSystem &system) {
		system.RegisterPanel("制作操作", EditorUiCategory::Scene, true, [this] { DrawToolbar(); });
		system.RegisterPanel("オブジェクト", EditorUiCategory::Scene, true, [this] { DrawHierarchy(); });
		system.RegisterPanel("設定", EditorUiCategory::Scene, true, [this] { DrawInspector(); });
		system.RegisterPanel("シーン", EditorUiCategory::Scene, true, [this] { DrawViewport(); });
		system.RegisterPanel("資産・メッセージ", EditorUiCategory::Scene, true, [this] { DrawAssets(); });
	}
	void EditorPanels::Unregister(EditorUiSystem &system) {
		for (const char *name : {"制作操作", "オブジェクト", "設定", "シーン", "資産・メッセージ"}) { system.UnregisterPanel(name); }
	}
	void EditorPanels::Commit(EditorDocument candidate) {
		for (const auto &record : candidate.objects) { const auto *type = registry_.Find(record.typeId); if (!type || (type->validate && !type->validate(record, error))) { return; } }
		if (!session_.IsRunning()) { history_.Execute(document_, candidate); return; }
		for (const auto &o : candidate.objects) {
			const auto *old = FindObject(session_.Runtime(), o.id); if (!old) { continue; }
			for (const char *key : {"name", "model", "visible", "position", "rotation", "scale", "settings"}) {
				const auto value = ReadEditorProperty(o, key);
				if (ReadEditorProperty(*old, key) != value) { session_.RecordUserEdit(o.id, key, value, error); }
			}
		}
	}
	void EditorPanels::FlushEdit(bool cancel) {
		if (!editing_) { return; }
		editing_ = false; axis_ = -1;
		const auto *original = FindObject(editStart_, editKey_.objectId);
		if (!original) { return; }
		CommitEditorProperty(ActiveDocument(), history_, session_.IsRunning() ? &session_ : nullptr, editKey_, ReadEditorProperty(*original, editKey_.property), editValue_, cancel, error);
	}
	void EditorPanels::TrackEdit(bool changed, const EditorDocument &before, std::string_view property) {
		if ((ImGui::IsItemActivated() || changed) && !editing_) { editStart_ = before; editKey_ = {selected, std::string(property)}; editing_ = true; editValue_ = ReadEditorProperty(*FindObject(before, selected), property); }
		if (changed) { if (const auto *object = FindObject(ActiveDocument(), selected)) { editValue_ = ReadEditorProperty(*object, property); } }
		if (editing_ && ImGui::IsItemDeactivated()) { FlushEdit(); }
	}
	///=============================================================================
	/// 操作・一覧・設定
	void EditorPanels::DrawToolbar() {
		ImGui::TextUnformatted(session_.IsRunning() ? "再生中の調整" : "シーン編集");
		ImGui::SameLine(); ImGui::TextUnformatted(EqualDocument(document_, saved_) ? "保存済み" : "● 未保存");
		if (ImGui::Button("保存")) { FlushEdit(); if (actions.save) { actions.save(); } }
		ImGui::BeginDisabled(session_.IsRunning());
		ImGui::SameLine(); if (ImGui::Button("読込")) { FlushEdit(); if (actions.load) { actions.load(); } }
		ImGui::SameLine(); if (ImGui::Button("Blender読込")) { FlushEdit(); if (actions.importBlender) { actions.importBlender(); } }
		ImGui::EndDisabled();
		Text("シーン名", sceneName);
		if (ImGui::Button("元に戻す")) { FlushEdit(); if (session_.IsRunning()) { session_.UndoUserEdit(error); } else { history_.Undo(document_); } }
		ImGui::SameLine(); if (ImGui::Button("やり直す")) { FlushEdit(); if (session_.IsRunning()) { session_.RedoUserEdit(error); } else { history_.Redo(document_); } }
		if (!session_.IsRunning()) {
			if (ImGui::Button("▶ 再生")) { FlushEdit(); if (actions.play) { actions.play(); } }
		} else {
			if (ImGui::Button(session_.IsPaused() ? "続ける" : "一時停止")) { FlushEdit(); session_.Pause(!session_.IsPaused()); }
			ImGui::SameLine(); if (ImGui::Button("■ 停止")) { FlushEdit(); if (actions.stop) { actions.stop(); } }
		}
		if (!session_.IsRunning() && !session_.Pending().empty()) {
			ImGui::SeparatorText("再生中の変更を残す");
			for (const auto &[key, change] : session_.Pending()) {
				const auto *object = FindObject(document_, key.objectId);
				const std::string label = (object ? object->name : "削除済み") + " / " + key.property + "##" + std::to_string(key.objectId);
				auto found = std::find_if(selectedChanges_.begin(), selectedChanges_.end(), [&key](const auto &v) { return v.objectId == key.objectId && v.property == key.property; });
				bool checked = found != selectedChanges_.end();
				if (ImGui::Checkbox(label.c_str(), &checked)) { if (checked) { selectedChanges_.push_back(key); } else { selectedChanges_.erase(found); } }
				ImGui::TextWrapped("%s → %s", change.before.dump().c_str(), change.after.dump().c_str());
			}
			if (ImGui::Button("選択した設定を反映")) { session_.ApplySelected(document_, selectedChanges_, history_, error); selectedChanges_.clear(); }
			ImGui::SameLine(); if (ImGui::Button("残りを破棄")) { session_.DiscardPending(); selectedChanges_.clear(); }
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { FlushEdit(true); }
	}
	void EditorPanels::DrawHierarchy() {
		Text("検索", search_);
		ImGui::BeginDisabled(session_.IsRunning() || editing_);
		if (ImGui::Button("追加")) { auto candidate = document_; selected = AddObject(candidate, {}); Commit(std::move(candidate)); }
		ImGui::SameLine(); if (ImGui::Button("複製") && FindObject(document_, selected)) { auto candidate = document_; auto copy = *FindObject(candidate, selected); copy.name += " Copy"; selected = AddObject(candidate, copy); Commit(std::move(candidate)); }
		ImGui::SameLine(); if (ImGui::Button("削除")) { auto candidate = document_; std::erase_if(candidate.objects, [this](const auto &o) { return o.id == selected; }); Commit(std::move(candidate)); selected = 0; }
		ImGui::EndDisabled();
		for (const auto &o : ActiveDocument().objects) {
			if (!search_.empty() && o.name.find(search_) == std::string::npos) { continue; }
			const auto label = o.name + "##object_" + std::to_string(o.id);
			if (ImGui::Selectable(label.c_str(), selected == o.id)) { const auto id = o.id; FlushEdit(); selected = id; break; }
		}
	}
	void EditorPanels::DrawInspector() {
		auto &document = ActiveDocument(); auto *object = FindObject(document, selected);
		if (!object) { selected = 0; ImGui::TextUnformatted("対象を一覧またはシーンで選択してください"); return; }
		ImGui::Text("ID: %llu", static_cast<unsigned long long>(selected));
		auto before = document; bool changed = Text("名前", object->name); TrackEdit(changed, before, "name");
		object = FindObject(document, selected); before = document; changed = ImGui::Checkbox("表示", &object->visible); TrackEdit(changed, before, "visible");
		object = FindObject(document, selected); before = document;
		changed = ImGui::DragFloat3("位置", &object->transform.translate.x, 0.1f); TrackEdit(changed, before, "position");
		object = FindObject(document, selected); before = document;
		constexpr float degrees = 180.0f / std::numbers::pi_v<float>;
		float rotation[]{object->transform.rotate.x * degrees, object->transform.rotate.y * degrees, object->transform.rotate.z * degrees};
		changed = ImGui::DragFloat3("回転（度）", rotation, 0.5f);
		if (changed) { object->transform.rotate = {rotation[0] / degrees, rotation[1] / degrees, rotation[2] / degrees}; } TrackEdit(changed, before, "rotation");
		object = FindObject(document, selected); before = document;
		changed = ImGui::DragFloat3("大きさ（倍率）", &object->transform.scale.x, 0.01f, 0.001f, 10000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp); TrackEdit(changed, before, "scale");
		if (ImGui::Button("Transformを初期値に戻す")) { FlushEdit(); auto candidate = document; FindObject(candidate, selected)->transform = {{1, 1, 1}, {}, {}}; Commit(std::move(candidate)); }
		object = FindObject(document, selected);
		ImGui::SeparatorText("描画設定"); ImGui::TextWrapped("モデル: %s", object->modelPath.empty() ? "未設定（空オブジェクト）" : object->modelPath.c_str());
		ImGui::TextWrapped("下の資産一覧でモデルを選択すると、この対象へ設定します。");
		if (const auto *type = registry_.Find(object->typeId); type && type->drawSettings) { auto candidate = document; if (type->drawSettings(*FindObject(candidate, selected))) { Commit(std::move(candidate)); } }
	}
	void EditorPanels::DrawAssets() {
		if (assets_.empty() || ImGui::Button("資産一覧を更新")) {
			assets_.clear(); std::error_code ec;
			for (std::filesystem::recursive_directory_iterator it(root_ / "resources/model", ec), end; !ec && it != end; it.increment(ec)) {
				if (it->is_regular_file(ec)) { const auto extension = it->path().extension().string(); if (extension == ".obj" || extension == ".gltf" || extension == ".glb") { const auto value = it->path().lexically_relative(root_ / "resources").generic_u8string(); assets_.emplace_back(value.begin(), value.end()); } }
			}
			std::sort(assets_.begin(), assets_.end());
		}
		if (!error.empty()) { ImGui::TextWrapped("%s", error.c_str()); }
		static char filter[256]{}; ImGui::InputText("モデル検索", filter, sizeof(filter));
		for (const auto &asset : assets_) {
			if (asset.find(filter) == std::string::npos) { continue; }
			if (ImGui::Selectable(asset.c_str()) && selected) { FlushEdit(); auto candidate = ActiveDocument(); if (auto *object = FindObject(candidate, selected)) { object->modelPath = asset; Commit(std::move(candidate)); } }
		}
	}
	///=============================================================================
	/// シーン画像と選択・軸操作
	void EditorPanels::DrawViewport() {
		ImGui::RadioButton("移動", &mode_, 0); ImGui::SameLine(); ImGui::RadioButton("回転", &mode_, 1); ImGui::SameLine(); ImGui::RadioButton("拡縮", &mode_, 2);
		ImGui::TextUnformatted("右ドラッグ: 視点 / WASD: 移動 / QE: 上下 / 色付きの軸: 配置操作");
		const auto position = ImGui::GetCursorScreenPos(), available = ImGui::GetContentRegionAvail();
		const auto rect = FitViewport({position.x, position.y, available.x, available.y}, 16.0f / 9.0f);
		imageRect_ = rect;
		if (rect.width <= 0 || rect.height <= 0) { viewportFocused = false; return; }
		ImGui::SetCursorScreenPos({rect.x, rect.y});
		const auto texture = actions.sceneTexture ? actions.sceneTexture() : 0;
		if (!texture) { ImGui::TextUnformatted("描画画面を準備しています"); return; }
		ImGui::Image(static_cast<ImTextureID>(texture), {rect.width, rect.height});
		const bool hovered = ImGui::IsItemHovered(); viewportFocused = hovered && ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput;
		auto &document = ActiveDocument();
		auto project = [&document, rect](MagMath::Vector3 point) -> std::optional<ImVec2> {
			using namespace DirectX; const auto &c = document.camera;
			const auto world = XMMatrixRotationX(c.rotate.x) * XMMatrixRotationY(c.rotate.y) * XMMatrixRotationZ(c.rotate.z) * XMMatrixTranslation(c.translate.x, c.translate.y, c.translate.z);
			XMFLOAT3 view; XMStoreFloat3(&view, XMVector3TransformCoord(XMVectorSet(point.x, point.y, point.z, 1), XMMatrixInverse(nullptr, world)));
			if (view.z <= 0.1f) { return {}; }
			const float factor = rect.height / (2 * std::tan(0.45f * 0.5f)); return ImVec2(rect.x + rect.width * 0.5f + view.x / view.z * factor, rect.y + rect.height * 0.5f - view.y / view.z * factor);
		};
		auto *object = FindObject(document, selected); auto *draw = ImGui::GetWindowDrawList(); const auto mouse = ImGui::GetIO().MousePos;
		bool hitAxis = false;
		draw->PushClipRect({rect.x, rect.y}, {rect.x + rect.width, rect.y + rect.height}, true);
		if (object) {
			const auto bounds = actions.bounds ? actions.bounds(*object) : EditorBounds{};
			std::array<std::optional<ImVec2>, 8> corners;
			const auto &t = object->transform;
			using namespace DirectX;
			const auto world = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z) * XMMatrixRotationX(t.rotate.x) * XMMatrixRotationY(t.rotate.y) * XMMatrixRotationZ(t.rotate.z) * XMMatrixTranslation(t.translate.x, t.translate.y, t.translate.z);
			for (int i = 0; i < 8; ++i) {
				XMFLOAT3 point; XMStoreFloat3(&point, XMVector3TransformCoord(XMVectorSet(i & 1 ? bounds.max.x : bounds.min.x, i & 2 ? bounds.max.y : bounds.min.y, i & 4 ? bounds.max.z : bounds.min.z, 1), world));
				corners[i] = project({point.x, point.y, point.z});
			}
			for (int i = 0; i < 8; ++i) { for (int bit : {1, 2, 4}) { const int j = i ^ bit; if (i < j && corners[i] && corners[j]) { draw->AddLine(*corners[i], *corners[j], IM_COL32(255, 215, 100, 255), 1.5f); } } }
			const auto center = project(object->transform.translate);
			if (center) {
				draw->AddCircle(*center, 9, IM_COL32(255, 215, 100, 255), 16, 2);
				for (int axis = 0; axis < 3; ++axis) {
					auto endpoint = object->transform.translate; if (axis == 0) { endpoint.x += 2; } else if (axis == 1) { endpoint.y += 2; } else { endpoint.z += 2; }
					const auto end = project(endpoint); if (!end) { continue; }
					const ImU32 color = axis == 0 ? IM_COL32(240, 85, 85, 255) : axis == 1 ? IM_COL32(90, 220, 110, 255) : IM_COL32(90, 155, 255, 255);
					draw->AddLine(*center, *end, color, 3); draw->AddCircleFilled(*end, 6, color);
					const float dx = mouse.x - end->x, dy = mouse.y - end->y;
					if (hovered && dx * dx + dy * dy < 144 && ImGui::IsMouseClicked(0)) {
						FlushEdit(); object = FindObject(document, selected); editStart_ = document; editing_ = true; axis_ = axis; dragStart_ = mouse.x - mouse.y;
						editKey_ = {selected, mode_ == 0 ? "position" : mode_ == 1 ? "rotation" : "scale"}; editValue_ = ReadEditorProperty(*object, editKey_.property);
						dragValue_ = mode_ == 0 ? object->transform.translate : mode_ == 1 ? object->transform.rotate : object->transform.scale; hitAxis = true;
					}
				}
			}
		}
		draw->PopClipRect();
		if (editing_ && axis_ >= 0 && object) {
			const float delta = ((mouse.x - mouse.y) - dragStart_) * (mode_ == 0 ? 0.02f : 0.01f);
			auto value = dragValue_; float *component = axis_ == 0 ? &value.x : axis_ == 1 ? &value.y : &value.z;
			*component = mode_ == 2 ? (std::max)(0.001f, *component + delta) : *component + delta;
			if (mode_ == 0) { object->transform.translate = value; } else if (mode_ == 1) { object->transform.rotate = value; } else { object->transform.scale = value; }
			editValue_ = ReadEditorProperty(*object, editKey_.property);
			if (!ImGui::IsMouseDown(0)) { FlushEdit(); axis_ = -1; }
		} else if (hovered && !hitAxis && ImGui::IsMouseClicked(0)) {
			FlushEdit(); const auto ray = ScreenToViewportRay({mouse.x, mouse.y}, rect, document.camera, 0.45f); float nearest = 1e30f; selected = 0;
			if (ray) { for (const auto &o : document.objects) { if (!o.visible) { continue; } const auto bounds = actions.bounds ? actions.bounds(o) : EditorBounds{}; const auto hit = IntersectEditorBounds(*ray, bounds, o.transform); if (hit && *hit < nearest) { nearest = *hit; selected = o.id; } } }
		}
		if (viewportFocused && ImGui::IsMouseDown(1) && !editing_) {
			auto &camera = document.camera; const auto &io = ImGui::GetIO();
			camera.rotate.y += io.MouseDelta.x * 0.005f; camera.rotate.x += io.MouseDelta.y * 0.005f;
			const float speed = io.DeltaTime * 10; const float forward = (ImGui::IsKeyDown(ImGuiKey_W) ? speed : 0) - (ImGui::IsKeyDown(ImGuiKey_S) ? speed : 0); const float side = (ImGui::IsKeyDown(ImGuiKey_D) ? speed : 0) - (ImGui::IsKeyDown(ImGuiKey_A) ? speed : 0);
			camera.translate.x += std::sin(camera.rotate.y) * forward + std::cos(camera.rotate.y) * side; camera.translate.z += std::cos(camera.rotate.y) * forward - std::sin(camera.rotate.y) * side;
			camera.translate.y += (ImGui::IsKeyDown(ImGuiKey_E) ? speed : 0) - (ImGui::IsKeyDown(ImGuiKey_Q) ? speed : 0);
		}
		if (!ImGui::IsWindowFocused() && editing_ && axis_ >= 0) { FlushEdit(); axis_ = -1; }
	}
#else
	void EditorPanels::Register(EditorUiSystem &) {}
	void EditorPanels::Unregister(EditorUiSystem &) {}
	void EditorPanels::FlushEdit(bool) {}
#endif
}

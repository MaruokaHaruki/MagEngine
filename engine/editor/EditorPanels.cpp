/*********************************************************************
 * \file EditorPanels.cpp
 * \brief Object3dから始める共通の制作操作
 *********************************************************************/
#include "EditorPanels.h"
#include "EditorUiSystem.h"
#include "EditorTheme.h"
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
			ImGui::TextUnformatted(label); ImGui::SetNextItemWidth(-1); ImGui::PushID(label);
			const bool changed = ImGui::InputText("##text", buffer.data(), buffer.size()); ImGui::PopID();
			if (!changed) { return false; }
			value = buffer.data(); return true;
		}
	}
	///=============================================================================
	/// パネル登録と編集履歴
	void EditorPanels::Register(EditorUiSystem &system) {
        system_ = &system;
        system.SetToolbar([this] { DrawToolbar(); });
        system.SetFileMenu([this] { DrawFileMenu(); });
        system.SetBeforeLayoutChange([this] { FinishEditing(); });
        system.SetAdditionalMessages([this] { if (!error.empty()) { ImGui::TextWrapped("%s", error.c_str()); } });
        system.GetViewport().SetOverlay([this](const ViewportRect &rect) { DrawViewportOverlay(rect); });
        system.RegisterPanel("authoring.objects", "オブジェクト", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Left, true, [this] { DrawHierarchy(); });
        system.RegisterPanel("authoring.inspector", "設定", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Right, true, [this] { DrawInspector(); });
        system.RegisterPanel("authoring.assets", "資産", EditorUiCategory::Authoring, EditorPanelScope::Engine, EditorDockRegion::Bottom, true, [this] { DrawAssets(); });
        system.RegisterPanel("authoring.playchanges", "再生変更の反映", EditorUiCategory::Tools, EditorPanelScope::Engine, EditorDockRegion::Bottom, false, [this] { DrawPlayChanges(); });
    }
    void EditorPanels::Unregister(EditorUiSystem &system) {
        for (const char *id : {"authoring.objects", "authoring.inspector", "authoring.assets", "authoring.playchanges"}) { system.UnregisterPanel(id); }
        system.SetToolbar({}); system.SetFileMenu({}); system.SetBeforeLayoutChange({}); system.SetAdditionalMessages({}); system.GetViewport().SetOverlay({}); system_ = nullptr;
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
        const bool narrow = EditorToolbarRows(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() / 16.0f) > 1;
        if (ImGui::BeginTable("authoring.toolbar", narrow ? 1 : 3, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("files", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            if (!narrow) {
                ImGui::TableSetupColumn("play", ImGuiTableColumnFlags_WidthStretch, 0.3f);
                ImGui::TableSetupColumn("state", ImGuiTableColumnFlags_WidthStretch, 0.3f);
            }
            ImGui::TableNextColumn();
            if (ImGui::Button("保存")) { FlushEdit(); if (actions.save) { actions.save(); } }
            ImGui::SameLine(); if (ImGui::Button("戻す")) { FlushEdit(); if (session_.IsRunning()) { session_.UndoUserEdit(error); } else { history_.Undo(document_); } }
            ImGui::SameLine(); if (ImGui::Button("やり直す")) { FlushEdit(); if (session_.IsRunning()) { session_.RedoUserEdit(error); } else { history_.Redo(document_); } }
            ImGui::TableNextColumn();
            if (!session_.IsRunning()) { if (ImGui::Button("再生")) { FlushEdit(); if (actions.play) { actions.play(); } } }
            else {
                if (ImGui::Button(session_.IsPaused() ? "続ける" : "一時停止")) { FlushEdit(); session_.Pause(!session_.IsPaused()); }
                ImGui::SameLine(); if (ImGui::Button("停止")) { FlushEdit(); if (actions.stop) { actions.stop(); } }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(session_.IsRunning() ? "再生中の調整" : "シーン編集");
            ImGui::SameLine(); ImGui::TextDisabled("%s", EqualDocument(document_, saved_) ? "保存済み" : "未保存");
            ImGui::EndTable();
        }
        ImGui::RadioButton("移動", &mode_, 0); ImGui::SameLine(); ImGui::RadioButton("回転", &mode_, 1); ImGui::SameLine(); ImGui::RadioButton("拡縮", &mode_, 2);
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { FlushEdit(true); }
        if (system_) {
            const bool pending = !session_.IsRunning() && !session_.Pending().empty();
            system_->SetPanelOpen("authoring.playchanges", pending);
            system_->SetPanelTitle("authoring.playchanges", "再生変更の反映 (" + std::to_string(session_.Pending().size()) + ")");
        }
    }
    void EditorPanels::DrawFileMenu() {
        Text("保存名", sceneName);
        if (ImGui::MenuItem("保存")) { FlushEdit(); if (actions.save) { actions.save(); } }
        ImGui::BeginDisabled(session_.IsRunning());
        if (ImGui::MenuItem("読込")) { FlushEdit(); if (actions.load) { actions.load(); } }
        if (ImGui::MenuItem("Blender読込")) { FlushEdit(); if (actions.importBlender) { actions.importBlender(); } }
        ImGui::EndDisabled();
    }
    void EditorPanels::DrawPlayChanges() {
        if (session_.IsRunning() || session_.Pending().empty()) { ImGui::TextUnformatted("反映する変更はありません"); return; }
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
		if (!object) { selected = 0; ImGui::TextWrapped("対象を一覧またはシーンで選択してください"); return; }
		if (ImGui::CollapsingHeader("詳細情報")) { ImGui::Text("ID: %llu", static_cast<unsigned long long>(selected)); }
		ImGui::SeparatorText("名前と表示");
		auto before = document; bool changed = Text("名前", object->name); TrackEdit(changed, before, "name");
		object = FindObject(document, selected); before = document; changed = ImGui::Checkbox("表示", &object->visible); TrackEdit(changed, before, "visible");
        ImGui::SeparatorText("位置・回転・大きさ");
        auto vectorField = [&](const char *label, const char *property, float speed, bool degrees) {
            ImGui::PushID(property); ImGui::TextUnformatted(label);
            if (ImGui::BeginTable("axes", ImGui::GetContentRegionAvail().x < 360 * ImGui::GetFontSize() / 16.0f ? 1 : 3, ImGuiTableFlags_SizingStretchSame)) {
                const ImVec4 colors[]{{0.94f,0.38f,0.38f,1},{0.4f,0.85f,0.5f,1},{0.4f,0.65f,1,1}};
                for (int axis = 0; axis < 3; ++axis) {
                    ImGui::TableNextColumn(); object = FindObject(document, selected); before = document;
                    auto value = ReadEditorProperty(*object, property); float component = value[axis].get<float>();
                    constexpr float factor = 180.0f / std::numbers::pi_v<float>; if (degrees) { component *= factor; }
                    ImGui::TextColored(colors[axis], "%s", axis == 0 ? "X" : axis == 1 ? "Y" : "Z"); ImGui::SameLine(); ImGui::SetNextItemWidth(-1);
                    ImGui::PushID(axis); const bool scale = std::string_view(property) == "scale";
                    const bool edited = ImGui::DragFloat("##value", &component, speed, scale ? 0.001f : 0, scale ? 10000.0f : 0, "%.3f", scale ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None);
                    if (edited) { value[axis] = degrees ? component / factor : component; WriteEditorProperty(*object, property, value, error); }
                    TrackEdit(edited, before, property); ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::PopID();
        };
        vectorField("位置", "position", 0.1f, false); vectorField("回転（度）", "rotation", 0.5f, true); vectorField("大きさ（倍率）", "scale", 0.01f, false);
		if (ImGui::Button("Transformを初期化")) { FlushEdit(); auto candidate = document; FindObject(candidate, selected)->transform = {{1, 1, 1}, {}, {}}; Commit(std::move(candidate)); }
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
		static char filter[256]{}; ImGui::TextUnformatted("モデル検索"); ImGui::SetNextItemWidth(-1); ImGui::InputText("##modelSearch", filter, sizeof(filter));
		for (const auto &asset : assets_) {
			if (asset.find(filter) == std::string::npos) { continue; }
			if (ImGui::Selectable(asset.c_str()) && selected) { FlushEdit(); auto candidate = ActiveDocument(); if (auto *object = FindObject(candidate, selected)) { object->modelPath = asset; Commit(std::move(candidate)); } }
		}
	}
	///=============================================================================
	/// シーン画像と選択・軸操作
	void EditorPanels::DrawViewportOverlay(const ViewportRect &rect) {
        imageRect_ = rect;
        const auto mousePosition = ImGui::GetIO().MousePos;
        const bool hovered = mousePosition.x >= rect.x && mousePosition.y >= rect.y && mousePosition.x < rect.x + rect.width && mousePosition.y < rect.y + rect.height && ImGui::IsWindowHovered();
        viewportFocused = hovered && ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput;

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

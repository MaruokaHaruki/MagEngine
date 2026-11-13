/*********************************************************************
 * \file EditorHost.cpp
 * \brief フレーム境界で編集値を実体へ適用し、既存ゲームと分離する
 *********************************************************************/
#include "EditorHost.h"
#include "DirectXCore.h"
#include "ModelManager.h"
#include "EditorUiSystem.h"
#include "LineManager.h"
#include "Input.h"
#include <commdlg.h>
#include <limits>
#include <algorithm>
#include <stdexcept>
#pragma comment(lib, "comdlg32.lib")

namespace MagEngine {
	namespace {
		///=============================================================================
		/// Object3dを制作型の契約へ接続する。描画方式自体は変更しない
		class Object3dEditorInstance final : public EditorRenderable {
		public:
			Object3dEditorInstance(const EngineContext &context, const EditorObjectRecord &record) {
				if (record.modelPath.empty()) { return; }
				const auto path = std::filesystem::path(std::u8string(record.modelPath.begin(), record.modelPath.end()));
				if (*path.begin() != "model") { throw std::runtime_error(record.name + ": モデルはresources/model内に置いてください"); }
				const auto encoded = path.lexically_relative("model").generic_u8string(); const std::string name(encoded.begin(), encoded.end());
				context.modelManager->LoadModel(name); const auto *model = context.modelManager->FindModel(name);
				if (!model) { throw std::runtime_error("モデルの登録に失敗しました"); }
				object_ = std::make_unique<Object3d>(); object_->Initialize(context.object3dSetup); object_->SetModel(name);
				bounds_ = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
				for (const auto &vertex : model->GetModelData().vertices) {
					bounds_.min.x = (std::min)(bounds_.min.x, vertex.position.x); bounds_.min.y = (std::min)(bounds_.min.y, vertex.position.y); bounds_.min.z = (std::min)(bounds_.min.z, vertex.position.z);
					bounds_.max.x = (std::max)(bounds_.max.x, vertex.position.x); bounds_.max.y = (std::max)(bounds_.max.y, vertex.position.y); bounds_.max.z = (std::max)(bounds_.max.z, vertex.position.z);
				}
			}
			void Update(const EditorObjectRecord &record, Camera &camera, const FrameTime &) override { if (object_) { object_->SetTransform(record.transform); object_->SetCamera(&camera); object_->Update(); } }
			void RegisterRenderables(RenderWorld &world) override { if (object_) { object_->RegisterRenderables(world); } }
			EditorBounds GetBounds() const override { return bounds_; }
		private:
			std::unique_ptr<Object3d> object_;
			EditorBounds bounds_;
		};
	}
	///=============================================================================
	/// 初期化と型の接続
	void EditorHost::Initialize(const EngineContext &context, const EditorLaunchOptions &options) {
		context_ = &context; options_ = options;
		store_ = std::make_unique<EditorDocumentStore>(options.projectRoot, &registry_);
		panels_ = std::make_unique<EditorPanels>(document_, saved_, history_, session_, registry_, options.projectRoot);
		EditorTypeDescriptor type; type.typeId = "object3d"; type.getBounds = [this](const auto &record) { return Bounds(record); };
		type.validate = [](const auto &record, std::string &error) {
			if (!record.settings.empty()) { error = record.name + ": 初期版Object3dの設定はTransform・モデル・表示に限定します"; return false; } return true;
		};
		type.create = [this](const auto &record) { return std::make_unique<Object3dEditorInstance>(*context_, record); };
		registry_.Register(std::move(type));
		if (!options.scene.empty()) {
			if (!store_->Load(options.projectRoot / options.scene, document_, panels_->error)) { throw std::runtime_error(panels_->error); }
			scenePath_ = options.projectRoot / options.scene;
			const auto name = options.scene.filename().u8string(); panels_->sceneName.assign(name.begin(), name.end());
		}
		saved_ = document_; camera_.SetFovY(0.45f); camera_.SetAspectRatio(16.0f / 9.0f);
		panels_->actions.save = [this] { request_ = Request::Save; };
		panels_->actions.load = [this] { request_ = Request::Load; };
		panels_->actions.importBlender = [this] { request_ = Request::Import; };
		panels_->actions.play = [this] { request_ = Request::Play; };
		panels_->actions.stop = [this] { request_ = Request::Stop; };
		panels_->actions.bounds = [this](const auto &record) { return Bounds(record); };
#ifdef _DEBUG
		if (options.editor) {
			panels_->Register(*context.editorUiSystem);
		}
#endif
		if (!ApplyDocument(document_, panels_->error)) { throw std::runtime_error(panels_->error); }
	}
	EditorBounds EditorHost::Bounds(const EditorObjectRecord &record) const {
		const auto found = bounds_.find(record.id); return found == bounds_.end() ? EditorBounds{} : found->second;
	}
	bool EditorHost::ApplyDocument(const EditorDocument &document, std::string &error) {
		if (!store_->ValidateAssets(document, error)) { return false; }
		try {
			bool sameModels = hasApplied_ && document.objects.size() == applied_.objects.size();
			for (const auto &record : document.objects) {
				const auto *previous = FindObject(applied_, record.id);
				if (!previous || previous->modelPath != record.modelPath || previous->typeId != record.typeId) { sameModels = false; }
				const auto *type = registry_.Find(record.typeId);
				if (!type || (type->validate && !type->validate(record, error))) { return false; }
			}
			if (sameModels) {
				applied_ = document; error.clear(); return true;
			}
			// GPUが参照中のObject3d定数バッファを交換・破棄しない。
			context_->graphics->WaitForGpuIdle();
			std::map<EditorObjectId, std::unique_ptr<EditorRenderable>> candidate;
			std::map<EditorObjectId, EditorBounds> bounds;
			for (const auto &record : document.objects) {
				const auto *type = registry_.Find(record.typeId);
				if (!type || (type->validate && !type->validate(record, error))) { return false; }
				if (!type->create) { error = record.typeId + ": 実体の生成処理が未登録です"; return false; }
				auto object = type->create(record); if (!object) { error = record.name + ": 実体を生成できません"; return false; }
				bounds[record.id] = object->GetBounds(); candidate.emplace(record.id, std::move(object));
			}
			objects_ = std::move(candidate); bounds_ = std::move(bounds); applied_ = document; hasApplied_ = true; error.clear(); return true;
		} catch (const std::exception &e) { error = e.what(); return false; }
	}
	///=============================================================================
	/// 保存と状態切替はフレーム境界で行う
	void EditorHost::Save() {
		for (const auto &record : document_.objects) { const auto *type = registry_.Find(record.typeId); if (!type || (type->validate && !type->validate(record, panels_->error))) { return; } }
		const auto path = std::filesystem::path(std::u8string(panels_->sceneName.begin(), panels_->sceneName.end()));
		if (path.empty() || path.filename() != path || path.extension() != ".json") { panels_->error = "シーン名は拡張子.jsonのファイル名で指定してください"; return; }
		const auto scenesRoot = options_.projectRoot / "resources/config/scenes";
		const auto relative = scenePath_.lexically_normal().lexically_relative(scenesRoot.lexically_normal());
		const bool inScenes = !relative.empty() && *relative.begin() != "..";
		const auto destination = inScenes && scenePath_.filename() == path
			? scenePath_
			: scenesRoot / path;
		if (store_->Save(destination, document_, panels_->error)) {
			scenePath_ = destination;
			saved_ = document_;
		}
	}
	bool EditorHost::CanClose() {
		if (!panels_) { return true; }
		panels_->FinishEditing();
		if (!session_.Pending().empty()) { session_.Stop(); hasApplied_ = false; panels_->error = "制作操作の反映候補を選んで反映するか、残りを破棄してください"; return false; }
		if (EqualDocument(document_, saved_)) { return true; }
		const auto result = MessageBoxW(nullptr, L"シーンに未保存の変更があります。保存しますか？\nはい: 保存 / いいえ: 破棄 / キャンセル: 編集を続ける", L"MagEngine シーン編集", MB_YESNOCANCEL | MB_ICONQUESTION);
		if (result == IDCANCEL) { return false; }
		if (result == IDYES) { Save(); return EqualDocument(document_, saved_); }
		return true;
	}
	void EditorHost::ProcessRequest() {
		const auto request = request_; request_ = Request::None;
		if (request == Request::None) { return; }
		if (request == Request::Save) { Save(); return; }
		if (request == Request::Stop) { session_.Stop(); hasApplied_ = false; return; }
		if (request == Request::Play) {
			if (!session_.Start(document_, panels_->error)) { return; }
			hasApplied_ = false; // 編集時のGPU実体を再生セッションへ使い回さない
			if (!ApplyDocument(session_.Runtime(), panels_->error)) { session_.Stop(); }
			return;
		}
		if (!CanClose()) { return; }
		wchar_t filename[32768]{};
		OPENFILENAMEW dialog{}; dialog.lStructSize = sizeof(dialog); dialog.lpstrFile = filename; dialog.nMaxFile = static_cast<DWORD>(std::size(filename)); dialog.lpstrFilter = L"JSON\0*.json\0"; dialog.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (!GetOpenFileNameW(&dialog)) { return; }
		EditorDocument candidate;
		const bool loaded = request == Request::Import ? store_->ImportBlender(filename, candidate, panels_->error) : store_->Load(filename, candidate, panels_->error);
		if (!loaded || !ApplyDocument(candidate, panels_->error)) { return; }
		document_ = std::move(candidate); history_.Reset(); panels_->selected = 0;
		if (request == Request::Load) {
			scenePath_ = filename;
			saved_ = document_;
			const auto name = scenePath_.filename().u8string();
			panels_->sceneName.assign(name.begin(), name.end());
		} else {
			scenePath_.clear();
			saved_ = {};
			panels_->sceneName = "imported.json";
		}
	}
	void EditorHost::Update(const FrameTime &frameTime) {
		const auto renderSize = context_->graphics->GetRenderSize();
		camera_.SetAspectRatio(static_cast<float>(renderSize.width) / renderSize.height);
		ProcessRequest();
		auto &document = panels_->ActiveDocument();
		if (session_.IsRunning() && !session_.IsPaused() && runtimeUpdate_) {

			try { runtimeUpdate_(frameTime, document); } catch (...) {  throw; }

		}
		if (!hasApplied_ || !EqualDocument(applied_, document)) { ApplyDocument(document, panels_->error); }
		camera_.SetTransform(document.camera); camera_.Update();
		for (auto &[id, object] : objects_) { if (const auto *record = FindObject(applied_, id)) { object->Update(*record, camera_, frameTime); } }
	}
	void EditorHost::RegisterRenderables(RenderWorld &world) {
		const auto &document = hasApplied_ ? applied_ : document_;
		for (const auto &record : document.objects) { const auto found = objects_.find(record.id); if (record.visible && found != objects_.end()) { found->second->RegisterRenderables(world); } }
	}
	void EditorHost::Finalize() {
		if (!context_) { return; }
#ifdef _DEBUG
		if (panels_ && options_.editor) { panels_->Unregister(*context_->editorUiSystem); }
#endif
		context_->graphics->WaitForGpuIdle(true); objects_.clear(); panels_.reset(); store_.reset(); runtimeUpdate_ = {}; context_ = nullptr;
	}
}

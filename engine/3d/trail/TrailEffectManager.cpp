/*********************************************************************
 * \file   TrailEffectManager.cpp
 * \brief  トレイルエフェクトマネージャ実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#include "TrailEffectManager.h"
#include "Logger.h"
#include "TrailEffectSetup.h"
#include "externals/imgui/imgui.h"
#include "externals/json.hpp"
#include <fstream>
#include <sstream>

using namespace Logger;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void TrailEffectManager::Initialize(TrailEffectSetup *setup) {
		//========================================
		// 引数チェック
		if (!setup) {
			throw std::runtime_error("TrailEffectManager: TrailEffectSetup is null.");
		}

		//========================================
		// Setupを保持
		setup_ = setup;

		Log("TrailEffectManager initialized", LogLevel::Success);
	}

	///=============================================================================
	///						更新処理
	void TrailEffectManager::Update(float deltaTime) {
		//========================================
		// すべてのアクティブなエフェクトを更新
		for (auto &pair : activeEffects_) {
			if (pair.second && pair.second->IsEnabled()) {
				pair.second->Update(deltaTime);
			}
		}
	}

	///=============================================================================
	///						描画
	void TrailEffectManager::Draw() {
		//========================================
		// すべてのアクティブなエフェクトを描画
		for (auto &pair : activeEffects_) {
			if (pair.second && pair.second->IsEnabled()) {
				pair.second->Draw();
			}
		}
	}

	///=============================================================================
	///						ImGui描画
	void TrailEffectManager::DrawImGui() {
		ImGui::Begin("TrailEffectManager");

		//========================================
		// 統計情報
		ImGui::Text("Active Effects: %zu", GetEffectCount());
		ImGui::Text("Registered Presets: %zu", GetPresetCount());
		ImGui::Separator();

		//========================================
		// 新規作成セクション
		if (ImGui::CollapsingHeader("Create New Preset", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginChild("CreatePreset", ImVec2(0, 100), true);

			ImGui::Text("New Preset Name:");
			ImGui::InputText("##newPresetName", presetNameBuffer_, IM_ARRAYSIZE(presetNameBuffer_));

			if (ImGui::Button("Create Preset", ImVec2(150, 0))) {
				if (strlen(presetNameBuffer_) > 0) {
					TrailEffectPreset newPreset;
					newPreset.name = presetNameBuffer_;
					RegisterPreset(newPreset);
					// 作成後、メインファイルに保存
					SaveAllPresetsToJson("resources/trail/test_preset.json");
					Log("Preset created and saved: " + newPreset.name, LogLevel::Success);
					memset(presetNameBuffer_, 0, sizeof(presetNameBuffer_));
				}
			}

			ImGui::EndChild();
		}

		ImGui::Separator();

		//========================================
		// プリセット管理セクション
		if (ImGui::CollapsingHeader("Preset Management", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginChild("PresetManagement", ImVec2(0, 300), true);

			//========================================
			// プリセット一覧と操作
			std::vector<std::string> presetNamesList = GetPresetNames();
			for (const auto &presetName : presetNamesList) {
				bool isCurrent = (editingPresetName_ == presetName && editingPresetActive_);

				ImGui::PushID(presetName.c_str());
				if (isCurrent) {
					ImGui::SetNextItemOpen(true);
				}

				if (ImGui::TreeNode(presetName.c_str())) {
					if (ImGui::Button("Edit")) {
						editingPresetName_ = presetName;
						editingPresetCopy_ = *GetPreset(presetName);
						editingPresetActive_ = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("Duplicate")) {
						TrailEffectPreset *original = GetPreset(presetName);
						if (original) {
							TrailEffectPreset copy = *original;
							copy.name = presetName + "_copy";
							RegisterPreset(copy);
							// 複製後、メインファイルに保存
							SaveAllPresetsToJson("resources/trail/test_preset.json");
							Log("Preset duplicated and saved: " + copy.name, LogLevel::Success);
						}
					}
					ImGui::SameLine();
					std::string deleteLabel = "Delete##" + presetName;
					if (ImGui::Button(deleteLabel.c_str())) {
						UnregisterPreset(presetName);
						if (editingPresetName_ == presetName) {
							editingPresetActive_ = false;
							editingPresetName_ = "";
						}
						// 削除後、メインファイルに保存
						SaveAllPresetsToJson("resources/trail/test_preset.json");
						Log("Preset deleted and saved", LogLevel::Success);
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			ImGui::EndChild();
		}

		ImGui::Separator();

		//========================================
		// エディターセクション
		if (editingPresetActive_ && ImGui::CollapsingHeader("Preset Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginChild("PresetEditor", ImVec2(0, 500), true);

			ImGui::Text("Editing: %s", editingPresetName_.c_str());

			//========================================
			// プリセット名変更
			static char renamingBuffer[128] = {};
			if (editingPresetActive_) {
				if (ImGui::Button("Rename Preset")) {
					strncpy_s(renamingBuffer, editingPresetName_.c_str(), sizeof(renamingBuffer) - 1);
				}
			}
			if (strlen(renamingBuffer) > 0) {
				ImGui::SameLine();
				ImGui::SetNextItemWidth(200);
				ImGui::InputText("##renameBuffer", renamingBuffer, IM_ARRAYSIZE(renamingBuffer));
				ImGui::SameLine();
				if (ImGui::Button("Confirm##rename")) {
					std::string newName = renamingBuffer;
					if (newName != editingPresetName_ && presets_.find(newName) == presets_.end()) {
						TrailEffectPreset preset = editingPresetCopy_;
						preset.name = newName;
						UnregisterPreset(editingPresetName_);
						RegisterPreset(preset);
						editingPresetName_ = newName;
						editingPresetCopy_ = preset;
						// 名前変更後、メインファイルに保存
						SaveAllPresetsToJson("resources/trail/test_preset.json");
						Log("Preset renamed and saved: " + newName, LogLevel::Success);
						memset(renamingBuffer, 0, sizeof(renamingBuffer));
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel##rename")) {
					memset(renamingBuffer, 0, sizeof(renamingBuffer));
				}
			}

			ImGui::Separator();

			//========================================
			// ライフタイム設定
			if (ImGui::TreeNodeEx("Lifetime Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("Life Time##editor", &editingPresetCopy_.lifeTime, 0.1f, 10.0f);
				ImGui::SliderFloat("Min Point Distance##editor", &editingPresetCopy_.minPointDistance, 0.01f, 2.0f);
				ImGui::SliderInt("Max Points##editor", (int *)&editingPresetCopy_.maxPoints, 8, 256);
				ImGui::TreePop();
			}

			//========================================
			// 幅設定
			if (ImGui::TreeNodeEx("Width Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("Width##editor", &editingPresetCopy_.width, 0.1f, 20.0f);
				ImGui::SliderFloat("Width Multiplier##editor", &editingPresetCopy_.widthMultiplier, 0.1f, 3.0f);
				ImGui::SliderFloat("Start Width##editor", &editingPresetCopy_.startWidth, 0.0f, 2.0f);
				ImGui::SliderFloat("End Width##editor", &editingPresetCopy_.endWidth, 0.0f, 2.0f);
				ImGui::TreePop();
			}

			//========================================
			// カラー設定
			if (ImGui::TreeNodeEx("Color Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::ColorEdit3("Base Color##editor", &editingPresetCopy_.color.x);
				ImGui::SliderFloat("Opacity##editor", &editingPresetCopy_.opacity, 0.0f, 1.0f);
				ImGui::ColorEdit3("Start Color##editor", &editingPresetCopy_.startColor.x);
				ImGui::ColorEdit3("End Color##editor", &editingPresetCopy_.endColor.x);
				ImGui::TreePop();
			}

			//========================================
			// アライメント・シミュレーション空間
			if (ImGui::TreeNodeEx("Alignment & Space", ImGuiTreeNodeFlags_DefaultOpen)) {
				const char *alignItems[] = {"World", "Local", "View"};
				ImGui::Combo("Alignment##editor", (int *)&editingPresetCopy_.alignment, alignItems, IM_ARRAYSIZE(alignItems));

				const char *spaceItems[] = {"World", "Local"};
				ImGui::Combo("Simulation Space##editor", (int *)&editingPresetCopy_.simulationSpace, spaceItems, IM_ARRAYSIZE(spaceItems));
				ImGui::TreePop();
			}

			//========================================
			// マテリアル設定
			if (ImGui::TreeNodeEx("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				static char materialBuffer[64] = {};
				if (editingPresetActive_) {
					strncpy_s(materialBuffer, editingPresetCopy_.material.c_str(), sizeof(materialBuffer) - 1);
				}
				if (ImGui::InputText("Material##editor", materialBuffer, IM_ARRAYSIZE(materialBuffer))) {
					editingPresetCopy_.material = materialBuffer;
				}

				const char *texModeItems[] = {"None", "Tiled", "Stretched"};
				ImGui::Combo("Texture Mode##editor", (int *)&editingPresetCopy_.textureMode, texModeItems, IM_ARRAYSIZE(texModeItems));

				ImGui::DragFloat2("Tiling##editor", &editingPresetCopy_.tiling.x, 0.01f, 0.1f, 10.0f);
				ImGui::DragFloat2("Offset##editor", &editingPresetCopy_.offset.x, 0.01f, 0.0f, 1.0f);
				ImGui::TreePop();
			}

			//========================================
			// 発光・クリア設定
			if (ImGui::TreeNodeEx("Emission Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Emitting##editor", &editingPresetCopy_.emitting);
				ImGui::Checkbox("Auto Clear##editor", &editingPresetCopy_.autoClear);
				ImGui::SliderFloat("Emission Rate##editor", &editingPresetCopy_.emissionRate, 1.0f, 200.0f);
				ImGui::TreePop();
			}

			//========================================
			// その他の設定
			if (ImGui::TreeNodeEx("Other Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("Corner Smooth##editor", &editingPresetCopy_.cornerSmooth, 0.0f, 1.0f);
				ImGui::SliderInt("Subdivisions##editor", (int *)&editingPresetCopy_.subdivisions, 1, 8);
				ImGui::SliderFloat("Noise Amplitude##editor", &editingPresetCopy_.noiseAmplitude, 0.0f, 5.0f);
				ImGui::SliderFloat("Noise Frequency##editor", &editingPresetCopy_.noiseFrequency, 0.1f, 10.0f);
				ImGui::SliderFloat("Velocity Damping##editor", &editingPresetCopy_.velocityDamping, 0.0f, 1.0f);
				ImGui::SliderFloat("Gravity Influence##editor", &editingPresetCopy_.gravityInfluence, 0.0f, 1.0f);

				const char *fadeModeItems[] = {"Linear", "EaseOut", "EaseIn"};
				ImGui::Combo("Fade Mode##editor", (int *)&editingPresetCopy_.fadeMode, fadeModeItems, IM_ARRAYSIZE(fadeModeItems));

				char shaderBuffer[64] = {};
				strncpy_s(shaderBuffer, editingPresetCopy_.shaderName.c_str(), sizeof(shaderBuffer) - 1);
				if (ImGui::InputText("Shader Name##editor", shaderBuffer, IM_ARRAYSIZE(shaderBuffer))) {
					editingPresetCopy_.shaderName = shaderBuffer;
				}
				ImGui::TreePop();
			}

			//========================================
			// リアルタイム反映テスト用
			if (ImGui::Button("Create Preview Instance##editor", ImVec2(200, 0))) {
				std::string previewName = "debug_" + editingPresetName_;
				TrailEffect *existingEffect = GetEffect(previewName);
				if (!existingEffect) {
					CreateFromPreset(editingPresetName_, previewName);
				}
			}
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "(Changes are applied in real-time)");

			ImGui::Separator();

			//========================================
			//========================================
			// 保存・キャンセルボタン
			if (ImGui::Button("Save Preset", ImVec2(150, 0))) {
				UpdatePreset(editingPresetName_, editingPresetCopy_);
				// メモリに確定後、すぐにメインファイルに保存
				SaveAllPresetsToJson("resources/trail/test_preset.json");
				Log("Preset updated and saved: " + editingPresetName_, LogLevel::Success);
			}
			ImGui::SameLine();
			if (ImGui::Button("Close##editor", ImVec2(100, 0))) {
				editingPresetActive_ = false;
			}

			ImGui::EndChild();
		}

		ImGui::Separator();

		//========================================
		// JSON状態表示
		ImGui::Text("Main Preset File: resources/trail/test_preset.json");
		ImGui::Text("(Changes are auto-saved to this file)");

		ImGui::Separator();

		//========================================
		// アクティブエフェクト一覧
		if (ImGui::TreeNode("Active Effects")) {
			for (auto &pair : activeEffects_) {
				if (ImGui::TreeNode(pair.first.c_str())) {
					pair.second->DrawImGui();
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		ImGui::End();
	}

	///=============================================================================
	///						プリセットを登録
	void TrailEffectManager::RegisterPreset(const TrailEffectPreset &preset) {
		//========================================
		// プリセットを登録
		presets_[preset.name] = preset;
		Log("Preset registered: " + preset.name, LogLevel::Success);
	}

	///=============================================================================
	///						プリセットを削除
	void TrailEffectManager::UnregisterPreset(const std::string &presetName) {
		//========================================
		// プリセットを検索
		auto it = presets_.find(presetName);
		if (it == presets_.end()) {
			Log("Preset not found: " + presetName, LogLevel::Warning);
			return;
		}

		//========================================
		// プリセットを削除
		presets_.erase(it);
		Log("Preset unregistered: " + presetName, LogLevel::Success);
	}

	///=============================================================================
	///						プリセットを更新
	void TrailEffectManager::UpdatePreset(const std::string &presetName,
										  const TrailEffectPreset &newPreset) {
		//========================================
		// プリセットを検索
		auto it = presets_.find(presetName);
		if (it == presets_.end()) {
			Log("Preset not found: " + presetName, LogLevel::Warning);
			return;
		}

		//========================================
		// プリセットを更新
		presets_[presetName] = newPreset;
		Log("Preset updated: " + presetName, LogLevel::Success);
	}

	///=============================================================================
	///						プリセットを取得
	TrailEffectPreset *TrailEffectManager::GetPreset(const std::string &presetName) {
		//========================================
		// プリセットを検索
		auto it = presets_.find(presetName);
		if (it == presets_.end()) {
			Log("Preset not found: " + presetName, LogLevel::Warning);
			return nullptr;
		}

		return &it->second;
	}

	///=============================================================================
	///						プリセット一覧を取得
	std::vector<std::string> TrailEffectManager::GetPresetNames() const {
		std::vector<std::string> names;
		for (const auto &pair : presets_) {
			names.push_back(pair.first);
		}
		return names;
	}

	///=============================================================================
	///						プリセットからインスタンスを作成
	TrailEffect *TrailEffectManager::CreateFromPreset(const std::string &presetName,
													  const std::string &instanceName) {
		//========================================
		// Setup が初期化されているかチェック
		if (!setup_) {
			Log("TrailEffectManager: Setup not initialized", LogLevel::Error);
			return nullptr;
		}

		//========================================
		// プリセットを検索
		auto presetIt = presets_.find(presetName);
		if (presetIt == presets_.end()) {
			Log("Preset not found: " + presetName, LogLevel::Warning);
			return nullptr;
		}

		//========================================
		// インスタンスを作成
		auto effect = std::make_unique<TrailEffect>();
		effect->Initialize(setup_, presetIt->second);

		//========================================
		// インスタンスを登録
		TrailEffect *ptr = effect.get();
		activeEffects_[instanceName] = std::move(effect);

		Log("TrailEffect created: " + instanceName + " (Preset: " + presetName + ")",
			LogLevel::Success);
		return ptr;
	}

	///=============================================================================
	///						インスタンスを破棄
	void TrailEffectManager::DestroyEffect(const std::string &instanceName) {
		//========================================
		// インスタンスを検索
		auto it = activeEffects_.find(instanceName);
		if (it == activeEffects_.end()) {
			Log("Effect not found: " + instanceName, LogLevel::Warning);
			return;
		}

		//========================================
		// インスタンスを削除
		activeEffects_.erase(it);
		Log("TrailEffect destroyed: " + instanceName, LogLevel::Success);
	}

	///=============================================================================
	///						インスタンスを取得
	TrailEffect *TrailEffectManager::GetEffect(const std::string &instanceName) {
		//========================================
		// インスタンスを検索
		auto it = activeEffects_.find(instanceName);
		if (it == activeEffects_.end()) {
			return nullptr;
		}

		return it->second.get();
	}

	///=============================================================================
	///						インスタンスを取得（const版）
	const TrailEffect *TrailEffectManager::GetEffect(const std::string &instanceName) const {
		//========================================
		// インスタンスを検索
		auto it = activeEffects_.find(instanceName);
		if (it == activeEffects_.end()) {
			return nullptr;
		}

		return it->second.get();
	}

	///=============================================================================
	///						すべてのエフェクトを破棄
	void TrailEffectManager::DestroyAllEffects() {
		//========================================
		// すべてのインスタンスを削除
		activeEffects_.clear();
		Log("All TrailEffects destroyed", LogLevel::Success);
	}

	///=============================================================================
	///						JSON保存・読み込み機能
	bool TrailEffectManager::SavePresetToJson(const std::string &presetName, const std::string &filePath) {
		//========================================
		// プリセットを取得
		TrailEffectPreset *preset = GetPreset(presetName);
		if (!preset) {
			Log("Preset not found: " + presetName, LogLevel::Warning);
			return false;
		}

		try {
			//========================================
			// jsonオブジェクトを作成
			nlohmann::json j;
			j["name"] = preset->name;

			// ライフタイム設定
			j["lifeTime"] = preset->lifeTime;
			j["minPointDistance"] = preset->minPointDistance;
			j["maxPoints"] = preset->maxPoints;

			// 幅設定
			j["width"] = preset->width;
			j["widthMultiplier"] = preset->widthMultiplier;
			j["startWidth"] = preset->startWidth;
			j["endWidth"] = preset->endWidth;

			// カラー設定
			j["color"] = {{"x", preset->color.x}, {"y", preset->color.y}, {"z", preset->color.z}};
			j["opacity"] = preset->opacity;
			j["startColor"] = {{"x", preset->startColor.x}, {"y", preset->startColor.y}, {"z", preset->startColor.z}};
			j["endColor"] = {{"x", preset->endColor.x}, {"y", preset->endColor.y}, {"z", preset->endColor.z}};

			// アライメント・シミュレーション空間
			j["alignment"] = preset->alignment;
			j["simulationSpace"] = preset->simulationSpace;

			// マテリアル設定
			j["material"] = preset->material;
			j["textureMode"] = preset->textureMode;
			j["tiling"] = {{"x", preset->tiling.x}, {"y", preset->tiling.y}};
			j["offset"] = {{"x", preset->offset.x}, {"y", preset->offset.y}};

			// 発光・クリア設定
			j["emitting"] = preset->emitting;
			j["autoClear"] = preset->autoClear;

			// コーナー・細分化設定
			j["cornerSmooth"] = preset->cornerSmooth;
			j["subdivisions"] = preset->subdivisions;

			// ノイズ設定
			j["noiseAmplitude"] = preset->noiseAmplitude;
			j["noiseFrequency"] = preset->noiseFrequency;

			// フェード設定
			j["fadeMode"] = preset->fadeMode;

			// 物理パラメータ
			j["emissionRate"] = preset->emissionRate;
			j["velocityDamping"] = preset->velocityDamping;
			j["gravityInfluence"] = preset->gravityInfluence;

			// シェーダー情報
			j["shaderName"] = preset->shaderName;

			//========================================
			// ファイルに保存
			std::ofstream file(filePath, std::ios::out);
			if (!file.is_open()) {
				Log("Failed to open file for writing: " + filePath + " (Check path and permissions)", LogLevel::Error);
				return false;
			}
			file << j.dump(2);
			file.close();

			Log("Preset saved successfully: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("JSON save error: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	bool TrailEffectManager::LoadPresetFromJson(const std::string &filePath) {
		try {
			//========================================
			// JSONファイルを読み込み
			std::ifstream file(filePath);
			if (!file.is_open()) {
				Log("Failed to open file: " + filePath, LogLevel::Warning);
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			//========================================
			// プリセットを復元
			TrailEffectPreset preset;
			preset.name = j.value("name", "Unnamed");

			// ライフタイム設定
			preset.lifeTime = j.value("lifeTime", 3.0f);
			preset.minPointDistance = j.value("minPointDistance", 0.1f);
			preset.maxPoints = j.value("maxPoints", 128u);

			// 幅設定
			preset.width = j.value("width", 2.0f);
			preset.widthMultiplier = j.value("widthMultiplier", 1.0f);
			preset.startWidth = j.value("startWidth", 1.0f);
			preset.endWidth = j.value("endWidth", 0.0f);

			// カラー設定
			if (j.contains("color")) {
				preset.color = {j["color"]["x"], j["color"]["y"], j["color"]["z"]};
			}
			preset.opacity = j.value("opacity", 0.8f);
			if (j.contains("startColor")) {
				preset.startColor = {j["startColor"]["x"], j["startColor"]["y"], j["startColor"]["z"]};
			}
			if (j.contains("endColor")) {
				preset.endColor = {j["endColor"]["x"], j["endColor"]["y"], j["endColor"]["z"]};
			}

			// アライメント・シミュレーション空間
			preset.alignment = j.value("alignment", 0u);
			preset.simulationSpace = j.value("simulationSpace", 0u);

			// マテリアル設定
			preset.material = j.value("material", "Default");
			preset.textureMode = j.value("textureMode", 0u);
			if (j.contains("tiling")) {
				preset.tiling = {j["tiling"]["x"], j["tiling"]["y"]};
			}
			if (j.contains("offset")) {
				preset.offset = {j["offset"]["x"], j["offset"]["y"]};
			}

			// 発光・クリア設定
			preset.emitting = j.value("emitting", true);
			preset.autoClear = j.value("autoClear", false);

			// コーナー・細分化設定
			preset.cornerSmooth = j.value("cornerSmooth", 0.1f);
			preset.subdivisions = j.value("subdivisions", 1u);

			// ノイズ設定
			preset.noiseAmplitude = j.value("noiseAmplitude", 0.0f);
			preset.noiseFrequency = j.value("noiseFrequency", 1.0f);

			// フェード設定
			preset.fadeMode = j.value("fadeMode", 0u);

			// 物理パラメータ
			preset.emissionRate = j.value("emissionRate", 50.0f);
			preset.velocityDamping = j.value("velocityDamping", 0.95f);
			preset.gravityInfluence = j.value("gravityInfluence", 0.2f);

			// シェーダー情報
			preset.shaderName = j.value("shaderName", "TrailDefault");

			//========================================
			// プリセットを登録
			RegisterPreset(preset);
			Log("Preset loaded from JSON: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("JSON load error: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	bool TrailEffectManager::SaveAllPresetsToJson(const std::string &filePath) {
		try {
			//========================================
			// JSONオブジェクトを作成
			nlohmann::json j;
			j["presets"] = nlohmann::json::array();

			//========================================
			// すべてのプリセットを保存
			for (const auto &name : GetPresetNames()) {
				TrailEffectPreset *preset = GetPreset(name);
				if (!preset)
					continue;

				nlohmann::json presetJson;
				presetJson["name"] = preset->name;
				presetJson["lifeTime"] = preset->lifeTime;
				presetJson["minPointDistance"] = preset->minPointDistance;
				presetJson["maxPoints"] = preset->maxPoints;
				presetJson["width"] = preset->width;
				presetJson["widthMultiplier"] = preset->widthMultiplier;
				presetJson["startWidth"] = preset->startWidth;
				presetJson["endWidth"] = preset->endWidth;
				presetJson["color"] = {{"x", preset->color.x}, {"y", preset->color.y}, {"z", preset->color.z}};
				presetJson["opacity"] = preset->opacity;
				presetJson["startColor"] = {{"x", preset->startColor.x}, {"y", preset->startColor.y}, {"z", preset->startColor.z}};
				presetJson["endColor"] = {{"x", preset->endColor.x}, {"y", preset->endColor.y}, {"z", preset->endColor.z}};
				presetJson["alignment"] = preset->alignment;
				presetJson["simulationSpace"] = preset->simulationSpace;
				presetJson["material"] = preset->material;
				presetJson["textureMode"] = preset->textureMode;
				presetJson["tiling"] = {{"x", preset->tiling.x}, {"y", preset->tiling.y}};
				presetJson["offset"] = {{"x", preset->offset.x}, {"y", preset->offset.y}};
				presetJson["emitting"] = preset->emitting;
				presetJson["autoClear"] = preset->autoClear;
				presetJson["cornerSmooth"] = preset->cornerSmooth;
				presetJson["subdivisions"] = preset->subdivisions;
				presetJson["noiseAmplitude"] = preset->noiseAmplitude;
				presetJson["noiseFrequency"] = preset->noiseFrequency;
				presetJson["fadeMode"] = preset->fadeMode;
				presetJson["emissionRate"] = preset->emissionRate;
				presetJson["velocityDamping"] = preset->velocityDamping;
				presetJson["gravityInfluence"] = preset->gravityInfluence;
				presetJson["shaderName"] = preset->shaderName;

				j["presets"].push_back(presetJson);
			}

			//========================================
			// ファイルに保存
			std::ofstream file(filePath, std::ios::out);
			if (!file.is_open()) {
				Log("Failed to open file for writing: " + filePath + " (Check path and permissions)", LogLevel::Error);
				return false;
			}
			file << j.dump(2);
			file.close();

			Log("All presets saved successfully: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("JSON save error: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	bool TrailEffectManager::LoadAllPresetsFromJson(const std::string &filePath) {
		try {
			//========================================
			// JSONファイルを読み込み
			std::ifstream file(filePath);
			if (!file.is_open()) {
				Log("Failed to open file: " + filePath, LogLevel::Warning);
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			//========================================
			// プリセット配列または単一プリセットを復元
			std::vector<nlohmann::json> presetsToLoad;

			if (j.contains("presets") && j["presets"].is_array()) {
				// 複数プリセット形式
				presetsToLoad = j["presets"];
			} else if (j.contains("name")) {
				// 単一プリセット形式
				presetsToLoad.push_back(j);
			} else {
				Log("Invalid JSON format: missing 'presets' array or 'name' field", LogLevel::Warning);
				return false;
			}

			//========================================
			// プリセットを復元
			for (const auto &presetJson : presetsToLoad) {
				TrailEffectPreset preset;
				preset.name = presetJson.value("name", "Unnamed");
				preset.lifeTime = presetJson.value("lifeTime", 3.0f);
				preset.minPointDistance = presetJson.value("minPointDistance", 0.1f);
				preset.maxPoints = presetJson.value("maxPoints", 128u);
				preset.width = presetJson.value("width", 2.0f);
				preset.widthMultiplier = presetJson.value("widthMultiplier", 1.0f);
				preset.startWidth = presetJson.value("startWidth", 1.0f);
				preset.endWidth = presetJson.value("endWidth", 0.0f);

				if (presetJson.contains("color")) {
					preset.color = {presetJson["color"]["x"], presetJson["color"]["y"], presetJson["color"]["z"]};
				}
				preset.opacity = presetJson.value("opacity", 0.8f);
				if (presetJson.contains("startColor")) {
					preset.startColor = {presetJson["startColor"]["x"], presetJson["startColor"]["y"], presetJson["startColor"]["z"]};
				}
				if (presetJson.contains("endColor")) {
					preset.endColor = {presetJson["endColor"]["x"], presetJson["endColor"]["y"], presetJson["endColor"]["z"]};
				}

				preset.alignment = presetJson.value("alignment", 0u);
				preset.simulationSpace = presetJson.value("simulationSpace", 0u);
				preset.material = presetJson.value("material", "Default");
				preset.textureMode = presetJson.value("textureMode", 0u);
				if (presetJson.contains("tiling")) {
					preset.tiling = {presetJson["tiling"]["x"], presetJson["tiling"]["y"]};
				}
				if (presetJson.contains("offset")) {
					preset.offset = {presetJson["offset"]["x"], presetJson["offset"]["y"]};
				}

				preset.emitting = presetJson.value("emitting", true);
				preset.autoClear = presetJson.value("autoClear", false);
				preset.cornerSmooth = presetJson.value("cornerSmooth", 0.1f);
				preset.subdivisions = presetJson.value("subdivisions", 1u);
				preset.noiseAmplitude = presetJson.value("noiseAmplitude", 0.0f);
				preset.noiseFrequency = presetJson.value("noiseFrequency", 1.0f);
				preset.fadeMode = presetJson.value("fadeMode", 0u);
				preset.emissionRate = presetJson.value("emissionRate", 50.0f);
				preset.velocityDamping = presetJson.value("velocityDamping", 0.95f);
				preset.gravityInfluence = presetJson.value("gravityInfluence", 0.2f);
				preset.shaderName = presetJson.value("shaderName", "TrailDefault");

				RegisterPreset(preset);
				Log("Preset loaded: " + preset.name, LogLevel::Success);
			}

			Log("All presets loaded from JSON: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("JSON load error: ") + e.what(), LogLevel::Error);
			return false;
		}
	}
}

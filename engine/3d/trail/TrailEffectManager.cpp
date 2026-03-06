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
		// プリセット一覧
		if (ImGui::TreeNode("Presets")) {
			for (const auto &presetName : GetPresetNames()) {
				ImGui::BulletText("%s", presetName.c_str());
			}
			ImGui::TreePop();
		}

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
}

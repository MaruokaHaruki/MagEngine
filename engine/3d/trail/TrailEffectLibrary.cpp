/*********************************************************************
 * \file   TrailEffectLibrary.cpp
 * \brief  トレイルエフェクトライブラリ実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#include "TrailEffectLibrary.h"
#include "Logger.h"
#include "TrailEffectManager.h"
#include "externals/json.hpp"
#include <fstream>
#include <sstream>

using namespace Logger;
using json = nlohmann::json;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						複数プリセットをファイルから読み込み
	bool TrailEffectLibrary::LoadPresets(const std::string &filePath, TrailEffectManager &manager) {
		//========================================
		// ファイル開く
		std::ifstream file(filePath);
		if (!file.is_open()) {
			Log("Failed to open file: " + filePath, LogLevel::Error);
			return false;
		}

		try {
			//========================================
			// JSONを解析
			json j;
			file >> j;

			//========================================
			// プリセット配列を取得
			if (!j.contains("trailEffectPresets")) {
				Log("No 'trailEffectPresets' key found in JSON", LogLevel::Warning);
				return false;
			}

			auto presetsArray = j["trailEffectPresets"];

			//========================================
			// 各プリセットをマネージャに登録
			for (const auto &presetJson : presetsArray) {
				TrailEffectPreset preset = JsonToPreset(&presetJson);
				manager.RegisterPreset(preset);
			}

			file.close();
			Log("Presets loaded successfully from: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("Failed to load presets: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	///						複数プリセットをファイルに保存
	bool TrailEffectLibrary::SavePresets(const std::string &filePath, const TrailEffectManager &manager) {
		try {
			//========================================
			// JSON構造を作成
			json j;
			j["version"] = "1.0";

			//========================================
			// プリセット配列を作成
			json presetsArray = json::array();
			for (const auto &presetName : manager.GetPresetNames()) {
				auto preset = const_cast<TrailEffectManager &>(manager).GetPreset(presetName);
				if (preset) {
					presetsArray.push_back(*(json *)PresetToJson(*preset));
				}
			}

			j["trailEffectPresets"] = presetsArray;

			//========================================
			// ファイルに書き込み
			std::ofstream file(filePath);
			if (!file.is_open()) {
				Log("Failed to open file for writing: " + filePath, LogLevel::Error);
				return false;
			}

			file << j.dump(4); // インデント4で見やすく
			file.close();

			Log("Presets saved successfully to: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("Failed to save presets: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	///						単一プリセットをファイルから読み込み
	bool TrailEffectLibrary::LoadPreset(const std::string &filePath, TrailEffectPreset &outPreset) {
		//========================================
		// ファイル開く
		std::ifstream file(filePath);
		if (!file.is_open()) {
			Log("Failed to open file: " + filePath, LogLevel::Error);
			return false;
		}

		try {
			//========================================
			// JSONを解析
			json j;
			file >> j;

			//========================================
			// プリセットに変換
			outPreset = JsonToPreset(&j);

			file.close();
			Log("Preset loaded successfully from: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("Failed to load preset: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	///						単一プリセットをファイルに保存
	bool TrailEffectLibrary::SavePreset(const std::string &filePath, const TrailEffectPreset &preset) {
		try {
			//========================================
			// JSON構造を作成
			auto j = *(json *)PresetToJson(preset);

			//========================================
			// ファイルに書き込み
			std::ofstream file(filePath);
			if (!file.is_open()) {
				Log("Failed to open file for writing: " + filePath, LogLevel::Error);
				return false;
			}

			file << j.dump(4);
			file.close();

			Log("Preset saved successfully to: " + filePath, LogLevel::Success);
			return true;
		} catch (const std::exception &e) {
			Log(std::string("Failed to save preset: ") + e.what(), LogLevel::Error);
			return false;
		}
	}

	///=============================================================================
	///						JSONからプリセットに変換
	TrailEffectPreset TrailEffectLibrary::JsonToPreset(const void *j) {
		const json *jsonPtr = static_cast<const json *>(j);
		TrailEffectPreset preset;

		//========================================
		// 基本情報
		if (jsonPtr->contains("name")) {
			preset.name = jsonPtr->at("name").get<std::string>();
		}

		//========================================
		// ビジュアルパラメータ
		if (jsonPtr->contains("color")) {
			const auto &color = jsonPtr->at("color");
			preset.color = {
				color.at("x").get<float>(),
				color.at("y").get<float>(),
				color.at("z").get<float>()};
		}

		if (jsonPtr->contains("opacity")) {
			preset.opacity = jsonPtr->at("opacity").get<float>();
		}

		if (jsonPtr->contains("width")) {
			preset.width = jsonPtr->at("width").get<float>();
		}

		//========================================
		// ライフサイクル
		if (jsonPtr->contains("lifeTime")) {
			preset.lifeTime = jsonPtr->at("lifeTime").get<float>();
		}

		if (jsonPtr->contains("emissionRate")) {
			preset.emissionRate = jsonPtr->at("emissionRate").get<float>();
		}

		//========================================
		// 物理パラメータ
		if (jsonPtr->contains("velocityDamping")) {
			preset.velocityDamping = jsonPtr->at("velocityDamping").get<float>();
		}

		if (jsonPtr->contains("gravityInfluence")) {
			preset.gravityInfluence = jsonPtr->at("gravityInfluence").get<float>();
		}

		//========================================
		// シェーダー情報
		if (jsonPtr->contains("shaderName")) {
			preset.shaderName = jsonPtr->at("shaderName").get<std::string>();
		}

		//========================================
		// カスタムパラメータ
		if (jsonPtr->contains("customParams")) {
			const auto &customParams = jsonPtr->at("customParams");
			for (auto it = customParams.begin(); it != customParams.end(); ++it) {
				preset.customParams[it.key()] = it.value().get<float>();
			}
		}

		return preset;
	}

	///=============================================================================
	///						プリセットからJSONに変換
	void *TrailEffectLibrary::PresetToJson(const TrailEffectPreset &preset) {
		auto *j = new json();

		//========================================
		// 基本情報
		(*j)["name"] = preset.name;

		//========================================
		// ビジュアルパラメータ
		(*j)["color"] = {
			{"x", preset.color.x},
			{"y", preset.color.y},
			{"z", preset.color.z}};

		(*j)["opacity"] = preset.opacity;
		(*j)["width"] = preset.width;

		//========================================
		// ライフサイクル
		(*j)["lifeTime"] = preset.lifeTime;
		(*j)["emissionRate"] = preset.emissionRate;

		//========================================
		// 物理パラメータ
		(*j)["velocityDamping"] = preset.velocityDamping;
		(*j)["gravityInfluence"] = preset.gravityInfluence;

		//========================================
		// シェーダー情報
		(*j)["shaderName"] = preset.shaderName;

		//========================================
		// カスタムパラメータ
		json customParams;
		for (const auto &pair : preset.customParams) {
			customParams[pair.first] = pair.second;
		}
		(*j)["customParams"] = customParams;

		return j;
	}
}

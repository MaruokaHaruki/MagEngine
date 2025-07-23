#include "LevelDataLoader.h"
#include <fstream>

///=============================================================================
///                        初期化
void LevelDataLoader::Initialize() {
	isLoaded_ = false;
	levelData_ = {};		   // 空のレベルデータで初期化
	selectedObjectIndex_ = -1; // 選択なし
	Logger::Log("LevelDataLoader initialized", Logger::LogLevel::Info);
}

///=============================================================================
///                        Jsonファイルからレベルデータを読み込み
bool LevelDataLoader::LoadLevelFromJson(const std::string &filePath) {
	try {
		// JSONファイルを開く
		std::ifstream file(filePath);
		if (!file.is_open()) {
			Logger::Log("Failed to open JSON file: " + filePath, Logger::LogLevel::Error);
			return false;
		}

		// JSONをパース
		nlohmann::json jsonData;
		file >> jsonData;
		file.close();

		Logger::Log("Successfully loaded JSON file: " + filePath, Logger::LogLevel::Success);

		// レベルデータをクリア
		levelData_ = {};

		// シーン名を取得（デフォルト値を設定）
		levelData_.name = jsonData.value("name", "unnamed_scene");

		// オブジェクト配列を処理
		if (jsonData.contains("objects") && jsonData["objects"].is_array()) {
			for (const auto &objJson : jsonData["objects"]) {
				auto levelObject = ParseObjectFromJson(objJson);
				if (levelObject) {
					levelData_.objects.push_back(std::move(levelObject));
				}
			}
		}

		isLoaded_ = true;
		Logger::Log("Level data loaded successfully. Objects count: " + std::to_string(levelData_.objects.size()), Logger::LogLevel::Success);
		return true;

	} catch (const std::exception &e) {
		Logger::Log("JSON parsing error: " + std::string(e.what()), Logger::LogLevel::Error);
		isLoaded_ = false;
		return false;
	}
}

///=============================================================================
///                        JSONオブジェクトからLevelObjectを作成（再帰処理）
std::unique_ptr<LevelObject> LevelDataLoader::ParseObjectFromJson(const nlohmann::json &jsonObj) {
	auto levelObject = std::make_unique<LevelObject>();

	// 基本情報を取得
	levelObject->name = jsonObj.value("name", "unnamed_object");
	levelObject->type = jsonObj.value("type", "UNKNOWN");
	levelObject->file_name = jsonObj.value("file_name", "");

	// トランスフォーム情報を取得と座標変換
	if (jsonObj.contains("transform")) {
		const auto &transform = jsonObj["transform"];

		// Blender座標系からエンジン座標系への変換を適用
		Vector3 blenderTranslation = GetVector3FromJson(transform["translation"]);
		Vector3 blenderRotation = GetVector3FromJson(transform["rotation"]);
		Vector3 blenderScale = GetVector3FromJson(transform["scale"], {1, 1, 1});

		// 座標変換を実行（エンジンのTransformメンバー名に合わせる）
		levelObject->transform.translate = ConvertPositionFromBlender(blenderTranslation);
		levelObject->transform.rotate = ConvertRotationFromBlender(blenderRotation);
		levelObject->transform.scale = blenderScale; // スケールは変換不要
	}

	// コライダー情報を取得（存在する場合のみ）
	if (jsonObj.contains("collider")) {
		const auto &colliderJson = jsonObj["collider"];
		auto collider = std::make_unique<LevelCollider>();

		collider->type = colliderJson.value("type", "BOX");

		// コライダーの中心位置とサイズも座標変換を適用
		Vector3 blenderCenter = GetVector3FromJson(colliderJson["center"]);
		collider->center = ConvertPositionFromBlender(blenderCenter);
		collider->size = GetVector3FromJson(colliderJson["size"], {1, 1, 1});
		// 注意: コライダーサイズは相対的な値なので、Z成分の符号は変更しない

		levelObject->collider = std::move(collider);
	}

	// 子オブジェクトを再帰的に処理
	if (jsonObj.contains("children") && jsonObj["children"].is_array()) {
		for (const auto &childJson : jsonObj["children"]) {
			auto childObject = ParseObjectFromJson(childJson);
			if (childObject) {
				levelObject->children.push_back(std::move(childObject));
			}
		}
	}

	return levelObject;
}

///=============================================================================
///                        Blender（右手系）からエンジン（左手系）への座標変換
Vector3 LevelDataLoader::ConvertPositionFromBlender(const Vector3 &blenderPos) {
	// Blender（右手系、Y-up）からエンジン（左手系、Y-up）への変換
	// 変換式: X' = X, Y' = Y, Z' = -Z
	return Vector3{
		blenderPos.x, // X軸はそのまま
		blenderPos.y, // Y軸はそのまま（両方ともY-up）
		-blenderPos.z // Z軸は反転（右手系→左手系）
	};
}

///=============================================================================
///                        Blender（右手系）からエンジン（左手系）への回転変換
Vector3 LevelDataLoader::ConvertRotationFromBlender(const Vector3 &blenderRot) {
	// Blender（右手系）からエンジン（左手系）への回転変換
	// 右手系から左手系への変換では、Y軸とZ軸周りの回転方向が反転する
	// 注意: この変換は一般的なケースであり、特定の実装によって調整が必要な場合がある
	return Vector3{
		blenderRot.x,  // X軸周りの回転はそのまま
		-blenderRot.y, // Y軸周りの回転は反転
		-blenderRot.z  // Z軸周りの回転は反転
	};
}

///=============================================================================
///                        JSONからVector3を安全に取得
Vector3 LevelDataLoader::GetVector3FromJson(const nlohmann::json &jsonArray, const Vector3 &defaultValue) {
	// JSON配列から安全にVector3を取得
	if (!jsonArray.is_array() || jsonArray.size() < 3) {
		Logger::Log("Invalid JSON array for Vector3, using default value", Logger::LogLevel::Warning);
		return defaultValue;
	}

	try {
		return Vector3{
			jsonArray[0].get<float>(),
			jsonArray[1].get<float>(),
			jsonArray[2].get<float>()};
	} catch (const std::exception &e) {
		Logger::Log("Error parsing Vector3 from JSON: " + std::string(e.what()), Logger::LogLevel::Error);
		return defaultValue;
	}
}

///=============================================================================
///                        更新
void LevelDataLoader::Update() {
	// 現在は特に更新処理なし
	// 必要に応じて、動的レベル読み込みやホットリロード機能を追加すると便利
}

///=============================================================================
///                        描画
void LevelDataLoader::Draw() {
	// 現在は特に描画処理なし
	// デバッグ表示が必要な場合は、レベルオブジェクトの情報をImGuiで表示する
}

///=============================================================================
///                        読み込んだレベルデータをObject3Dリストに変換してシーンに配置
bool LevelDataLoader::CreateObjectsFromLevelData(Object3dSetup *object3dSetup, std::vector<std::unique_ptr<Object3d>> &outObjectList) {
	// レベルデータが読み込まれているかチェック
	if (!isLoaded_ || !object3dSetup) {
		Logger::Log("Level data not loaded or invalid object3dSetup", Logger::LogLevel::Error);
		return false;
	}

	// 既存のオブジェクトリストをクリア
	outObjectList.clear();

	// ルートオブジェクトを順次処理
	for (const auto &rootObject : levelData_.objects) {
		if (rootObject) {
			// 各ルートオブジェクトを再帰的にObject3Dに変換
			Transform rootTransform = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
			CreateObject3DFromLevelObject(rootObject, object3dSetup, outObjectList, rootTransform);
		}
	}

	Logger::Log("Successfully created " + std::to_string(outObjectList.size()) + " Object3D instances from level data", Logger::LogLevel::Success);
	return true;
}

///=============================================================================
///                        LevelObjectからObject3Dを再帰的に作成してリストに追加
void LevelDataLoader::CreateObject3DFromLevelObject(const std::unique_ptr<LevelObject> &levelObject,
													Object3dSetup *object3dSetup,
													std::vector<std::unique_ptr<Object3d>> &outObjectList,
													const Transform &parentTransform) {
	if (!levelObject) {
		return;
	}

	// 新しいObject3Dを作成
	auto object3d = std::make_unique<Object3d>();
	object3d->Initialize(object3dSetup);

	// モデルファイルが指定されている場合は設定
	if (!levelObject->file_name.empty()) {
		try {
			object3d->SetModel(levelObject->file_name);
			Logger::Log("Set model: " + levelObject->file_name + " for object: " + levelObject->name, Logger::LogLevel::Info);
		} catch (const std::exception &e) {
			Logger::Log("Failed to set model " + levelObject->file_name + " for object " + levelObject->name + ": " + e.what() + " - Using default axisPlus.obj", Logger::LogLevel::Warning);
			// モデル設定に失敗した場合、デフォルトのaxisPlus.objを設定
			try {
				object3d->SetModel("axisPlus.obj");
			} catch (const std::exception &defaultError) {
				Logger::Log("Failed to set default model axisPlus.obj: " + std::string(defaultError.what()), Logger::LogLevel::Error);
			}
		}
	} else {
		Logger::Log("No model file specified for object: " + levelObject->name + " - Using default axisPlus.obj", Logger::LogLevel::Info);
		// モデルファイルが指定されていない場合もデフォルトのaxisPlus.objを設定
		try {
			object3d->SetModel("axisPlus.obj");
		} catch (const std::exception &e) {
			Logger::Log("Failed to set default model axisPlus.obj: " + std::string(e.what()), Logger::LogLevel::Error);
		}
	}

	// 親のトランスフォームと現在のオブジェクトのトランスフォームを合成
	Transform combinedTransform = CombineTransforms(parentTransform, levelObject->transform);

	// Object3Dにトランスフォームを設定（エンジンのメソッド名に合わせる）
	object3d->SetScale(combinedTransform.scale);
	object3d->SetRotation(combinedTransform.rotate);
	object3d->SetPosition(combinedTransform.translate);

	// Object3Dを更新してワールド行列を確定
	object3d->Update();

	// リストに追加
	outObjectList.push_back(std::move(object3d));

	// 子オブジェクトを再帰的に処理
	for (const auto &child : levelObject->children) {
		if (child) {
			// 現在のオブジェクトのトランスフォームを親として子を処理
			CreateObject3DFromLevelObject(child, object3dSetup, outObjectList, combinedTransform);
		}
	}
}

///=============================================================================
///                        2つのTransformを合成（親→子の順で適用）
Transform LevelDataLoader::CombineTransforms(const Transform &parent, const Transform &child) {
	Transform combined;

	// スケールの合成（乗算）
	combined.scale.x = parent.scale.x * child.scale.x;
	combined.scale.y = parent.scale.y * child.scale.y;
	combined.scale.z = parent.scale.z * child.scale.z;

	// 回転の合成（度数法での加算）
	// 注意: 実際のゲームエンジンではクォータニオンを使用するべきだが、
	//       ここでは簡易的にオイラー角の加算で処理
	combined.rotate.x = parent.rotate.x + child.rotate.x;
	combined.rotate.y = parent.rotate.y + child.rotate.y;
	combined.rotate.z = parent.rotate.z + child.rotate.z;

	// 平行移動の合成
	// 子の位置を親のスケールとローテーションを考慮して変換
	// 簡易実装: スケールのみ適用（回転変換は複雑なため省略）
	combined.translate.x = parent.translate.x + (child.translate.x * parent.scale.x);
	combined.translate.y = parent.translate.y + (child.translate.y * parent.scale.y);
	combined.translate.z = parent.translate.z + (child.translate.z * parent.scale.z);

	return combined;
}

///=============================================================================
///                        ImGui描画（デバッグ用）
void LevelDataLoader::ImGuiDraw(std::vector<std::unique_ptr<Object3d>> &outObjectList) {
	if (!isLoaded_ || outObjectList.empty()) {
		ImGui::Text("No level objects loaded");
		return;
	}

	ImGui::Text("Level Objects: %zu", outObjectList.size());

	// オブジェクト選択コンボボックス
	if (ImGui::BeginCombo("Select Object", selectedObjectIndex_ >= 0 ? std::to_string(selectedObjectIndex_).c_str() : "None")) {
		for (int i = 0; i < static_cast<int>(outObjectList.size()); i++) {
			bool isSelected = (selectedObjectIndex_ == i);
			if (ImGui::Selectable(("Object " + std::to_string(i)).c_str(), isSelected)) {
				selectedObjectIndex_ = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// 選択されたオブジェクトの操作
	if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(outObjectList.size())) {
		auto &selectedObject = outObjectList[selectedObjectIndex_];
		if (selectedObject) {
			ImGui::Separator();
			ImGui::Text("Object %d Controls", selectedObjectIndex_);

			// 現在の位置を取得
			Vector3 position = selectedObject->GetPosition();
			Vector3 rotation = selectedObject->GetRotation();
			Vector3 scale = selectedObject->GetScale();

			// 位置の編集
			if (ImGui::SliderFloat3("Position", &position.x, -50.0f, 50.0f)) {
				selectedObject->SetPosition(position);
			}

			// 回転の編集
			if (ImGui::SliderFloat3("Rotation", &rotation.x, -180.0f, 180.0f)) {
				selectedObject->SetRotation(rotation);
			}

			// スケールの編集
			if (ImGui::SliderFloat3("Scale", &scale.x, 0.1f, 10.0f)) {
				selectedObject->SetScale(scale);
			}

			// リセットボタン
			if (ImGui::Button("Reset Transform")) {
				selectedObject->SetPosition({0.0f, 0.0f, 0.0f});
				selectedObject->SetRotation({0.0f, 0.0f, 0.0f});
				selectedObject->SetScale({1.0f, 1.0f, 1.0f});
			}
		}
	}
}

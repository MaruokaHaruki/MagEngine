// LightManager.cpp
/*********************************************************************
 * \file   LightManager.cpp
 * \brief  ライト管理用クラス
 *a
 * \author YourName
 * \date   March 2025
 * \note   
 *********************************************************************/
#include "LightManager.h"
#include "ImguiSetup.h"
#include "Logger.h"
using namespace Logger;

///=============================================================================
///						初期化
void LightManager::Initialize() {
    // デフォルトの平行光源を追加
    DirectionalLight mainDirLight{};
    mainDirLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainDirLight.direction = { 0.0f, -1.0f, 0.0f };
    mainDirLight.intensity = 0.8f;
    directionalLights_["Main"] = mainDirLight;

    // デフォルトのポイントライト追加
    PointLight mainPointLight{};
    mainPointLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainPointLight.position = { 0.0f, 2.0f, 0.0f };
    mainPointLight.intensity = 1.0f;
    mainPointLight.radius = 10.0f;
    mainPointLight.decay = 1.0f;
    pointLights_["Main"] = mainPointLight;

    // デフォルトのスポットライト追加
    SpotLight mainSpotLight{};
    mainSpotLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainSpotLight.position = { 0.0f, 5.0f, 0.0f };
    mainSpotLight.direction = { 0.0f, -1.0f, 0.0f };
    mainSpotLight.intensity = 1.0f;
    mainSpotLight.distance = 15.0f;
    mainSpotLight.decay = 1.5f;
    mainSpotLight.cosAngle = cosf(0.5f);
    spotLights_["Main"] = mainSpotLight;

    Log("LightManager initialized", LogLevel::Info);
}

///=============================================================================
///						終了処理
void LightManager::Finalize() {
	directionalLights_.clear();
	pointLights_.clear();
	spotLights_.clear();
	Log("LightManager finalized", LogLevel::Info);
}

///=============================================================================
///						更新
void LightManager::Update() {
    // 必要に応じてライトの位置や方向を更新
}

///=============================================================================
///						ImGuiの描画
void LightManager::DrawImGui() {
    ImGui::Begin("Light Manager");

    if (ImGui::CollapsingHeader("Directional Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int selectedIndex = 0;
        const char* items = "Main\0Custom1\0Custom2\0\0";
        if (ImGui::Combo("Active Directional Light", &selectedIndex, items)) {
            // 選択に基づいてライト名を設定
            switch (selectedIndex) {
                case 0: SetActiveDirectionalLight("Main"); break;
                case 1: SetActiveDirectionalLight("Custom1"); break;
                case 2: SetActiveDirectionalLight("Custom2"); break;
            }
        }
        
        DirectionalLight& light = directionalLights_[activeDirectionalLightName_];
        ImGui::ColorEdit4("Color##DirLight", &light.color.x);
        ImGui::DragFloat3("Direction##DirLight", &light.direction.x, 0.01f, -1.0f, 1.0f);
        ImGui::SliderFloat("Intensity##DirLight", &light.intensity, 0.0f, 5.0f);
    }

    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int selectedIndex = 0;
        const char* items = "Main\0Custom1\0Custom2\0\0";
        if (ImGui::Combo("Active Point Light", &selectedIndex, items)) {
            // 選択に基づいてライト名を設定
            switch (selectedIndex) {
                case 0: SetActivePointLight("Main"); break;
                case 1: SetActivePointLight("Custom1"); break;
                case 2: SetActivePointLight("Custom2"); break;
            }
        }
        
        PointLight& light = pointLights_[activePointLightName_];
        ImGui::ColorEdit4("Color##PointLight", &light.color.x);
        ImGui::DragFloat3("Position##PointLight", &light.position.x, 0.1f);
        ImGui::SliderFloat("Intensity##PointLight", &light.intensity, 0.0f, 5.0f);
        ImGui::SliderFloat("Radius##PointLight", &light.radius, 0.1f, 50.0f);
        ImGui::SliderFloat("Decay##PointLight", &light.decay, 0.0f, 5.0f);
    }

    if (ImGui::CollapsingHeader("Spot Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int selectedIndex = 0;
        const char* items = "Main\0Custom1\0Custom2\0\0";
        if (ImGui::Combo("Active Spot Light", &selectedIndex, items)) {
            // 選択に基づいてライト名を設定
            switch (selectedIndex) {
                case 0: SetActiveSpotLight("Main"); break;
                case 1: SetActiveSpotLight("Custom1"); break;
                case 2: SetActiveSpotLight("Custom2"); break;
            }
        }
        
        SpotLight& light = spotLights_[activeSpotLightName_];
        ImGui::ColorEdit4("Color##SpotLight", &light.color.x);
        ImGui::DragFloat3("Position##SpotLight", &light.position.x, 0.1f);
        ImGui::DragFloat3("Direction##SpotLight", &light.direction.x, 0.01f, -1.0f, 1.0f);
        ImGui::SliderFloat("Intensity##SpotLight", &light.intensity, 0.0f, 5.0f);
        ImGui::SliderFloat("Distance##SpotLight", &light.distance, 0.1f, 50.0f);
        ImGui::SliderFloat("Decay##SpotLight", &light.decay, 0.0f, 5.0f);
        
        float angleRad = acosf(light.cosAngle);
        float angleDeg = angleRad * 180.0f / 3.14159f;
        if (ImGui::SliderFloat("Angle (degrees)##SpotLight", &angleDeg, 0.0f, 90.0f)) {
            light.cosAngle = cosf(angleDeg * 3.14159f / 180.0f);
        }
    }

    ImGui::End();
}

///=============================================================================
///						ディレクショナルライトの追加
void LightManager::AddDirectionalLight(const std::string& name, const Vector4& color, 
    const Vector3& direction, float intensity) {
    DirectionalLight light{};
    light.color = color;
    light.direction = direction;
    light.intensity = intensity;
    directionalLights_[name] = light;
}

///=============================================================================
///						ディレクショナルライトの取得
const DirectionalLight& LightManager::GetDirectionalLight(const std::string& name) const {
    std::string lightName = name.empty() ? activeDirectionalLightName_ : name;
    auto it = directionalLights_.find(lightName);
    if (it != directionalLights_.end()) {
        return it->second;
    }
    return directionalLights_.at("Main");
}

///=============================================================================
///						アクティブなディレクショナルライトの設定
void LightManager::SetActiveDirectionalLight(const std::string& name) {
    if (directionalLights_.find(name) != directionalLights_.end()) {
        activeDirectionalLightName_ = name;
    }
}

///=============================================================================
///						ポイントライトの追加
void LightManager::AddPointLight(const std::string& name, const Vector4& color, 
    const Vector3& position, float intensity, float radius, float decay) {
    PointLight light{};
    light.color = color;
    light.position = position;
    light.intensity = intensity;
    light.radius = radius;
    light.decay = decay;
    pointLights_[name] = light;
}

///=============================================================================
///						ポイントライトの取得
const PointLight& LightManager::GetPointLight(const std::string& name) const {
    std::string lightName = name.empty() ? activePointLightName_ : name;
    auto it = pointLights_.find(lightName);
    if (it != pointLights_.end()) {
        return it->second;
    }
    return pointLights_.at("Main");
}

///=============================================================================
///						アクティブなポイントライトの設定
void LightManager::SetActivePointLight(const std::string& name) {
    if (pointLights_.find(name) != pointLights_.end()) {
        activePointLightName_ = name;
    }
}

///=============================================================================
///						スポットライトの追加
void LightManager::AddSpotLight(const std::string& name, const Vector4& color,
    const Vector3& position, const Vector3& direction, float intensity,
    float distance, float decay, float angle) {
    SpotLight light{};
    light.color = color;
    light.position = position;
    light.direction = direction;
    light.intensity = intensity;
    light.distance = distance;
    light.decay = decay;
    light.cosAngle = cosf(angle);
    spotLights_[name] = light;
}

///=============================================================================
///						スポットライトの取得
const SpotLight& LightManager::GetSpotLight(const std::string& name) const {
    std::string lightName = name.empty() ? activeSpotLightName_ : name;
    auto it = spotLights_.find(lightName);
    if (it != spotLights_.end()) {
        return it->second;
    }
    return spotLights_.at("Main");
}

///=============================================================================
///						アクティブなスポットライトの設定
void LightManager::SetActiveSpotLight(const std::string& name) {
    if (spotLights_.find(name) != spotLights_.end()) {
        activeSpotLightName_ = name;
    }
}
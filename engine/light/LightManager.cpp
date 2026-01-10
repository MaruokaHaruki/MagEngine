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
#include "LineManager.h"
#include "Logger.h"

///=============================================================================
///						初期化
void LightManager::Initialize() {
    // デフォルトの平行光源を追加
    MagMath::DirectionalLight mainDirLight{};
    mainDirLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainDirLight.direction = { 0.0f, -1.0f, 0.0f };
    mainDirLight.intensity = 0.8f;
    directionalLights_["Main"] = mainDirLight;

    // デフォルトのポイントライト追加
    MagMath::PointLight mainPointLight{};
    mainPointLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainPointLight.position = { 0.0f, 2.0f, 0.0f };
    mainPointLight.intensity = 1.0f;
    mainPointLight.radius = 10.0f;
    mainPointLight.decay = 1.0f;
    pointLights_["Main"] = mainPointLight;

    // デフォルトのスポットライト追加
    MagMath::SpotLight mainSpotLight{};
    mainSpotLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    mainSpotLight.position = { 0.0f, 5.0f, 0.0f };
    mainSpotLight.direction = { 0.0f, -1.0f, 0.0f };
    mainSpotLight.intensity = 1.0f;
    mainSpotLight.distance = 15.0f;
    mainSpotLight.decay = 1.5f;
    mainSpotLight.cosAngle = cosf(0.5f);
    spotLights_["Main"] = mainSpotLight;

    Logger::Log("LightManager initialized", Logger::LogLevel::Info);
}

///=============================================================================
///						終了処理
void LightManager::Finalize() {
	directionalLights_.clear();
	pointLights_.clear();
	spotLights_.clear();
	Logger::Log("LightManager finalized", Logger::LogLevel::Info);
}

///=============================================================================
///						更新
void LightManager::Update() {
    // 必要に応じてライトの位置や方向を更新
    
    // デバッグ表示の更新
    if (lineManager_ && showLightDebug_) {
        DrawLightDebugLines();
    }
}

///=============================================================================
///						ImGuiの描画
void LightManager::DrawImGui() {
    ImGui::Begin("Light Manager");

    // デバッグ表示設定
    if (ImGui::CollapsingHeader("Debug Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show Light Debug", &showLightDebug_);
        if (showLightDebug_) {
            ImGui::Indent();
            ImGui::Checkbox("Directional Lights", &showDirectionalLightDebug_);
            ImGui::Checkbox("Point Lights", &showPointLightDebug_);
            ImGui::Checkbox("Spot Lights", &showSpotLightDebug_);
            
            // 表示スタイル設定を追加
            ImGui::Separator();
            ImGui::Checkbox("Show Labels", &showDebugLabels_);
            ImGui::Checkbox("Show Parameters", &showDebugParameters_);
            ImGui::SliderFloat("Line Thickness", &debugLineThickness_, 1.0f, 5.0f);
            ImGui::SliderFloat("Debug Scale", &debugLightScale_, 0.1f, 10.0f);
            
            ImGui::Unindent();
        }
        ImGui::Separator();
    }

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
        
        MagMath::DirectionalLight& light = directionalLights_[activeDirectionalLightName_];
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
        
        MagMath::PointLight& light = pointLights_[activePointLightName_];
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
        
        MagMath::SpotLight& light = spotLights_[activeSpotLightName_];
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
///						ライト情報をラインで描画
void LightManager::DrawLightDebugLines() {
    if (!lineManager_ || !showLightDebug_) return;
    
    // 現在アクティブなライトを可視化
    if (showDirectionalLightDebug_) {
        VisualizeDirectionalLight("");
    }
    
    if (showPointLightDebug_) {
        VisualizePointLight("");
    }
    
    if (showSpotLightDebug_) {
        VisualizeSpotLight("");
    }
}

///=============================================================================
///						ディレクショナルライトの追加
void LightManager::AddDirectionalLight(const std::string& name, const MagMath::Vector4& color, 
    const MagMath::Vector3& direction, float intensity) {
    MagMath::DirectionalLight light{};
    light.color = color;
    light.direction = direction;
    light.intensity = intensity;
    directionalLights_[name] = light;
}

///=============================================================================
///						ディレクショナルライトの取得
const MagMath::DirectionalLight& LightManager::GetDirectionalLight(const std::string& name) const {
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
///						ディレクショナルライトの可視化
void LightManager::VisualizeDirectionalLight(const std::string& lightName) {
    if (!lineManager_) return;
    
    const MagMath::DirectionalLight& light = GetDirectionalLight(lightName);
    if (light.intensity <= 0) return;  // 強度が0以下なら表示しない
    
    // カメラの位置を取得（シーンのカメラから取得することが望ましい）
    // ここではデモとして固定位置を使用
    MagMath::Vector3 cameraPos = {0.0f, 2.0f, -5.0f};
    
    // ディレクショナルライトの基準点（カメラからの相対位置）
    MagMath::Vector3 center = {
        cameraPos.x,
        cameraPos.y + 3.0f,
        cameraPos.z + 5.0f
    };
    
    // 方向を表す矢印の長さ（強度に応じて変化）
    float arrowLength = 3.0f * debugLightScale_ * light.intensity;
    
    // 方向ベクトル
    MagMath::Vector3 direction = {
        light.direction.x * arrowLength,
        light.direction.y * arrowLength,
        light.direction.z * arrowLength
    };
    
    // 正規化された方向ベクトル（矢印の先端用）
    MagMath::Vector3 normalizedDir;
    float dirLength = sqrtf(light.direction.x * light.direction.x + 
                           light.direction.y * light.direction.y + 
                           light.direction.z * light.direction.z);
                           
    if (dirLength > 0.0001f) {
        normalizedDir = {
            light.direction.x / dirLength,
            light.direction.y / dirLength,
            light.direction.z / dirLength
        };
    } else {
        normalizedDir = {0.0f, -1.0f, 0.0f}; // デフォルト方向
    }
    
    // 方向ベクトルの先端
    MagMath::Vector3 arrowTip = {
        center.x + direction.x,
        center.y + direction.y,
        center.z + direction.z
    };
    
    // ライトの色を取得（強度に応じて明るさを調整）
    MagMath::Vector4 color = {
        light.color.x * light.intensity,
        light.color.y * light.intensity,
        light.color.z * light.intensity,
        1.0f
    };
    
    // 中心から方向への線を描画（太さを設定）
    lineManager_->DrawLine(center, arrowTip, color, debugLineThickness_);
    
    // 中心点のマーカー（太陽のシンボル）
    float sunSize = 0.3f * debugLightScale_;
    lineManager_->DrawSunSymbol(center, sunSize, color, debugLineThickness_);
    
    // 矢印の先端のサイズ
    float arrowheadSize = 0.2f * debugLightScale_;
    
    // 矢印の先端
    lineManager_->DrawArrowhead(arrowTip, normalizedDir, arrowheadSize, color, debugLineThickness_);
    
    // ラベルとパラメータ表示
    if (showDebugLabels_) {
        lineManager_->DrawText3D(center, lightName.empty() ? activeDirectionalLightName_ : lightName, {1.0f, 1.0f, 0.5f, 1.0f});
    }
    
    if (showDebugParameters_) {
        // パラメータ表示
        char paramText[128];
        sprintf_s(paramText, "Dir:(%.1f,%.1f,%.1f)\nInt:%.2f",
                light.direction.x, light.direction.y, light.direction.z, light.intensity);
        lineManager_->DrawText3D({center.x, center.y - 0.5f, center.z}, paramText, {0.8f, 0.8f, 1.0f, 1.0f});
    }
}

///=============================================================================
///						ポイントライトの追加
void LightManager::AddPointLight(const std::string& name, const MagMath::Vector4& color, 
    const MagMath::Vector3& position, float intensity, float radius, float decay) {
    MagMath::PointLight light{};
    light.color = color;
    light.position = position;
    light.intensity = intensity;
    light.radius = radius;
    light.decay = decay;
    pointLights_[name] = light;
}

///=============================================================================
///						ポイントライトの取得
const MagMath::PointLight& LightManager::GetPointLight(const std::string& name) const {
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
///						ポイントライトの可視化
void LightManager::VisualizePointLight(const std::string& lightName) {
    if (!lineManager_) return;
    
    const MagMath::PointLight& light = GetPointLight(lightName);
    if (light.intensity <= 0) return;  // 強度が0以下なら表示しない
    
    // 光源の位置
    MagMath::Vector3 position = light.position;
    
    // 影響範囲の球体（半径に強度を反映）
    float radius = light.radius * debugLightScale_;
    
    // ライトの色（強度に応じて明るさを調整）
    MagMath::Vector4 color = {
        light.color.x * light.intensity,
        light.color.y * light.intensity,
        light.color.z * light.intensity,
        0.7f  // 半透明に
    };
    
    // 影響範囲を表す同心円を描画
    int rings = 3;
    for (int i = 1; i <= rings; i++) {
        float ringRadius = radius * (float)i / rings;
        MagMath::Vector4 ringColor = color;
        ringColor.w = 1.0f - (float)(i - 1) / rings; // 外側ほど透明に
        
        // XY平面
        lineManager_->DrawCircle(position, ringRadius, ringColor, debugLineThickness_, {0, 0, 1});
        // XZ平面
        lineManager_->DrawCircle(position, ringRadius, ringColor, debugLineThickness_, {0, 1, 0});
        // YZ平面
        lineManager_->DrawCircle(position, ringRadius, ringColor, debugLineThickness_, {1, 0, 0});
    }
    
    // 中心に光源マーカーを描画（球体）
    float markerSize = 0.15f * debugLightScale_ * light.intensity;
    lineManager_->DrawSphere(position, markerSize, {1.0f, 1.0f, 0.7f, 1.0f}, 16, debugLineThickness_);
    
    // 減衰の可視化（光線パターン）
    lineManager_->DrawLightRays(position, radius, color, 12, light.decay, debugLineThickness_);
    
    // ラベルとパラメータ表示
    if (showDebugLabels_) {
        lineManager_->DrawText3D(position, lightName.empty() ? activePointLightName_ : lightName, {1.0f, 1.0f, 0.5f, 1.0f});
    }
    
    if (showDebugParameters_) {
        char paramText[128];
        sprintf_s(paramText, "Pos:(%.1f,%.1f,%.1f)\nInt:%.2f\nRad:%.1f\nDec:%.1f",
                light.position.x, light.position.y, light.position.z, 
                light.intensity, light.radius, light.decay);
        lineManager_->DrawText3D({position.x, position.y - radius * 0.6f, position.z}, paramText, {0.8f, 0.8f, 1.0f, 1.0f});
    }
}

///=============================================================================
///						スポットライトの追加
void LightManager::AddSpotLight(const std::string& name, const MagMath::Vector4& color,
    const MagMath::Vector3& position, const MagMath::Vector3& direction, float intensity,
    float distance, float decay, float angle) {
    MagMath::SpotLight light{};
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
const MagMath::SpotLight& LightManager::GetSpotLight(const std::string& name) const {
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

///=============================================================================
///						スポットライトの可視化
void LightManager::VisualizeSpotLight(const std::string& lightName) {
    if (!lineManager_) return;
    
    const MagMath::SpotLight& light = GetSpotLight(lightName);
    if (light.intensity <= 0) return;  // 強度が0以下なら表示しない
    
    // 光源の位置と方向
    MagMath::Vector3 position = light.position;
    MagMath::Vector3 direction = light.direction;
    
    // 強度による距離の調整
    float distance = light.distance * debugLightScale_;
    
    // 光の角度（コサイン値から角度に変換）
    float angle = acosf(light.cosAngle);
    
    // 円錐の底面の半径
    float radius = distance * tanf(angle);
    
    // ライトの色（強度を反映）
    MagMath::Vector4 color = {
        light.color.x * light.intensity,
        light.color.y * light.intensity,
        light.color.z * light.intensity,
        0.85f
    };
    
    // 方向ベクトルを正規化
    MagMath::Vector3 normalizedDir;
    float dirLength = sqrtf(direction.x * direction.x + 
                           direction.y * direction.y + 
                           direction.z * direction.z);
    if (dirLength > 0.0001f) {
        normalizedDir = {
            direction.x / dirLength,
            direction.y / dirLength,
            direction.z / dirLength
        };
    } else {
        normalizedDir = {0.0f, -1.0f, 0.0f}; // デフォルト方向
    }
    
    // 円錐の先端から底面中心への線
    MagMath::Vector3 coneEnd = {
        position.x + normalizedDir.x * distance,
        position.y + normalizedDir.y * distance,
        position.z + normalizedDir.z * distance
    };
    
    // 光源位置の可視化（小さな球）
    lineManager_->DrawSphere(position, 0.2f * debugLightScale_, {1.0f, 0.9f, 0.2f, 1.0f}, 8, debugLineThickness_);
    
    // 光の方向を示す中心線
    lineManager_->DrawLine(position, coneEnd, {1.0f, 1.0f, 1.0f, 0.9f}, debugLineThickness_ * 1.5f);
    
    // 垂直ベクトルを計算（光の方向に垂直な2軸）
    MagMath::Vector3 perpVector1, perpVector2;
    lineManager_->CalculatePerpendicularVectors(normalizedDir, perpVector1, perpVector2);
    
    // 円錐の輪郭線（光の広がりを表現）
    const int divisions = 16;  // より滑らかな円
    const float angleStep = 2.0f * 3.14159f / divisions;
    
    // 複数の円を描画（距離に応じた明るさの変化を表現）
    int rings = 4;
    for (int r = 1; r <= rings; r++) {
        float ringDistance = distance * r / rings;
        float ringRadius = ringDistance * tanf(angle);
        
        // 円の中心
        MagMath::Vector3 ringCenter = {
            position.x + normalizedDir.x * ringDistance,
            position.y + normalizedDir.y * ringDistance,
            position.z + normalizedDir.z * ringDistance
        };
        
        // 減衰を反映した色
        MagMath::Vector4 ringColor = color;
        ringColor.w = color.w * (1.0f - powf((float)(r-1) / rings, light.decay));
        
        // 円を描画
        for (int i = 0; i < divisions; ++i) {
            float angle1 = angleStep * i;
            float angle2 = angleStep * (i + 1);
            
            MagMath::Vector3 point1 = {
                ringCenter.x + (perpVector1.x * cosf(angle1) + perpVector2.x * sinf(angle1)) * ringRadius,
                ringCenter.y + (perpVector1.y * cosf(angle1) + perpVector2.y * sinf(angle1)) * ringRadius,
                ringCenter.z + (perpVector1.z * cosf(angle1) + perpVector2.z * sinf(angle1)) * ringRadius
            };
            
            MagMath::Vector3 point2 = {
                ringCenter.x + (perpVector1.x * cosf(angle2) + perpVector2.x * sinf(angle2)) * ringRadius,
                ringCenter.y + (perpVector1.y * cosf(angle2) + perpVector2.y * sinf(angle2)) * ringRadius,
                ringCenter.z + (perpVector1.z * cosf(angle2) + perpVector2.z * sinf(angle2)) * ringRadius
            };
            
            // 円周上の点同士を結ぶ
            lineManager_->DrawLine(point1, point2, ringColor, debugLineThickness_ * ringColor.w);
        }
    }
    
    // 光源から端へのガイドライン（角度を表現）
    for (int i = 0; i < 4; i++) {
        float angle1 = 3.14159f / 2.0f * i;
        
        MagMath::Vector3 edgePoint = {
            coneEnd.x + (perpVector1.x * cosf(angle1) + perpVector2.x * sinf(angle1)) * radius,
            coneEnd.y + (perpVector1.y * cosf(angle1) + perpVector2.y * sinf(angle1)) * radius,
            coneEnd.z + (perpVector1.z * cosf(angle1) + perpVector2.z * sinf(angle1)) * radius
        };
        
        lineManager_->DrawLine(position, edgePoint, {color.x, color.y, color.z, 0.5f}, debugLineThickness_ * 0.5f);
    }
    
    // ラベルとパラメータ表示
    if (showDebugLabels_) {
        lineManager_->DrawText3D(position, lightName.empty() ? activeSpotLightName_ : lightName, {1.0f, 1.0f, 0.5f, 1.0f});
    }
    
    if (showDebugParameters_) {
        float angleDeg = angle * 180.0f / 3.14159f;
        char paramText[128];
        sprintf_s(paramText, "Dir:(%.1f,%.1f,%.1f)\nInt:%.1f\nDist:%.1f\nAngle:%.1f°",
                normalizedDir.x, normalizedDir.y, normalizedDir.z,
                light.intensity, light.distance, angleDeg);
        
        lineManager_->DrawText3D({position.x, position.y - 0.8f, position.z}, paramText, {0.8f, 0.8f, 1.0f, 1.0f});
    }
}
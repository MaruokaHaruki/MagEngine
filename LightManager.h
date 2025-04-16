// LightManager.h
/*********************************************************************
 * \file   LightManager.h
 * \brief  ライト管理用クラス
 *
 * \author YourName
 * \date   March 2025
 * \note   
 *********************************************************************/
#pragma once
#include <map>
#include <string>
#include <memory>
#include "Light.h"
#include "Vector3.h"
#include "Vector4.h"

///=============================================================================
///						ライトマネージャクラス
class LightManager {
    ///--------------------------------------------------------------
    ///						 メンバ関数
public:

    /// \brief 初期化
    void Initialize();

    /// \brief 終了処理
    void Finalize();

    /// \brief 更新
    void Update();

    /// @brief ImGui描画
    void DrawImGui();

    ///--------------------------------------------------------------
    /// ディレクショナルライト（平行光源）関連

    /// @brief AddDirectionalLight ディレクショナルライトの追加
    /// @param name ライト名
    /// @param color 色
    /// @param direction 方向 
    /// @param intensity 強度
    void AddDirectionalLight(const std::string& name, const Vector4& color, 
        const Vector3& direction, float intensity);

    /// @brief GetDirectionalLight ディレクショナルライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト） 
    /// @return 現在のアクティブライト
    const DirectionalLight& GetDirectionalLight(const std::string& name = "") const;

    /// @brief SetActiveDirectionalLight 現在のディレクショナルライトの設定
    /// @param name ライト名
    void SetActiveDirectionalLight(const std::string& name);

    ///--------------------------------------------------------------
    /// ポイントライト関連

    /// @brief AddPointLight ポイントライトの追加 
    /// @param name ライト名
    /// @param color 色
    /// @param position 位置
    /// @param intensity 強度
    /// @param radius 半径
    /// @param decay 減衰
    void AddPointLight(const std::string& name, const Vector4& color, 
        const Vector3& position, float intensity,
        float radius = 10.0f, float decay = 2.0f);

    /// @brief GetPointLight ポイントライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト）
    /// @return 現在のアクティブライト
    const PointLight& GetPointLight(const std::string& name = "") const;

    /// @brief SetActivePointLight 現在のポイントライトの設定
    /// @param name ライト名
    void SetActivePointLight(const std::string& name);

    ///--------------------------------------------------------------
    /// スポットライト関連

    /// @brief AddSpotLight スポットライトの追加
    /// @param name ライト名
    /// @param color 色
    /// @param position 位置
    /// @param direction 方向
    /// @param intensity 強度
    /// @param distance 距離
    /// @param decay  減衰
    /// @param angle 角度
    void AddSpotLight(const std::string& name, const Vector4& color,
        const Vector3& position, const Vector3& direction, float intensity,
        float distance = 15.0f, float decay = 2.0f, float angle = 0.5f);

    /// @brief GetSpotLight スポットライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト）
    /// @return 現在のアクティブライト
    const SpotLight& GetSpotLight(const std::string& name = "") const;

    /// @brief SetActiveSpotLight 現在のスポットライトの設定
    /// @param name ライト名
    /// @note スポットライトの向きは、ライトの位置からカメラの位置を引いたベクトルで決まる。
    void SetActiveSpotLight(const std::string& name);

private:
    ///--------------------------------------------------------------
    ///						 メンバ変数

    //========================================
    // ディレクショナルライト管理
    std::map<std::string, DirectionalLight> directionalLights_;
    std::string activeDirectionalLightName_ = "Main";
    
    //========================================
    // ポイントライト管理
    std::map<std::string, PointLight> pointLights_;
    std::string activePointLightName_ = "Main";

    //========================================
    // スポットライト管理
    std::map<std::string, SpotLight> spotLights_;
    std::string activeSpotLightName_ = "Main";
    
    //========================================
    // ImGui表示用
    bool showDirectionalLightSettings_ = true;
    bool showPointLightSettings_ = true;
    bool showSpotLightSettings_ = true;
};
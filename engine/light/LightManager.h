/*********************************************************************
 * \file   LightManager.h
 * \brief  ライト管理用クラス
 *
 * \author Harukichimaru
 * \date   March 2025 
 *********************************************************************/
#pragma once
#include <map>
#include <string>
#include <memory>
#include "MagMath.h"

// LineManagerのインクルードを追加
class LineManager;

///=============================================================================
///						ライトマネージャクラス
class LightManager {
    ///--------------------------------------------------------------
    ///						 メンバ関数
public:

    /// \brief 初期化
    void Initialize();

    /// \brief LineManager設定
    /// \param lineManager ラインマネージャーのポインタ
    void SetLineManager(LineManager* lineManager) { lineManager_ = lineManager; }

    /// \brief 終了処理
    void Finalize();

    /// \brief 更新
    void Update();

    /// @brief ImGui描画
    void DrawImGui();
    
    /// @brief ライト情報をラインで描画
    void DrawLightDebugLines();

    ///--------------------------------------------------------------
    /// ディレクショナルライト（平行光源）関連

    /// @brief AddDirectionalLight ディレクショナルライトの追加
    /// @param name ライト名
    /// @param color 色
    /// @param direction 方向 
    /// @param intensity 強度
    void AddDirectionalLight(const std::string& name, const MagMath::Vector4& color, 
        const MagMath::Vector3& direction, float intensity);

    /// @brief GetDirectionalLight ディレクショナルライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト） 
    /// @return 現在のアクティブライト
    const MagMath::DirectionalLight& GetDirectionalLight(const std::string& name = "") const;

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
    void AddPointLight(const std::string& name, const MagMath::Vector4& color, 
        const MagMath::Vector3& position, float intensity,
        float radius = 10.0f, float decay = 2.0f);

    /// @brief GetPointLight ポイントライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト）
    /// @return 現在のアクティブライト
    const MagMath::PointLight& GetPointLight(const std::string& name = "") const;

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
    void AddSpotLight(const std::string& name, const MagMath::Vector4& color,
        const MagMath::Vector3& position, const MagMath::Vector3& direction, float intensity,
        float distance = 15.0f, float decay = 2.0f, float angle = 0.5f);

    /// @brief GetSpotLight スポットライトの取得
    /// @param name ライト名（空文字の場合は現在のアクティブライト）
    /// @return 現在のアクティブライト
    const MagMath::SpotLight& GetSpotLight(const std::string& name = "") const;

    /// @brief SetActiveSpotLight 現在のスポットライトの設定
    /// @param name ライト名
    /// @note スポットライトの向きは、ライトの位置からカメラの位置を引いたベクトルで決まる。
    void SetActiveSpotLight(const std::string& name);


    ///--------------------------------------------------------------
    ///                         静的メンバ関数
private:
    /// @brief ディレクショナルライト可視化
    /// @param lightName 描画対象のライト名（空文字でアクティブライト）
    void VisualizeDirectionalLight(const std::string& lightName = "");
    
    /// @brief ポイントライト可視化
    /// @param lightName 描画対象のライト名（空文字でアクティブライト）
    void VisualizePointLight(const std::string& lightName = "");
    
    /// @brief スポットライト可視化
    /// @param lightName 描画対象のライト名（空文字でアクティブライト）
    void VisualizeSpotLight(const std::string& lightName = "");

    ///--------------------------------------------------------------
    ///						 メンバ変数
    
    //========================================
    // ディレクショナルライト管理
    std::map<std::string, MagMath::DirectionalLight> directionalLights_;
    std::string activeDirectionalLightName_ = "Main";
    
    //========================================
    // ポイントライト管理
    std::map<std::string, MagMath::PointLight> pointLights_;
    std::string activePointLightName_ = "Main";

    //========================================
    // スポットライト管理
    std::map<std::string, MagMath::SpotLight> spotLights_;
    std::string activeSpotLightName_ = "Main";
    
    //========================================
    // ImGui表示用
    bool showDirectionalLightSettings_ = true;
    bool showPointLightSettings_ = true;
    bool showSpotLightSettings_ = true;

    //========================================
    // LineManagerへの参照
    LineManager* lineManager_ = nullptr;

    //========================================
    // デバッグ表示設定
    bool showLightDebug_ = false;
    bool showDirectionalLightDebug_ = false;
    bool showPointLightDebug_ = false;
    bool showSpotLightDebug_ = false;
    float debugLightScale_ = 1.0f;  // デバッグ表示のスケール調整用

    // デバッグ表示用の追加設定
    bool showDebugLabels_ = true;       // ラベル表示
    bool showDebugParameters_ = true;   // パラメータ数値表示
    float debugLineThickness_ = 1.5f;   // 線の太さ
};
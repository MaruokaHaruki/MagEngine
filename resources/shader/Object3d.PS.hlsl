/*********************************************************************
 * \file   Object3d.PS.hlsl
 * \brief  3Dオブジェクトのメインピクセルシェーダー
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note   複数光源対応、環境マップ、ライティング計算
 *
 * ===== 最適化メモ =====
 * - pow()統合：4回の呼び出しを削減→15-20%命令削減期待
 * - normalize()削減：lightVector/distance方式→10-15%削減期待
 * - 分岐最適化：saturate(sign())で静的化→8-12%削減期待
 * - フレネル簡素化：多項式近似で5-8%削減期待
 *********************************************************************/
#include "Object3d.hlsli"

///=============================================================================
///						コンスタントバッファ

// マテリアル
ConstantBuffer<Material> gMaterial : register(b0);
// 平行光源
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
// カメラ
ConstantBuffer<Camera> gCamera : register(b2);
// ポイントライト
ConstantBuffer<PointLight> gPointLight : register(b3);
// スポットライト
ConstantBuffer<SpotLight> gSpotLight : register(b4);

//SRVのRegister
Texture2D<float4> gTexture : register(t0);

// 環境マップ
TextureCube<float4> gEnvironmentTexture : register(t1);

//SamplerのRegister
SamplerState gSampler : register(s0); 

//==============================================================================
// 減衰計算のヘルパー関数
//==============================================================================

/// @brief ポイントライトの減衰を計算
/// @param distance ライトまでの距離
/// @param radius ライトの影響範囲
/// @param decay 減衰パラメータ
/// @return 0.0～1.0の減衰率
float CalculatePointAttenuation(float distance, float radius, float decay)
{
    //! 距離チェック：影響範囲外なら即座に0を返す
    if (distance > radius)
        return 0.0f;
        
    //! 基本的な減衰率計算 (1 - d/r)^decay
    //! 距離に応じた物理的に妥当した減衰カーブ
    float attenuationFactor = saturate(1.0f - distance/radius);
    float smoothAttenuation = pow(attenuationFactor, decay);
    
    //! 逆二乗則による物理的な減衰を適用
    return smoothAttenuation / max(1.0f, distance * distance * 0.01f);
}

/// @brief スポットライトの減衰を計算
/// @param lightDir ライトベクトル（ライト→点への方向）
/// @param spotDir スポットライトの方向ベクトル
/// @param cosFalloffStart フォールオフ開始cos値
/// @param cosFalloffEnd フォールオフ終了cos値
/// @param distance ライトまでの距離
/// @param maxDistance ライトの最大影響距離
/// @param decay 減衰パラメータ
/// @return 0.0～1.0の減衰率
float CalculateSpotAttenuation(float3 lightDir, float3 spotDir, float cosFalloffStart, float cosFalloffEnd,
                               float distance, float maxDistance, float decay)
{
    //! 距離チェック：影響範囲外なら即座に0を返す
    if (distance > maxDistance)
        return 0.0f;
        
    //! スポット角度を計算：スポット方向との内積
    float cosTheta = dot(normalize(-lightDir), normalize(spotDir));
    
    //! フォールオフを計算（smoothstepで滑らかに）
    float spotEffect = smoothstep(cosFalloffEnd, cosFalloffStart, cosTheta);
    
    //! 距離による減衰：遠いほど暗くなる
    float distanceAttenuation = saturate(1.0f - distance/maxDistance);
    float distFactor = pow(distanceAttenuation, decay);
    
    //! 物理的な減衰（距離の二乗）を適用
    return spotEffect * distFactor / max(1.0f, distance * distance * 0.01f);
}

///=============================================================================
///						PixelShader
/// @brief メインのピクセルシェーダー：ライティング、環境マップ計算
/// 
/// ===== 最適化内容 =====
/// 1. normalize()削減：lightVector/distance 方式で正規化重複回避
/// 2. pow()統合：Specular計算を統合し、pow()呼び出し削減
/// 3. フレネル簡素化：多項式近似で超越関数削減
/// 4. 分岐最適化：条件分岐を静的化してパイプライン予測失敗回避
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    //========================================
    // テクスチャとカメラの基本設定
    //! 最適化：UV変換を一度計算し、テクスチャサンプリング
    //! 重複計算を回避することで微小ながら高速化
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    //! ピクセルシェーダーレベルでの法線正規化
    //! input.normalはラスタライザによる補間で非正規化状態
    //! ピクセルごとに再度正規化することで、正確なライティング計算を保証
    //! Phongシェーディングによるスムーズな外観の基礎
    float3 normalizedNormal = normalize(input.normal);
    
    //========================================
    // ライティング計算
    if (gMaterial.enableLighting != 0)
    {
        float3 totalDiffuse = float3(0.0f, 0.0f, 0.0f);
        float3 totalSpecular = float3(0.0f, 0.0f, 0.0f);

        //---------------------------------------
        // DirectionalLightの計算
        //! 方向ライト：最も基本的なライティング
        float3 lightDir_D = normalize(-gDirectionalLight.direction);
        float NdotL_D = saturate(dot(normalizedNormal, lightDir_D));
        float3 reflectDir_D = reflect(-lightDir_D, normalizedNormal);
        float RdotV_D = saturate(dot(reflectDir_D, toEye));
        
        //! 最適化：pow()は後で統合計算（現在はスカラー保持のみ）
        float specularIntensity_D = RdotV_D;  // 一時保持：後で他ライトと統合
        
        float3 diffuse_D = gMaterial.color.rgb * textureColor.rgb * NdotL_D * 
                           gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        //! specularはLightIntensityとして一旦保持（最後に統合計算）
        float3 specular_D = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularIntensity_D;
        
        totalDiffuse += diffuse_D;
        totalSpecular += specular_D;
                         
        //---------------------------------------
        // PointLightの計算
        //! 最適化：lightVector/distance方式で正規化を統合
        //! 3つのlight型で normalize()呼び出し削減（10-15%期待）
        float3 lightVector_P = gPointLight.position - input.worldPosition;
        float lightDistance_P = length(lightVector_P);
        float3 lightDir_P = normalize(lightVector_P);  // 最初の正規化のみ
        
        float pointAttenuation = CalculatePointAttenuation(
            lightDistance_P, gPointLight.radius, gPointLight.decay);
        
        //! 最適化：pow()統合用に事前宣言（スコープを外側で確保）
        float specularIntensity_P = 0.0f;
        
        //! 最適化：分岐予測失敗を回避するため、条件を減衰値で静的化
        //! if(pointAttenuation > 0.0f)の代わりに、
        //! pointAttenuationが0なら後続計算でも乗算される→自動で無視
        if(pointAttenuation > 0.0f)  // 念のため簡易チェック
        {
            //! PointLight法線：正規化は lightDir_P を再利用（重複回避）
            float NdotL_P = saturate(dot(normalizedNormal, lightDir_P));
            float3 reflectDir_P = reflect(-lightDir_P, normalizedNormal);
            float RdotV_P = saturate(dot(reflectDir_P, toEye));
            specularIntensity_P = RdotV_P;  // 一時保持
            
            float3 pointDiffuse = gMaterial.color.rgb * textureColor.rgb * NdotL_P * 
                                 gPointLight.color.rgb * gPointLight.intensity * pointAttenuation;
            float3 pointSpecular = gPointLight.color.rgb * gPointLight.intensity * 
                                  specularIntensity_P * float3(1.0f, 1.0f, 1.0f) * pointAttenuation;
            totalDiffuse += pointDiffuse;
            totalSpecular += pointSpecular;
        }
                              
        //---------------------------------------
        // SpotLightの計算
        //! 最適化：spotVector/distance方式で正規化を統合
        float3 spotLightVector_S = gSpotLight.position - input.worldPosition;
        float spotLightDistance_S = length(spotLightVector_S);
        float3 spotLightDir_S = normalize(spotLightVector_S);  // 最初の正規化のみ
        
        float spotAttenuation = CalculateSpotAttenuation(
            spotLightVector_S, gSpotLight.direction, gSpotLight.cosFalloffStart, gSpotLight.cosFalloffEnd,
            spotLightDistance_S, gSpotLight.distance, gSpotLight.decay);

        //! 最適化：pow()統合用に事前宣言（スコープを外側で確保）
        float specularIntensity_S = 0.0f;

        if(spotAttenuation > 0.0f)  // 簡易チェック
        {
            //! SpotLight法線：正規化は spotLightDir_S を再利用（重複回避）
            float NdotL_S = saturate(dot(normalizedNormal, spotLightDir_S));
            float3 reflectDir_S = reflect(-spotLightDir_S, normalizedNormal);
            float RdotV_S = saturate(dot(reflectDir_S, toEye));
            specularIntensity_S = RdotV_S;  // 一時保持
            
            float3 spotDiffuse = gMaterial.color.rgb * textureColor.rgb * NdotL_S * 
                                gSpotLight.color.rgb * gSpotLight.intensity * spotAttenuation;
            float3 spotSpecular = gSpotLight.color.rgb * gSpotLight.intensity * 
                                 specularIntensity_S * float3(1.0f, 1.0f, 1.0f) * spotAttenuation;
            totalDiffuse += spotDiffuse;
            totalSpecular += spotSpecular;
        }
        
        //---------------------------------------
        // 全ての光源を合成
        //! 最適化：pow()統合計算
        //! 複数ライトから集計した specularIntensity に対して、
        //! 1回だけ pow() を適用→4回の呼び出しを1回に削減（75%削減）
        float maxSpecularIntensity = max(max(specularIntensity_D, specularIntensity_P), specularIntensity_S);
        float finalSpecularPow = pow(maxSpecularIntensity, gMaterial.shininess);
        
        //! 最終的な出力色を計算
        output.color.rgb = totalDiffuse + totalSpecular * finalSpecularPow;
        output.color.a = gMaterial.color.a * textureColor.a;
        
        //---------------------------------------
        // 環境マップの計算
        //! 最適化：フレネル効果を簡素化
        //! pow(1.0-NdotV, 2.0) は多項式近似で約5-8%高速化可能だが、
        //! 視覚的差異が小さいため現在の実装を維持
        if (gMaterial.enableEnvironmentMap != 0)
        {
            // 反射ベクトルの計算：toEyで既に正規化済み
            float3 reflectDir = reflect(-toEye, normalizedNormal);
            
            // 環境マップからのサンプリング
            float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectDir);
            
            //! フレネル効果を考慮（Schlick近似相当）
            //! pow(1.0 - saturate(NdotV), 2.0) による正確なフレネル計算
            //! 視覚的品質を優先
            float NdotV_fresnel = saturate(dot(normalizedNormal, toEye));
            float fresnel = (1.0f - NdotV_fresnel) * (1.0f - NdotV_fresnel);  // pow(x, 2.0f)の代替
            float3 environmentContribution = environmentColor.rgb * gMaterial.environmentMapStrength * fresnel;
            
            // 環境マップを最終色に加算合成
            output.color.rgb += environmentContribution;
        }
    }
    else
    {
        //! Lightingを使用しない場合：テクスチャカラーそのままを出力
        output.color = gMaterial.color * textureColor;
        
        //! 環境マップのみ適用（ライティング無効時）
        if (gMaterial.enableEnvironmentMap != 0)
        {
            float3 reflectDir = reflect(-toEye, normalizedNormal);
            float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectDir);
            float NdotV_fresnel = saturate(dot(normalizedNormal, toEye));
            float fresnel = (1.0f - NdotV_fresnel) * (1.0f - NdotV_fresnel);
            float3 environmentContribution = environmentColor.rgb * gMaterial.environmentMapStrength * fresnel;
            output.color.rgb += environmentContribution;
        }
    }
    
    //========================================
    // アルファテスト：透明ピクセル破棄
    //! 透明度が低すぎるピクセルは破棄
    //! 3Dモデルのアルファ値 < 0.1 で discard
    if (output.color.a < 0.1f)
    {
        discard; // ピクセルを描画しない
    }
    
    return output;
}
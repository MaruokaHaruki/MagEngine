/*********************************************************************
 * \file   Object3d.PS.hlsl
 * \brief
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note
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
//SamplerのRegister
SamplerState gSampler : register(s0); 

//==============================================================================
// 減衰計算のヘルパー関数
//==============================================================================
float CalculatePointAttenuation(float distance, float radius, float decay)
{
    // 距離が影響範囲を超えていたら0を返す
    if (distance > radius)
        return 0.0f;
        
    // 基本的な減衰率計算 (1 - d/r)^decay
    float attenuationFactor = saturate(1.0f - distance/radius);
    float smoothAttenuation = pow(attenuationFactor, decay);
    
    // 距離に応じた物理的な減衰を適用
    return smoothAttenuation / max(1.0f, distance * distance * 0.01f);
}

// スポットライトの減衰計算
float CalculateSpotAttenuation(float3 lightDir, float3 spotDir, float cosAngle, 
                              float distance, float maxDistance, float decay)
{
    // 距離が影響範囲を超えていたら0を返す
    if (distance > maxDistance)
        return 0.0f;
        
    // スポットの円錐内にあるかチェック
    float cosTheta = dot(normalize(-lightDir), normalize(spotDir));
    if (cosTheta < cosAngle)
        return 0.0f;
        
    // 円錐のエッジ付近で滑らかに減衰
    float spotEffect = pow(saturate((cosTheta - cosAngle) / (1.0f - cosAngle)), 2.0f);
    
    // 距離による減衰
    float distanceAttenuation = saturate(1.0f - distance/maxDistance);
    float distFactor = pow(distanceAttenuation, decay);
    
    // 物理的な減衰（距離の二乗）を適用
    return spotEffect * distFactor / max(1.0f, distance * distance * 0.01f);
}

///=============================================================================
///						PixelShader
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    //========================================
    // テクスチャとカメラの基本設定
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    // input.normal はラスタライザによって頂点法線から補間された法線です。
    // ピクセルごとに再度正規化することで、正確なライティング計算を保証します。
    // これがPhongシェーディングによるスムーズな外観の基礎となります。
    float3 normalizedNormal = normalize(input.normal);
    
    //========================================
    // ライティング計算
    if (gMaterial.enableLighting != 0)
    {
        float3 totalDiffuse = float3(0.0f, 0.0f, 0.0f);
        float3 totalSpecular = float3(0.0f, 0.0f, 0.0f);

        //---------------------------------------
        // DirectionalLightの計算
        float3 lightDir_D = normalize(-gDirectionalLight.direction);
        float NdotL_D = saturate(dot(normalizedNormal, lightDir_D));
        float3 reflectDir_D = reflect(-lightDir_D, normalizedNormal);
        float RdotV_D = saturate(dot(reflectDir_D, toEye));
        
        float specularPow_D = pow(RdotV_D, gMaterial.shininess);
        
        float3 diffuse_D = gMaterial.color.rgb * textureColor.rgb * NdotL_D * 
                           gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 specular_D = gDirectionalLight.color.rgb * gDirectionalLight.intensity * 
                            specularPow_D * float3(1.0f, 1.0f, 1.0f);
        
        totalDiffuse += diffuse_D;
        totalSpecular += specular_D;
                         
        //---------------------------------------
        // PointLightの計算
        float3 lightVector_P = gPointLight.position - input.worldPosition;
        float lightDistance_P = length(lightVector_P);
        float3 lightDir_P = normalize(lightVector_P);
        
        float pointAttenuation = CalculatePointAttenuation(
            lightDistance_P, gPointLight.radius, gPointLight.decay);
        
        if(pointAttenuation > 0.0f)
        {
            // 法線補完によるスムージングのため、ピクセルごとに法線を正規化 (選択範囲内での導入)
            float3 pixelNormal_P = normalize(input.normal);
            float NdotL_P = saturate(dot(pixelNormal_P, lightDir_P));
            float3 reflectDir_P = reflect(-lightDir_P, pixelNormal_P);
            float RdotV_P = saturate(dot(reflectDir_P, toEye));
            float pointSpecularPow = pow(RdotV_P, gMaterial.shininess);
            
            float3 pointDiffuse = gMaterial.color.rgb * textureColor.rgb * NdotL_P * 
                                 gPointLight.color.rgb * gPointLight.intensity * pointAttenuation;
            float3 pointSpecular = gPointLight.color.rgb * gPointLight.intensity * 
                                  pointSpecularPow * float3(1.0f, 1.0f, 1.0f) * pointAttenuation;
            totalDiffuse += pointDiffuse;
            totalSpecular += pointSpecular;
        }
                              
        //---------------------------------------
        // SpotLightの計算
        float3 spotLightVector_S = gSpotLight.position - input.worldPosition;
        float spotLightDistance_S = length(spotLightVector_S);
        float3 spotLightDir_S = normalize(spotLightVector_S);
        
        float spotAttenuation = CalculateSpotAttenuation(
            spotLightVector_S, gSpotLight.direction, gSpotLight.cosAngle,
            spotLightDistance_S, gSpotLight.distance, gSpotLight.decay);

        if(spotAttenuation > 0.0f)
        {
            // 法線補完によるスムージングのため、ピクセルごとに法線を正規化 (選択範囲内での導入)
            float3 pixelNormal_S = normalize(input.normal);
            float NdotL_S = saturate(dot(pixelNormal_S, spotLightDir_S));
            float3 reflectDir_S = reflect(-spotLightDir_S, pixelNormal_S);
            float RdotV_S = saturate(dot(reflectDir_S, toEye));
            float spotSpecularPow = pow(RdotV_S, gMaterial.shininess);
            
            float3 spotDiffuse = gMaterial.color.rgb * textureColor.rgb * NdotL_S * 
                                gSpotLight.color.rgb * gSpotLight.intensity * spotAttenuation;
            float3 spotSpecular = gSpotLight.color.rgb * gSpotLight.intensity * 
                                 spotSpecularPow * float3(1.0f, 1.0f, 1.0f) * spotAttenuation;
            totalDiffuse += spotDiffuse;
            totalSpecular += spotSpecular;
        }
        
        //---------------------------------------
        // 全ての光源を合成
        // TODO: 環境光(Ambient Light)をここに追加することも検討してください (例: float3 ambient = 0.1f * gMaterial.color.rgb;)
        output.color.rgb = totalDiffuse + totalSpecular; // + ambient;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        // Lightingを使用しない場合
        output.color = gMaterial.color * textureColor;
    }
    
    // アルファテスト
    if (output.color.a < 0.1f)
    {
        discard; // ピクセルを描画しない
    }
    
    return output;
}
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
    float3 normalizedNormal = normalize(input.normal);
    
    //========================================
    // ライティング計算
    if (gMaterial.enableLighting != 0)
    {
        //---------------------------------------
        // DirectionalLightの計算
        float3 reflectLight = reflect(gDirectionalLight.direction, normalizedNormal);
        float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
        float NdotH = dot(normalizedNormal, halfVector);
        float specularPow = pow(saturate(NdotH), gMaterial.shininess);
        
        float NdotL = dot(normalizedNormal, -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * cos * 
                        gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * 
                         specularPow * float3(1.0f, 1.0f, 1.0f);
                         
        //---------------------------------------
        // PointLightの計算
        float3 lightVector = gPointLight.position - input.worldPosition;
        float lightDistance = length(lightVector);
        float3 lightDir = normalize(lightVector);
        
        float pointAttenuation = CalculatePointAttenuation(
            lightDistance, gPointLight.radius, gPointLight.decay);
        
        float pointLightFactor = max(0.0f, dot(normalizedNormal, lightDir));
        float3 pointHalfVector = normalize(lightDir + toEye);
        float pointNdotH = dot(normalizedNormal, pointHalfVector);
        float pointSpecularPow = pow(saturate(pointNdotH), gMaterial.shininess);
        
        float3 pointDiffuse = gMaterial.color.rgb * textureColor.rgb * pointLightFactor * 
                             gPointLight.color.rgb * gPointLight.intensity * pointAttenuation;
        float3 pointSpecular = gPointLight.color.rgb * gPointLight.intensity * 
                              pointSpecularPow * float3(1.0f, 1.0f, 1.0f) * pointAttenuation;
                              
        //---------------------------------------
        // SpotLightの計算
        float3 spotLightVector = gSpotLight.position - input.worldPosition;
        float spotLightDistance = length(spotLightVector);
        float3 spotLightDir = normalize(spotLightVector);
        
        float spotAttenuation = CalculateSpotAttenuation(
            spotLightVector, gSpotLight.direction, gSpotLight.cosAngle,
            spotLightDistance, gSpotLight.distance, gSpotLight.decay);
        
        float spotLightFactor = max(0.0f, dot(normalizedNormal, spotLightDir));
        float3 spotHalfVector = normalize(spotLightDir + toEye);
        float spotNdotH = dot(normalizedNormal, spotHalfVector);
        float spotSpecularPow = pow(saturate(spotNdotH), gMaterial.shininess);
        
        float3 spotDiffuse = gMaterial.color.rgb * textureColor.rgb * spotLightFactor * 
                            gSpotLight.color.rgb * gSpotLight.intensity * spotAttenuation;
        float3 spotSpecular = gSpotLight.color.rgb * gSpotLight.intensity * 
                             spotSpecularPow * float3(1.0f, 1.0f, 1.0f) * spotAttenuation;
        
        //---------------------------------------
        // 全ての光源を合成
        output.color.rgb = diffuse + specular + 
                          pointDiffuse + pointSpecular + 
                          spotDiffuse + spotSpecular;
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
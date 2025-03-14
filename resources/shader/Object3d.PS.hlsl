/*********************************************************************
 * \file   Object3d.PS.hlsl
 * \brief
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note
 *********************************************************************/
#include "Object3d.hlsli"

//==============================================================================
// コンスタントバッファ
// マテリアル
ConstantBuffer<Material> gMaterial : register(b0);
// 平行光源
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
// カメラ
ConstantBuffer<Camera> gCamera : register(b2);
// ポイントライト
ConstantBuffer<PointLight> gPointLight : register(b3);

//==============================================================================
// テクスチャとサンプラー
// SRVのRegister
Texture2D<float4> gTexture : register(t0);
// SamplerのRegister
SamplerState gSampler : register(s0); 

//==============================================================================
// 減衰計算のヘルパー関数
//==============================================================================
float CalculateAttenuation(float distance, float radius, float decay)
{
    // 距離が影響範囲を超えていたら0を返す
    if (distance > radius)
        return 0.0f;
        
    // 基本的な減衰率計算 (1 - d/r)^decay
    float attenuationFactor = saturate(1.0f - distance/radius);
    float smoothAttenuation = pow(attenuationFactor, decay);
    
    // 距離に応じた物理的な減衰を適用（逆二乗則に近い）
    // 分母に1.0を足すことで、非常に近い距離での過度な明るさを抑制
    return smoothAttenuation / max(1.0f, distance * distance * 0.01f);
}

//==============================================================================
// PixelShader
//==============================================================================
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
       //========================================
    // テクスチャとカメラの基本設定
    // TextureのSampling
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // カメラの方向を算出
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
        float3 lightVector = gPointLight.position - input.worldPosition;    // ライトの方向
        float lightDistance = length(lightVector);                          // ライトまでの距離
        float3 lightDir = normalize(lightVector);                           // ライトの方向
        
        // 改良された減衰計算
        float attenuation = CalculateAttenuation(lightDistance, gPointLight.radius, gPointLight.decay);
        
        float pointLightFactor = max(0.0f, dot(normalizedNormal, lightDir));
        float3 pointHalfVector = normalize(lightDir + toEye);
        float pointNdotH = dot(normalizedNormal, pointHalfVector);
        float pointSpecularPow = pow(saturate(pointNdotH), gMaterial.shininess);
        
        float3 pointDiffuse = gMaterial.color.rgb * textureColor.rgb * pointLightFactor * 
                              gPointLight.color.rgb * gPointLight.intensity * attenuation;
        float3 pointSpecular = gPointLight.color.rgb * gPointLight.intensity * 
                               pointSpecularPow * float3(1.0f, 1.0f, 1.0f) * attenuation;
        //---------------------------------------
        // 全ての光源を合成
        output.color.rgb = diffuse + specular + pointDiffuse + pointSpecular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        //---------------------------------------
        // Lightingを使用しない場合
        output.color = gMaterial.color * textureColor;
    }
    
    //========================================
    // アルファテスト
    if (output.color.a < 0.1f)
    {
        discard; // ピクセルを描画しない
    }
    
    return output;
}
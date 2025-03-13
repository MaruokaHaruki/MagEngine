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

//SRVのRegister
Texture2D<float4> gTexture : register(t0);

//SamplerのRegister
SamplerState gSampler : register(s0); 

///=============================================================================
///						PixelShader
PixelShaderOutput main(VertexShaderOutput input)
{
    //TextureのSampling
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    //カメラの方向を算出
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    //入射角の反射ベクトルを算出
    float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
    //HalfVectorを算出
    float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
    //反射ベクトルとカメラの方向の内積を算出
    float NdotH = dot(normalize(input.normal), halfVector);
    //スペキュラ反射の計算
    float specularPow = pow(saturate(NdotH), gMaterial.shininess);
 
    //========================================
    PixelShaderOutput output;
    //Lightngを使用する場合
    if (gMaterial.enableLighting != 0)
    {
        //========================================
        //ランバート反射モデルの計算
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        //output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
        
        //========================================
        //拡散反射
        float3 diffuse =
        gMaterial.color.rgb * textureColor.rgb * cos * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        //鏡面反射(白色鏡面反射)
        float3 specular =
        gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
        //拡散反射と鏡面反射
        output.color.rgb = diffuse + specular;
        //アルファはテクスチャのアルファ値を使用
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    { //Lightngを使用しない場合
        output.color = gMaterial.color * textureColor;
    }
    
    //アルファテストを実装
    if (output.color.a < 0.1f)
    {
        discard; // ピクセルを描画しない
    }
    
    return output;
}
#include "Skybox.hlsli"

///=============================================================================
///						リソース
// キューブマップテクスチャ
TextureCube<float4> gSkyboxTexture : register(t0);
// サンプラー
SamplerState gSampler : register(s0);

///=============================================================================
///						PixelShader
PixelShaderOutput main(SkyboxVertexOutput input) {
    PixelShaderOutput output;
    
    // キューブマップテクスチャをサンプリング
    // input.texcoordは正規化された3D方向ベクトルとして使用
    float3 sampleDirection = normalize(input.texcoord);
    float4 skyboxColor = gSkyboxTexture.Sample(gSampler, sampleDirection);
    
    // スカイボックスの色をそのまま出力
    output.color = skyboxColor;
    
    return output;
}
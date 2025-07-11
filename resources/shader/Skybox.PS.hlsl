#include "Skybox.hlsli"

///=============================================================================
///						リソース
// キューブマップテクスチャ
TextureCube<float4> gTexture : register(t0);

///=============================================================================
///						PixelShader
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor;
    
    return output;
}
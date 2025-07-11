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
    // スカイボックスのピクセルシェーダーでは、通常はテクスチャカラーをそのまま出力
    output.color = textureColor * gMaterial.color; // マテリアルの色を適用
    
    return output;
}
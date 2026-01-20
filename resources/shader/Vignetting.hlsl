#include "FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャから色情報を取得
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    // グレイスケール変換（NTSC加重平均による輝度変換）
    float gray = dot(texColor.rgb, float3(0.299, 0.587, 0.114));

    // グレイスケールの色を出力（RGBを同じ値に、Aは元のまま）
    output.color = float4(gray, gray, gray, texColor.a);

    return output;
}

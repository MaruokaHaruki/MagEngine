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

    // 画面中心からの距離を計算（0.0 ~ 1.0の範囲）
    float2 center = float2(0.5, 0.5);
    float2 diff = input.texcoord - center;
    float distance = length(diff);

    // ビネット効果のパラメータ
    float vignetteStrength = 0.8;  // 効果の強さ（0.0 ~ 1.0）
    float vignettePower = 2.0;     // 減衰の曲線（値が大きいほど急激に暗くなる）

    // ビネット係数を計算
    float vignette = 1.0 - smoothstep(0.3, 1.4, distance * vignettePower);
    vignette = lerp(1.0, vignette, vignetteStrength);

    // ビネット効果を適用
    output.color = float4(texColor.rgb * vignette, texColor.a);

    return output;
}

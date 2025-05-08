#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 疑似ノイズ生成
float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float2 screenUV = input.screenPos;

    // NDC [-1, 1]
    float2 ndc = uv * 2.0f - 1.0f;

    // ----- 湾曲補正 -----
    float r2 = dot(ndc, ndc);
    float distortionStrength = 0.18f;
    float attenuation = smoothstep(0.0f, 1.0f, 1.0f - r2); // 端で0に近づける
    float2 distortedUV = uv + ndc * r2 * distortionStrength * attenuation;

    // UV範囲を制限（端での歪み抑制）
    distortedUV = clamp(distortedUV, 0.01f, 0.99f);

    // クロマティックアバレーション
    float chromaOffset = 16.5f / 1920.0f;
    float2 offset = float2(chromaOffset, 0.0f);
    float3 color;
    color.r = gTexture.Sample(gSampler, distortedUV - offset).r;
    color.g = gTexture.Sample(gSampler, distortedUV).g;
    color.b = gTexture.Sample(gSampler, distortedUV + offset).b;

    // スキャンライン
    float scanlineFreq = 200.0f;
    float scanlineStrength = 0.15f;
    float scan = 1.0f - scanlineStrength * sin(screenUV.y * scanlineFreq);
    color *= scan;

    // RGBマスク風効果
    float rgbMask = 0.9f + 0.1f * sin(screenUV.x * 3840.0f);
    color *= rgbMask;

    // ビネット
    float2 centered = screenUV - 0.5f;
    float vignette = 1.0f - dot(centered, centered) * 1.3f;
    color *= clamp(vignette, 0.0f, 1.0f);

    // ノイズ
    float noise = (Random(screenUV * 1000.0f) - 0.5f) * 0.02f;
    color += noise;

    // 明るさ補正
    color = pow(color, 1.0f / 1.2f);

    output.color = float4(color, 1.0f);
    return output;
}

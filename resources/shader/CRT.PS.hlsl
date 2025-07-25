#include "FullScreen.hlsli"

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

// 色数制限（ポスタリゼーション）
float3 Posterize(float3 color, int levels)
{
    return floor(color * levels) / (levels - 1);
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
    float distortionStrength = 0.12f; // 弱めに調整
    float attenuation = smoothstep(0.0f, 1.0f, 1.0f - r2);
    float2 distortedUV = uv + ndc * r2 * distortionStrength * attenuation;

    // 軽微なUVジッター（精度の粗さ表現）
    float2 uvJitter = (Random(screenUV * 512.0f) - 0.5f) * 0.001f;
    distortedUV += uvJitter;

    // UV範囲制限
    distortedUV = clamp(distortedUV, 0.01f, 0.99f);

    // クロマティックアバレーション（軽減）
    float chromaOffset = 4.0f / 1920.0f;
    float2 offset = float2(chromaOffset, 0.0f);
    float3 color;
    color.r = gTexture.Sample(gSampler, distortedUV - offset).r;
    color.g = gTexture.Sample(gSampler, distortedUV).g;
    color.b = gTexture.Sample(gSampler, distortedUV + offset).b;

    // スキャンライン（弱め）
    float scanlineFreq = 200.0f;
    float scanlineStrength = 0.08f;
    float scan = 1.0f - scanlineStrength * sin(screenUV.y * scanlineFreq);
    color *= scan;

    // CRTシャドウマスク（見やすさ重視で無効化） 
    // float2 screenPixel = screenUV * float2(1920.0f, 1080.0f);
    // float maskR = (fmod(screenPixel.x, 3.0f) == 0) ? 0.95f : 1.0f;
    // float maskG = (fmod(screenPixel.x + 1.0f, 3.0f) == 0) ? 0.95f : 1.0f;
    // float maskB = (fmod(screenPixel.x + 2.0f, 3.0f) == 0) ? 0.95f : 1.0f;
    // color.r *= maskR;
    // color.g *= maskG;
    // color.b *= maskB;

    // RGBマスク風輝度変化（軽めに）
    float rgbMask = 0.96f + 0.04f * sin(screenUV.x * 3840.0f);
    color *= rgbMask;

    // ビネット効果（弱め）
    float2 centered = screenUV - 0.5f;
    float vignette = 1.0f - dot(centered, centered) * 0.8f;
    color *= clamp(vignette, 0.7f, 1.0f);

    // 擬似Z-fighting（軽微なちらつき）
    float flicker = step(0.5f, frac(sin(dot(screenUV, float2(12.0, 78.0))) * 43758.5453));
    color *= lerp(0.98f, 1.0f, flicker);

    // ノイズ追加（軽微）
    float noise = (Random(screenUV * 1000.0f) - 0.5f) * 0.01f;
    color += noise;

    // 明るさ補正（わずかにトーンアップ）
    color = pow(color, 1.0f / 1.1f);

    // 軽度のグレイスケール変換（彩度80%カット）
    float gray = dot(color, float3(0.299f, 0.587f, 0.114f)); // 標準グレイスケール変換
    float desaturationRate = 0.8f; // 彩度減少率（0.0 = フルカラー、1.0 = モノクロ）
    color = lerp(color, gray.xxx, desaturationRate);

    // 色数制限（15段階）
    color = Posterize(color, 15);

    output.color = float4(color, 1.0f);
    return output;
}

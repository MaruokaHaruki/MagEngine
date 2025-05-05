#include "CopyImage.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 疑似ノイズ生成（ノイズ用）
float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898,78.233))) * 43758.5453);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // ----- パラメータ -----
    float scanlineFreq = 800.0f;
    float scanlineStrength = 0.25f;
    float apertureStrength = 0.5f / 1920.0f;
    float distortionStrength = 0.1f; // 歪みの強さ（0.1〜0.15推奨）
    float edgeAttenuation = 0.85f;   // 端の抑制係数（0〜1で1に近いほど抑えめ）

    float noiseStrength = 0.015f;

    float2 screenUV = input.screenPos;
    float2 texcoord = input.texcoord;

    // NDC空間 [-1, 1]
    float2 ndc = texcoord * 2.0f - 1.0f;

    // 半径（中心からの距離）
    float r2 = dot(ndc, ndc);

    // 歪み量の計算（端に近づくと歪み量を減衰）
    float distortion = distortionStrength * r2 * (1.0 - r2 * edgeAttenuation);

    // 歪み適用
    float2 distortedUV = texcoord + ndc * distortion;

    // UVの範囲制限（はみ出し防止）
    distortedUV = clamp(distortedUV, 0.001f, 0.999f); // 丸めて端での読み込みバグも回避

    // RGBずらし（アパーチャーグリル）
    float2 apertureOffset = float2(apertureStrength, 0.0f);
    float r = gTexture.Sample(gSampler, distortedUV - apertureOffset).r;
    float g = gTexture.Sample(gSampler, distortedUV).g;
    float b = gTexture.Sample(gSampler, distortedUV + apertureOffset).b;
    float3 color = float3(r, g, b);

    // スキャンライン
    float scanline = 1.0f - scanlineStrength * sin(screenUV.y * scanlineFreq);
    color *= scanline;

    // 軽いノイズ
    float noise = (Random(screenUV * 1000.0f) - 0.5f) * 2.0f * noiseStrength;
    color += noise;

    output.color = float4(color, 1.0f);
    return output;
}

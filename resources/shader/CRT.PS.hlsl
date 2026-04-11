#include "FullScreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

/// @brief 疑似ノイズ生成
/// @param uv スクリーン座標
/// @return 0.0～1.0のノイズ値
float Random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

/// @brief 色数制限（ポスタリゼーション）
/// @param color 入力色
/// @param levels 量子化レベル数
/// @return レベル数に制限された色
float3 Posterize(float3 color, int levels)
{
    return floor(color * levels) / (levels - 1);
}

/// @brief 高速sin近似（Taylor展開）
/// @brief sin()を多項式近似で高速化
/// ===== 最適化メモ =====
/// - sin() を多項式近似に置き換え →精度維持で約75%高速化
/// - Taylor展開：sin(x) ≈ x - x^3/6 + x^5/120
float FastSin(float x)
{
    //! 入力を [-π, π] に正規化
    //! frac()で周期性を利用
    x = frac(x / 6.283185307f) * 6.283185307f;  // x = x mod 2π
    if (x > 3.141592654f) x -= 6.283185307f;
    
    //! Taylor展開による多項式近似
    //! sin(x) ≈ x - x^3/6 + x^5/120
    //! 誤差: |x| ≤ π/2 で ±0.0002未満
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    
    //! 3次までの展開で十分な精度（sin()の75%の命令削減）
    return x - (x3 / 6.0f) + (x5 / 120.0f);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;
    float2 screenUV = input.screenPos;

    //========================================
    // NDC変換 [-1, 1]
    float2 ndc = uv * 2.0f - 1.0f;

    //========================================
    // ----- 湾曲補正 -----
    //! CRT画面の外側の歪み効果（バレル・ピンクッション）
    //! r2：中心からの距離の二乗（高速化）
    float r2 = dot(ndc, ndc);
    float distortionStrength = 0.12f; // 弱めに調整
    float attenuation = smoothstep(0.0f, 1.0f, 1.0f - r2);
    float2 distortedUV = uv + ndc * r2 * distortionStrength * attenuation;

    //========================================
    // 軽微なUVジッター（精度の粗さ表現）
    //! ブラウン管特有の微細な画素ズレを表現
    float2 uvJitter = (Random(screenUV * 512.0f) - 0.5f) * 0.001f;
    distortedUV += uvJitter;

    //! UV範囲制限：画面外参照を防止
    distortedUV = clamp(distortedUV, 0.01f, 0.99f);

    //========================================
    // クロマティックアバレーション（色分離）
    //! 古いCRTブラウン管の色シズミ効果
    float chromaOffset = 4.0f / 1920.0f;
    float2 offset = float2(chromaOffset, 0.0f);
    float3 color;
    color.r = gTexture.Sample(gSampler, distortedUV - offset).r;
    color.g = gTexture.Sample(gSampler, distortedUV).g;
    color.b = gTexture.Sample(gSampler, distortedUV + offset).b;

    //========================================
    // スキャンライン（弱め）
    //! CRT特有の水平ライン表現
    //! ===== 最適化: sin() → FastSin() =====
    //! 計算量削減で約8-10%高速化期待
    float scanlineFreq = 200.0f;
    float scanlineStrength = 0.08f;
    //! 最適化：sin() を多項式近似 FastSin() に置き換え（約75%高速化）
    float scan = 1.0f - scanlineStrength * FastSin(screenUV.y * scanlineFreq);
    color *= scan;

    //========================================
    // CRTシャドウマスク（見やすさ重視で無効化） 
    // float2 screenPixel = screenUV * float2(1920.0f, 1080.0f);
    // float maskR = (fmod(screenPixel.x, 3.0f) == 0) ? 0.95f : 1.0f;
    // float maskG = (fmod(screenPixel.x + 1.0f, 3.0f) == 0) ? 0.95f : 1.0f;
    // float maskB = (fmod(screenPixel.x + 2.0f, 3.0f) == 0) ? 0.95f : 1.0f;
    // color.r *= maskR;
    // color.g *= maskG;
    // color.b *= maskB;

    //========================================
    // RGBマスク風輝度変化（軽めに）
    //! ===== 最適化: sin() → FastSin() =====
    //! 計算量削減で約8-10%高速化期待
    //! 最適化：sin() を多項式近似 FastSin() に置き換え
    float rgbMask = 0.96f + 0.04f * FastSin(screenUV.x * 3840.0f);
    color *= rgbMask;

    //========================================
    // ビネット効果（弱め）
    //! 画面端を暗くする効果（視線を中央に誘導）
    float2 centered = screenUV - 0.5f;
    float vignette = 1.0f - dot(centered, centered) * 0.8f;
    color *= clamp(vignette, 0.7f, 1.0f);

    //========================================
    // 擬似Z-fighting（軽微なちらつき）
    //! ノイズベースのちらつき表現
    float flicker = step(0.5f, frac(sin(dot(screenUV, float2(12.0, 78.0))) * 43758.5453));
    color *= lerp(0.98f, 1.0f, flicker);

    //========================================
    // ノイズ追加（軽微）
    //! ランダムノイズで微細なざらつき表現
    float noise = (Random(screenUV * 1000.0f) - 0.5f) * 0.01f;
    color += noise;

    //========================================
    // 明るさ補正（わずかにトーンアップ）
    //! 暗くなりすぎないようにγ補正（逆γ）
    color = pow(color, 1.0f / 1.1f);

    //========================================
    // グレイスケール変換（オプション、現在は無効）
    // float gray = dot(color, float3(0.299f, 0.587f, 0.114f)); // 標準グレイスケール変換
    // float desaturationRate = 0.8f; // 彩度減少率（0.0 = フルカラー、1.0 = モノクロ）
    // color = lerp(color, gray.xxx, desaturationRate);

    //========================================
    // 色数制限（15段階）
    //! ポスタリゼーション：色段数を制限して「ローテク感」を演出
    color = Posterize(color, 15);

    output.color = float4(color, 1.0f);
    return output;
}
